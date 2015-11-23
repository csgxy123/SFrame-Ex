/*
* Copyright (C) 2015 Dato, Inc.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Affero General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <sgraph/sgraph.hpp>
#include <sgraph/sgraph_vertex_apply.hpp>
#include <cxxtest/TestSuite.h>
#include "sgraph_test_util.hpp"

using namespace graphlab;

class sgraph_vertex_apply_test: public CxxTest::TestSuite {

public:

void check_vertex_apply_result(std::vector<std::shared_ptr<sarray<flexible_type>>>& val) {
  for(auto& iter: val) {
    std::vector<flexible_type> ret;
    graphlab::copy(*iter, std::inserter(ret, ret.end()));
    for (auto& retval: ret) {
      TS_ASSERT_EQUALS((int)retval.get_type(), (int)flex_type_enum::FLOAT);
      TS_ASSERT_EQUALS(retval.get<double>(), 2.0);
    }
  }
}

void test_vertex_apply() {
  // there are 4 overloads
  size_t n_vertex = 10;
  size_t n_partition = 2;
  sgraph ring_graph  = create_ring_graph(n_vertex, n_partition, false);
  size_t data_index = ring_graph.vertex_group(0)[0].column_index("vdata");
  // map data + 1 = 2.0
  auto ret = sgraph_compute::vertex_apply(ring_graph,
                                          flex_type_enum::FLOAT,
                                          [=](const std::vector<flexible_type>& val){
                                            TS_ASSERT_LESS_THAN(data_index, val.size());
                                            return val[data_index] + 1.0;
                                          });
  check_vertex_apply_result(ret);


  // map data + prevret / 2 = 2.0
  ret = sgraph_compute::vertex_apply(ring_graph,
                                     ret,
                                     flex_type_enum::FLOAT,
                                     [=](const std::vector<flexible_type>& val, flexible_type prev_ret){
                                       TS_ASSERT_LESS_THAN(data_index, val.size());
                                       return val[data_index] + prev_ret / 2; 
                                     });
  check_vertex_apply_result(ret);



  // map data + prevret / 2 = 2.0
  ret = sgraph_compute::vertex_apply(ring_graph,
                                     "vdata",
                                     ret,
                                     flex_type_enum::FLOAT,
                                     [=](const flexible_type& val, flexible_type prev_ret){
                                       return val + prev_ret / 2; 
                                     });
  check_vertex_apply_result(ret);


  // map data + 1 = 2.0
  ret = sgraph_compute::vertex_apply(ring_graph,
                                     "vdata",
                                     flex_type_enum::FLOAT,
                                     [=](const flexible_type& val){
                                       return val + 1.0;
                                     });
  check_vertex_apply_result(ret);

  double vsum = sgraph_compute::vertex_reduce<double>(ring_graph,
                                      [=](const std::vector<flexible_type>& val, double& sum) {
                                        TS_ASSERT_LESS_THAN(data_index, val.size());
                                        sum += (double)val[data_index];
                                      }, 
                                      [=](const double& val, double& sum) {
                                        sum += val; 
                                      });
  TS_ASSERT_EQUALS(vsum, n_vertex);


  flexible_type vsum2 = sgraph_compute::vertex_reduce<flexible_type>(ring_graph,
                                               "vdata",
                                               [=](const flexible_type& val, flexible_type& sum) {
                                                 sum += val;
                                               },
                                               [=](const flexible_type& val, flexible_type& sum) {
                                                 sum += val;
                                               });
  TS_ASSERT_EQUALS(vsum2, n_vertex);

}
};
