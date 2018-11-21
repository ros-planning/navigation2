// Copyright (c) 2018 Intel Corporation
// Copyright (c) 2018 Simbe Robotics
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

#ifndef NAV2_RECOVERY_MANAGER__SPIN_HPP_
#define NAV2_RECOVERY_MANAGER__SPIN_HPP_

#include <string>
#include <memory>

#include "nav2_tasks/spin_task.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "std_msgs/msg/empty.hpp"

namespace nav2_behaviors
{

class Spin : public rclcpp::Node
{
public:
  Spin();
  ~Spin();

  nav2_tasks::TaskStatus spin(const nav2_tasks::SpinCommand::SharedPtr command);

protected:
  std::unique_ptr<nav2_tasks::SpinTaskServer> task_server_;

  std::shared_ptr<rclcpp::Publisher<geometry_msgs::msg::Twist>> vel_pub_;
};

}  // nav2_behaviors

#endif  // NAV2_RECOVERY_MANAGER__SPIN_HPP_