project(GraphLab)

cmake_minimum_required(VERSION 2.8.3)

cmake_policy(SET CMP0003 NEW)
cmake_policy(SET CMP0005 NEW)
cmake_policy(SET CMP0054 NEW)

if(${CMAKE_MAJOR_VERSION} GREATER 2)
    cmake_policy(SET CMP0045 OLD)
    cmake_policy(SET CMP0046 OLD)
    cmake_policy(SET CMP0042 NEW)
endif()

set(DATO_DEPS_CMAKE ${CMAKE_SOURCE_DIR}/cmake)
set(DATO_DEPS_LOCAL_CMAKE ${CMAKE_SOURCE_DIR}/local_cmake)
set(CMAKE_MODULE_PATH ${DATO_DEPS_CMAKE};${DATO_DEPS_LOCAL_CMAKE};${CMAKE_SOURCE_DIR}/cmake_utils)
set(CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/deps/local)
set(ENV{PYTHONHOME} ${CMAKE_SOURCE_DIR}/deps/conda)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(CheckLibraryExists)
include(CheckFunctionExists)
include(ExternalProject)
include(CheckCXXSourceCompiles)

include(ColorFormat)

#***********************************#
#*                                 *#
#*  Global Link and Include Flags  *#
#*                                 *#
#***********************************#
include_directories(SYSTEM
    ${CMAKE_SOURCE_DIR}/cxxtest
    ${CMAKE_SOURCE_DIR}/deps/local/include)

link_directories(${CMAKE_SOURCE_DIR}/deps/local/lib)
if(EXISTS ${CMAKE_SOURCE_DIR}/deps/local/lib64)
    link_directories(${CMAKE_SOURCE_DIR}/deps/local/lib64)
endif()

#***********************************#
#*                                 *#
#*         CCache Support          *# 
#*                                 *#
#***********************************#
find_program(HAS_CACHE ccache)
if (HAS_CACHE)
    message("-- ${Magenta}CCache Detected${ColourReset}")
    SET_PROPERTY(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    SET_PROPERTY(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif()

#***********************************#
#*                                 *#
#*         Platform Specific       *# 
#*                                 *#
#***********************************#
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CLANG false)
elseif(WIN32)
    set(CLANG false)
else()
    set(CLANG true)
endif()

if(CMAKE_GENERATOR MATCHES "MinGW Makefiles")
    set(MINGW_MAKEFILES true)
else()
    set(MINGW_MAKEFILES false)
endif()

if(CMAKE_GENERATOR MATCHES "MSYS Makefiles")
    set(MSYS_MAKEFILES true)
else()
    set(MSYS_MAKEFILES false)
endif()

if(CMAKE_GENERATOR MATCHES "Visual Studio*")
endif()

if(WIN32)
	if(MINGW)
    	set(COMPILER_FLAGS "${COMPILER_FLAGS} -Wa,-mbig-obj")
	elseif(MSVC)
		set(COMPILER_FLAGS "${COMPILER_FLAGS} /bigobj")
	endif()
endif()

set(MINGW_ROOT "/mingw64/bin")

# Seperate variables so that cython's CMakeLists.txt can use it too
if(WIN32)
    set(INSTALLATION_SYSTEM_BINARY_FILES 
        ${CMAKE_SOURCE_DIR}/deps/local/bin/libzmq.dll
        ${CMAKE_SOURCE_DIR}/deps/local/bin/libsodium-13.dll
        ${MINGW_ROOT}/libconv-2.dll
        ${MINGW_ROOT}/libssh2-1.dll
        ${MINGW_ROOT}/zlib1.dll
        ${MINGW_ROOT}/libwinpthread-1.dll
        ${MINGW_ROOT}/libgcc_s_seh-1.dll
        ${MINGW_ROOT}/libstdc++-6.dll
        ${MINGW_ROOT}/libeay32.dll)
    message("${INSTALLATION_SYSTEM_BINARY_FILES}")
else()
    if(EXISTS ${CMAKE_SOURCE_DIR}/deps/local/lib64/libgomp.so.1)
        list(APPEND INSTALLATION_SYSTEM_BINARY_FILES ${CMAKE_SOURCE_DIR}/deps/local/lib64/libgomp.so.1)
    endif()
endif()

#***********************************#
#*                                 *#
#*         Common Defines          *# 
#*                                 *#
#***********************************#
if(WIN32)
    add_definitions(-DWINVER=0x0600)
    add_definitions(-D_WIN32_WINNT=0x0600)
endif()

add_definitions(-DCURL_STATICLIB)
add_definitions(-DIN_GRAPHLAB_SOURCE_TREE)
add_definitions(0DFUSION_MAX_VECTOR_SIZE=20)

#***********************************#
#*                                 *#
#*         Locate OpenMP           *# 
#*                                 *#
#***********************************#
if(EXISTS ${CMAKE_SOURCE_DIR}/deps/local/lib/libgomp.so)
    add_library(libgomp.so SHARED IMPORTED)
    set_property(TARGET libgomp.so PROPERTY IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/deps/local/lib/libgomp.so)

    add_library(openmp INTERFACE)
    target_compile_options(openmp INTERFACE "-fopenmp")
    target_link_libraries(openmp INTERFACE libgomp.so)
    message(STATUS "OpenMP Library Found")
elseif(EXISTS ${CMAKE_SOURCE_DIR}/deps/local/lib64/libgomp.so)
    add_library(libgomp.so SHARED IMPORTED)
    set_property(TARGET libgomp.so PROPERTY IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/deps/local/lib64/libgomp.so)

    add_library(openmp INTERFACE)
    target_compile_options(openmp INTERFACE "-fopenmp")
    target_link_libraries(openmp INTERFACE libgomp.so)
    message(STATUS "OpenMP Library Found")
else()
    add_library(openmp INTERFACE)
    target_compile_definitions(openmp INTERFACE __NO_OPENMP__)
    message(WARNING "OpenMP Libraries Not Found")
endif()

find_package(OpenMP)
if(OPENMP_FOUND)
	message("OpenMP!!!")
endif()

#***********************************#
#*                                 *#
#*     Compiler & Linker Flags     *# 
#*                                 *#
#***********************************#
if(CLANG)
    set(CPP11_FLAGS "-std=c++11 -stdlib=libc++ -Wno-deprecated-register -Wno-enum-compare -Wno-conversion-null -ftemplate-depth=900" 
        CACHE STRING "C++11 enabling flags")
else()
    set(CPP11_FLAGS "-std=c++11 -Wno-enum-compare -Wno-conversion-null -ftemplate-depth=900"
        CACHE STRING "C++11 enabling flags")
endif()

set(C_REAL_COMPILER_FLAGS "-Wall -pthread -g ${COMPILER_FLAGS}" CACHE STRING "common compiler options")
set(CPP_REAL_COMPILER_FLAGS "-Wall -pthread -g ${CPP11_FLAGS} ${COMPILER_FLAGS}" CACHE STRING "common compiler options")

# Disable address space randomization for OS X lion and above
if(APPLE)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Xlinker -no_pie")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Xlinker -no_pie")
    if(NOT CLANG)
        set(ALTERNATE_LINKER "-Wa,-q")
    endif()
endif()

if(WIN32)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath,${CMAKE_SOURCE_DIR}/deps/local/bin")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath,/mingw64/bin")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,-rpath,/mingw64/bin")
endif()
