// Copyright (c) 2020, Samsung Research America
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

#ifndef SMAC_PLANNER__NODE_BASIC_HPP_
#define SMAC_PLANNER__NODE_BASIC_HPP_

#include <math.h>
#include <vector>
#include <cmath>
#include <iostream>
#include <functional>
#include <queue>
#include <memory>
#include <utility>
#include <limits>

#include "ompl/base/StateSpace.h"

#include "smac_planner/constants.hpp"
#include "smac_planner/node_se2.hpp"
#include "smac_planner/node_2d.hpp"
#include "smac_planner/types.hpp"
#include "smac_planner/collision_checker.hpp"

namespace smac_planner
{

/**
 * @class smac_planner::NodeBasic
 * @brief NodeBasic implementation for priority queue insertion
 */
template<typename NodeT>
class NodeBasic
{
public:
  /**
   * @brief A constructor for smac_planner::NodeBasic
   * @param cost_in The costmap cost at this node
   * @param index The index of this node for self-reference
   */
  explicit NodeBasic(const unsigned int index)
  : _index(index),
    graph_node_ptr(nullptr)
  {}

  /**
   * @brief A destructor for smac_planner::NodeBasic
   */
  ~NodeBasic() {}

  /**
   * @brief Gets cell index
   * @return Reference to cell index
   */
  inline unsigned int & getIndex()
  {
    return _index;
  }

  typename NodeT::Coordinates pose;  // Used by NodeSE2
  NodeT * graph_node_ptr;
  unsigned int index;
};

template class NodeBasic<Node2D>;
template class NodeBasic<NodeSE2>;

}  // namespace smac_planner

#endif  // SMAC_PLANNER__NODE_BASIC_HPP_
