// Copyright (c) 2023, Samsung Research America
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License. Reserved.

#include <math.h>
#include <stdlib.h>
#include <memory>
#include <string>
#include <vector>
#include <cstdlib>

#include "gtest/gtest.h"
#include "nav2_route/node_spatial_tree.hpp"

using namespace nav2_route;  // NOLINT

TEST(NodeSpatialTreeTest, test_kd_tree)
{
  std::cout << "HELLO" << std::endl;
  // Create a large graph of random nodes
  unsigned int seed = time(NULL);
  Graph graph;
  unsigned int graph_size = 10000;
  graph.resize(graph_size);
  for (unsigned int i = 0; i != graph_size; i++) {
    graph[i].nodeid = i;
    graph[i].coords.x = static_cast<float>((rand_r(&seed) % 6000000) + 1);
    graph[i].coords.y = static_cast<float>((rand_r(&seed) % 6000000) + 1);
  }

  std::cout << "Compute Kd tree" << std::endl;
  // Compute our kd tree for this graph
  int num_of_nearest_nodes = 1;
  std::shared_ptr<NodeSpatialTree> kd_tree = 
    std::make_shared<NodeSpatialTree>(num_of_nearest_nodes);
  std::cout << "Constructor" << std::endl;

  kd_tree->computeTree(graph);
  std::cout << "Completed tree" << std::endl;

  // Test a bunch of times to find the nearest neighbor to this node
  // By checking for the idx in the Kd-tree and then brute force searching
  unsigned int num_tests = 50;
  for (unsigned int i = 0; i != num_tests; i++) {
    std::vector<unsigned int> kd_tree_idx;
    geometry_msgs::msg::PoseStamped pose;
    pose.pose.position.x = static_cast<float>((rand_r(&seed) % 6000000) + 1);
    pose.pose.position.y = static_cast<float>((rand_r(&seed) % 6000000) + 1);

    std::cout << "Ran" << std::endl;
    if (!kd_tree->findNearestGraphNodesToPose(pose, kd_tree_idx)) {
      EXPECT_TRUE(false);  // Unable to find nearest neighbor!
    }
    std::cout << "Worked" << std::endl;

    unsigned int closest_via_brute_force;
    float closest_dist = std::numeric_limits<float>::max();
    for (unsigned int j = 0; j != graph_size; j++) {
      float dist = hypotf(
        pose.pose.position.x - graph[j].coords.x,
        pose.pose.position.y - graph[j].coords.y);
      if (dist < closest_dist) {
        closest_dist = dist;
        closest_via_brute_force = j;
      }
    }

    EXPECT_EQ(kd_tree_idx.front(), closest_via_brute_force);
  }
}
