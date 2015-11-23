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
#include <sgraph/sgraph_triple_apply.hpp>
#include <boost/bind.hpp>
#include <cxxtest/TestSuite.h>

#include "sgraph_test_util.hpp"
#include "sgraph_check_degree_count.hpp"
#include "sgraph_check_pagerank.hpp"

using namespace graphlab;

// Implement degree count function using triple_apply 
std::vector<std::pair<flexible_type, flexible_type>> triple_apply_degree_count(
  sgraph& g, sgraph::edge_direction dir, bool use_batch_triple_apply_mock) {

  sgraph_compute::triple_apply_fn_type fn;
  g.init_vertex_field("__degree__", flex_int(0));
  std::vector<std::string> vertex_fields = g.get_vertex_fields();
  size_t degree_idx;
  for (size_t i = 0; i < vertex_fields.size(); ++i) {
    if (vertex_fields[i] == "__degree__") degree_idx = i;
  }

  if (dir == sgraph::edge_direction::IN_EDGE) {
    fn = [=](sgraph_compute::edge_scope& scope) {
      scope.lock_vertices();
      scope.target()[degree_idx]++;
      scope.unlock_vertices();
    };
  } else if (dir == sgraph::edge_direction::OUT_EDGE) {
    fn = [=](sgraph_compute::edge_scope& scope) {
      scope.lock_vertices();
      scope.source()[degree_idx]++;
      scope.unlock_vertices();
    };
  } else {
    fn = [=](sgraph_compute::edge_scope& scope) {
      scope.lock_vertices();
      scope.source()[degree_idx]++;
      scope.target()[degree_idx]++;
      scope.unlock_vertices();
    };
  }

  if (use_batch_triple_apply_mock) {
    sgraph_compute::batch_triple_apply_mock(g, fn, {"__degree__"});
  } else {
    sgraph_compute::triple_apply(g, fn, {"__degree__"});
  }

  auto result = g.fetch_vertex_data_field("__degree__");
  auto vertex_ids = g.fetch_vertex_data_field(sgraph::VID_COLUMN_NAME);
  std::vector<std::pair<flexible_type, flexible_type>> ret;
  for (size_t i = 0; i < result.size(); ++i) {
    std::vector<flexible_type> degree_vec;
    std::vector<flexible_type> id_vec;
    result[i]->get_reader()->read_rows(0, g.num_vertices(), degree_vec);
    vertex_ids[i]->get_reader()->read_rows(0, g.num_vertices(), id_vec);
    for (size_t j = 0; j < degree_vec.size(); ++j) {
      ret.push_back({id_vec[j], degree_vec[j]});
    }
  }
  g.remove_vertex_field("__degree__");
  return ret;
}

class sgraph_triple_apply_test : public CxxTest::TestSuite {

public:

void test_triple_apply_degree_count() {
  degree_count_fn_type f = boost::bind(&triple_apply_degree_count, _1, _2, false);
  check_degree_count(f);
}

void test_batch_triple_apply_degree_count() {
  degree_count_fn_type f = boost::bind(&triple_apply_degree_count, _1, _2, true);
  check_degree_count(f);
}

void test_triple_apply_edge_data_modification() {
  // Create an edge field, and assign it the value of the sum of source and target ids.
  size_t n_vertex = 1000;
  size_t n_partition = 4;
  sgraph g = create_ring_graph(n_vertex, n_partition, false /* one direction */);

  for (size_t i = 0; i < 2; ++i) {
    g.init_edge_field("id_sum", flex_int(0));
    size_t field_id = g.get_edge_field_id("id_sum");
    TS_ASSERT_EQUALS(field_id, 3);
    if (i == 0) {
      // test regular triple_apply
      sgraph_compute::triple_apply(g,
                                   [=](sgraph_compute::edge_scope& scope) {
                                     scope.edge()[field_id] = scope.source()[0] + scope.target()[0];
                                   },
                                   {}, {"id_sum"});
    } else if (i == 1) {
      // test batch triple_apply
      sgraph_compute::batch_triple_apply_mock(g,
                                              [=](sgraph_compute::edge_scope& scope) {
                                                scope.edge()[field_id] = scope.source()[0] + scope.target()[0];
                                              },
                                              {}, {"id_sum"});
    }
    sframe edge_sframe = g.get_edges();
    std::vector<std::vector<flexible_type>> edge_data_rows;
    edge_sframe.get_reader()->read_rows(0, edge_sframe.size(), edge_data_rows);
    for (auto& row : edge_data_rows) {
      TS_ASSERT_EQUALS(int(row[0] + row[1]), int(row[3]));
    }
    g.remove_edge_field("id_sum");
  }
}

};
