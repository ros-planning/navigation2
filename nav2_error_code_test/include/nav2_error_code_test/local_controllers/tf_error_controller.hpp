// Copyright (c) 2022 Joshua Wallace
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

#ifndef NAV2_WS_TF_ERROR_CONTROLLER_HPP
#define NAV2_WS_TF_ERROR_CONTROLLER_HPP

#include "nav2_core/controller.hpp"
#include "nav2_core/controller_exceptions.hpp"


namespace nav2_error_code_test
{

class ControllerTFError : public nav2_core::Controller
{
public:
  ControllerTFError() = default;
  ~ControllerTFError() = default;

  void configure(
    const rclcpp_lifecycle::LifecycleNode::WeakPtr &,
    std::string name, std::shared_ptr<tf2_ros::Buffer>,
    std::shared_ptr<nav2_costmap_2d::Costmap2DROS>) override {}

  void cleanup() {}

  void activate() {}

  void deactivate() {}

  void setPlan(const nav_msgs::msg::Path & path) {}

  virtual geometry_msgs::msg::TwistStamped computeVelocityCommands(
    const geometry_msgs::msg::PoseStamped & pose,
    const geometry_msgs::msg::Twist & velocity,
    nav2_core::GoalChecker * goal_checker)
  {
    throw nav2_core::ControllerTFError("TF ERROR");
  }

  void setSpeedLimit(const double & speed_limit, const bool & percentage) {}

};

}  // nav2_error_code_test_controller

#endif //NAV2_WS_TF_ERROR_CONTROLLER_HPP
