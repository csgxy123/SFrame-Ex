/**
 * Copyright (C) 2015 Dato, Inc.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 */
#ifndef SFRAME_QUERY_ENGINE_TEST_UTIL_CHECK_NODE
#define SFRAME_QUERY_ENGINE_TEST_UTIL_CHECK_NODE

#include <sframe_query_engine/execution/execution_node.hpp>
#include<flexible_type/flexible_type.hpp>
#include <cxxtest/TestSuite.h>

namespace graphlab {
namespace query_eval {

class execution_node;

inline void check_node(std::shared_ptr<execution_node> node, std::vector<flexible_type> expected) {
    size_t consumer_id = node->register_consumer();
    std::vector<flexible_type> actual;
    while(1) {
      auto rows = node->get_next(consumer_id);
      if (rows == nullptr) break;
      for(const auto& val: *rows) {
        TS_ASSERT_EQUALS(val.size(), 1);
        actual.push_back(val[0]);
      }
    }
    TS_ASSERT_EQUALS(actual.size(), expected.size());
    for (size_t i = 0; i < actual.size(); ++i) {
      TS_ASSERT_EQUALS(actual[i], expected[i]);
    }
  }

inline void check_node(std::shared_ptr<execution_node> node, std::vector<std::vector<flexible_type>> expected) {
    size_t consumer_id = node->register_consumer();
    std::vector<std::vector<flexible_type>> actual;
    while(1) {
      auto rows = node->get_next(consumer_id);
      if (rows == nullptr) break;
      for(const auto& val: *rows) {
        actual.push_back(val);
      }
    }

    TS_ASSERT_EQUALS(actual.size(), expected.size());
    for (size_t i = 0; i < actual.size(); ++i) {
      TS_ASSERT_EQUALS(actual[i].size(), expected[i].size());
      for (size_t j = 0; j < actual[i].size(); ++j)
        TS_ASSERT_EQUALS(actual[i][j], expected[i][j]);
    }
  }

inline void check_node_throws(std::shared_ptr<execution_node> node) {
    size_t consumer_id = node->register_consumer();
    TS_ASSERT_THROWS_ANYTHING(node->get_next(consumer_id));
  }

  }
}
#endif
