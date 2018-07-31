// Copyright (c) 2018 Intel Corporation
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

#ifndef PLANNING__POINTTOPOINTPLANNINGTASKSERVER_HPP_
#define PLANNING__POINTTOPOINTPLANNINGTASKSERVER_HPP_

#include "task/TaskServer.hpp"
#include "nav2_msgs/msg/path_end_points.hpp"
#include "nav2_msgs/msg/path.hpp"

using PointToPointPlanningCommand = nav2_msgs::msg::PathEndPoints;
using PointToPointPlanningResult = nav2_msgs::msg::Path;

typedef TaskServer<PointToPointPlanningCommand, PointToPointPlanningResult>
  PointToPointPlanningTaskServer;

#endif  // PLANNING__POINTTOPOINTPLANNINGTASKSERVER_HPP_
