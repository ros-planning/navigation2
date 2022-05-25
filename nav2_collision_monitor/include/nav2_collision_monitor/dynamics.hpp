// Copyright (c) 2022 Samsung Research Russia
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

#ifndef NAV2_COLLISION_MONITOR__DYNAMICS_HPP_
#define NAV2_COLLISION_MONITOR__DYNAMICS_HPP_

#include "nav2_collision_monitor/types.hpp"

namespace nav2_collision_monitor
{

// Interpolates the position of fixed point in robot_base frame
// moving with towards velocity for dt time interval
void fixPoint(const Velocity & velocity, const double dt, Point & point);

// Transforms point to current_pose frame
void fixPoint(const Pose & curr_pose, Point & point);

// Linearly moves pose towards to velocity direction on dt time interval.
// Velocity is being rotated on twist angle for dt time interval.
// NOTE: dt should be relatively small to consider all movements to be linear
void movePose(Velocity & velocity, const double dt, Pose & pose);

}  // namespace nav2_collision_monitor

#endif  // NAV2_COLLISION_MONITOR__DYNAMICS_HPP_
