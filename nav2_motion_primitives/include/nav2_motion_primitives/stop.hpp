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

#ifndef NAV2_MOTION_PRIMITIVES__STOP_HPP_
#define NAV2_MOTION_PRIMITIVES__STOP_HPP_

#include <chrono>
#include <memory>

#include "nav2_motion_primitives/motion_primitive.hpp"
#include "nav2_msgs/action/empty.hpp"
#include "nav2_tasks/follow_path_task.hpp"

namespace nav2_motion_primitives
{
using StopAction = nav2_msgs::action::Empty;

class Stop : public MotionPrimitive<StopAction>
{
public:
  explicit Stop(rclcpp::Node::SharedPtr & node);
  ~Stop();

  Status onRun(const std::shared_ptr<const StopAction::Goal> command) override;

  Status onCycleUpdate() override;

protected:
  // For stopping the path following controller from sending commands to the robot
  std::unique_ptr<nav2_tasks::FollowPathTaskClient> controller_client_;
};

}  // namespace nav2_motion_primitives

#endif  // NAV2_MOTION_PRIMITIVES__STOP_HPP_
