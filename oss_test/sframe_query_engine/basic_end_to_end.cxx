/**
 * Copyright (C) 2015 Dato, Inc.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 */
#include <sframe_query_engine/planning/planner.hpp>
#include <sframe_query_engine/planning/planner_node.hpp>
#include <sframe_query_engine/operators/all_operators.hpp>
#include <sframe_query_engine/util/aggregates.hpp>
#include <sframe/sarray.hpp>
#include <cxxtest/TestSuite.h>

using namespace graphlab;
using namespace graphlab::query_eval;

class basic_end_to_end: public CxxTest::TestSuite {
 public:

  void test_basic_linear() {
    const size_t TEST_LENGTH = 128 + 64;
    std::vector<flexible_type> data;
    for (size_t i = 0;i < TEST_LENGTH; ++i) data.push_back(i);
    auto sa = std::make_shared<sarray<flexible_type>>();
    sa->open_for_write();
    graphlab::copy(data.begin(), data.end(), *sa);
    sa->close();

    auto root = op_sarray_source::make_planner_node(sa);

    // add_one = root + 1
    auto add_one = 
        op_transform::make_planner_node(
            root, 
            [](const sframe_rows::row& a)->flexible_type {
              return a[0] + 1;
            },
            flex_type_enum::INTEGER);

    // sum_both = add_one + root
    //
    auto sum_both = 
        op_binary_transform::make_planner_node(
            root, 
            add_one,
            [](const sframe_rows::row& a, 
               const sframe_rows::row& b)->flexible_type {
              return a[0] + b[0];
            },
            flex_type_enum::INTEGER);

    auto res = planner().materialize(sum_both);
    std::vector<flexible_type> all_rows;
    res.select_column(0)->get_reader()->read_rows(0, res.size(), all_rows);

    TS_ASSERT_EQUALS(all_rows.size(), TEST_LENGTH);
    for (flex_int i = 0;i < TEST_LENGTH; ++i) {
      flex_int j = (flex_int)(all_rows[i]);
      TS_ASSERT_EQUALS(2*i+1, j);
    }
  }

  void test_sub_linear() {
    const size_t TEST_LENGTH = 1000000;
    std::vector<flexible_type> data;
    for (size_t i = 0;i < TEST_LENGTH; ++i) data.push_back(i);
    auto sa = std::make_shared<sarray<flexible_type>>();
    sa->open_for_write();
    graphlab::copy(data.begin(), data.end(), *sa);
    sa->close();

    auto root = op_sarray_source::make_planner_node(sa);

    // even_selector = root % 2 == 0
    auto even_selector = 
        op_transform::make_planner_node(
            root, 
            [](const sframe_rows::row& a)->flexible_type {
              return (flex_int)(a[0]) % 2 == 0;
            },
            flex_type_enum::INTEGER);

    // filter = root[even_selector]
    auto filter = op_logical_filter::make_planner_node(root, even_selector);

    auto res = planner().materialize(filter);
    std::vector<flexible_type> all_rows;
    res.select_column(0)->get_reader()->read_rows(0, res.size(), all_rows);

    TS_ASSERT_EQUALS(all_rows.size(), TEST_LENGTH / 2);
    for (flex_int i = 0;i < TEST_LENGTH / 2; ++i) {
      TS_ASSERT_EQUALS(2*i, all_rows[i]);
    }
  }


  void test_diamond() {
    const size_t TEST_LENGTH = 1000;
    std::vector<flexible_type> data;
    for (size_t i = 0;i < TEST_LENGTH; ++i) data.push_back(i);
    auto sa = std::make_shared<sarray<flexible_type>>();
    sa->open_for_write();
    graphlab::copy(data.begin(), data.end(), *sa);
    sa->close();

    auto root = op_sarray_source::make_planner_node(sa);

    // even_selector = root % 2 == 0
    auto even_selector = 
        op_transform::make_planner_node(
            root, 
            [](const sframe_rows::row& a)->flexible_type {
              return (flex_int)(a[0]) % 2 == 0;
            },
            flex_type_enum::INTEGER);

    // add_one = root + 1
    auto add_one = 
        op_transform::make_planner_node(
            root, 
            [](const sframe_rows::row& a)->flexible_type {
              return a[0] + 1;
            },
            flex_type_enum::INTEGER);

    // filter = add_one[even_selector]
    auto filter = op_logical_filter::make_planner_node(add_one, even_selector);

    auto res = planner().materialize(filter);
    std::vector<flexible_type> all_rows;
    res.select_column(0)->get_reader()->read_rows(0, res.size(), all_rows);

    TS_ASSERT_EQUALS(all_rows.size(), TEST_LENGTH / 2);
    for (flex_int i = 0;i < TEST_LENGTH / 2; ++i) {
      TS_ASSERT_EQUALS(2*i + 1, all_rows[i]);
    }
  }
  void test_reduction_aggregate() {
    const size_t TEST_LENGTH = 1000000;
    std::vector<flexible_type> data;
    for (size_t i = 0;i < TEST_LENGTH; ++i) data.push_back(i);
    auto sa = std::make_shared<sarray<flexible_type>>();
    sa->open_for_write();
    graphlab::copy(data.begin(), data.end(), *sa);
    sa->close();
    auto root = op_sarray_source::make_planner_node(sa);
    flex_int m = query_eval::reduce<flex_int>(root, 
                                 [](const flexible_type& f, 
                                    flex_int& val) {
                                      if (f > val) val = f;
                                    },
                                  [](const flex_int& f,
                                     flex_int& val) {
                                       if (f > val) val = f;
                                   },
                                   0);
    TS_ASSERT_EQUALS(m, TEST_LENGTH - 1);
  }
};
