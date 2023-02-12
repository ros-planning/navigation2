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
// limitations under the License.

#ifndef NAV2_ROUTE__PLUGINS__EDGE_COST_FUNCTIONS__SEMANTIC_SCORER_HPP_
#define NAV2_ROUTE__PLUGINS__EDGE_COST_FUNCTIONS__SEMANTIC_SCORER_HPP_

#include <memory>
#include <string>
#include <unordered_map>

#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/lifecycle_node.hpp"
#include "nav2_route/interfaces/edge_cost_function.hpp"
#include "nav2_util/node_utils.hpp"

namespace nav2_route
{

/**
 * @class SemanticScorer
 * @brief Scores an edge based on arbitrary graph semantic data such as set priority/danger
 * levels or regional attributes (e.g. living room, bathroom, work cell 2)
 */
class SemanticScorer : public EdgeCostFunction
{
public:
  /**
   * @brief Constructor
   */
  SemanticScorer() = default;

  /**
   * @brief destructor
   */
  virtual ~SemanticScorer() = default;

  /**
   * @brief Configure
   */
  void configure(
    const rclcpp_lifecycle::LifecycleNode::SharedPtr node,
    const std::string & name) override;

  /**
   * @brief Main scoring plugin API
   * @param edge The edge pointer to score, which has access to the
   * start/end nodes and their associated metadata and actions
   * @param cost of the edge scored
   * @return bool if this edge is open valid to traverse
   */
  bool score(const EdgePtr edge, float & cost) override;

  /**
   * @brief Get name of the plugin for parameter scope mapping
   * @return Name
   */
  std::string getName() override;

protected:
  std::string name_, key_;
  std::unordered_map<std::string, float> semantic_info_;
  float weight_;
};

}  // namespace nav2_route

#endif  // NAV2_ROUTE__PLUGINS__EDGE_COST_FUNCTIONS__SEMANTIC_SCORER_HPP_