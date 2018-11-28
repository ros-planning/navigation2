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

#include <cmath>
#include <chrono>
#include <ctime>
#include <thread>
#include <algorithm>
#include <memory>

#include "nav2_behaviors/spin.hpp"
#include "tf2/LinearMath/Quaternion.h"
#include "tf2/LinearMath/Matrix3x3.h"

using nav2_tasks::TaskStatus;
using namespace std::chrono_literals;

namespace nav2_behaviors
{

Spin::Spin()
: Behavior<nav2_tasks::SpinCommand, nav2_tasks::SpinResult>("Spin")
{
  // TODO(orduno) Pull values from param server or robot
  max_rotational_vel_ = 1.0;
  min_rotational_vel_ = 0.4;
  rotational_acc_lim_ = 3.2;
  goal_tolerance_angle_ = 0.10;
}

Spin::~Spin()
{
}

nav2_tasks::TaskStatus Spin::onRun(const nav2_tasks::SpinCommand::SharedPtr command)
{
  double yaw, pitch, roll;
  getAnglesFromQuaternion(command->quaternion, yaw, pitch, roll);

  if (roll != 0.0 || pitch != 0.0) {
    RCLCPP_INFO(get_logger(), "Spinning on Y and X not supported, "
      "will only spin in Z.");
  }

  RCLCPP_INFO(get_logger(), "Currently only supported spinning by a fixed amount");

  return nav2_tasks::TaskStatus::SUCCEEDED;
}

nav2_tasks::TaskStatus Spin::onCycleUpdate(nav2_tasks::SpinResult & result)
{
  * start_time_ = std::chrono::system_clock::now();

  // Currently only an open-loop controller is implemented
  // TODO(orduno) Create a base class for open-loop controlled behaviors
  //              controlledSpin() has not been fully tested
  TaskStatus status = timedSpin();

  // For now sending an empty task result
  nav2_tasks::SpinResult empty_result;
  result = empty_result;


  return status;
}

nav2_tasks::TaskStatus Spin::timedSpin()
{
  // Output control command
  geometry_msgs::msg::Twist cmd_vel;

  // TODO(orduno) fixed speed
  cmd_vel.linear.x = 0.0;
  cmd_vel.linear.y = 0.0;
  cmd_vel.angular.z = 0.5;
  vel_pub_->publish(cmd_vel);

  // TODO(orduno) fixed time
  auto current_time = std::chrono::system_clock::now();
  if (current_time - * start_time_ >= 4s) {
    // Stop the robot
    cmd_vel.angular.z = 0.0;
    vel_pub_->publish(cmd_vel);
    RCLCPP_INFO(get_logger(), "Completed rotation");
    return TaskStatus::SUCCEEDED;
  }

  return TaskStatus::RUNNING;
}

nav2_tasks::TaskStatus Spin::controlledSpin()
{
  // TODO(orduno) Test controller

  // Get current robot orientation
  double start_yaw, current_yaw;

  if (!getRobotYaw(start_yaw)) {
    return TaskStatus::FAILED;
  }

  while (true) {
    if (task_server_->cancelRequested()) {
      RCLCPP_INFO(get_logger(), "Task cancelled");
      task_server_->setCanceled();
      return TaskStatus::CANCELED;
    }

    getRobotYaw(current_yaw);

    double current_angle = current_yaw - start_yaw;

    double dist_left = M_PI - current_angle;

    // TODO(orduno) forward simulation to check if future position is feasible

    // compute the velocity that will let us stop by the time we reach the goal
    // v_f^2 == v_i^2 + 2 * a * d
    // solving for v_i if v_f = 0
    double vel = sqrt(2 * rotational_acc_lim_ * dist_left);

    // limit velocity
    vel = std::min(std::max(vel, min_rotational_vel_), max_rotational_vel_);

    geometry_msgs::msg::Twist cmd_vel;
    cmd_vel.linear.x = 0.0;
    cmd_vel.linear.y = 0.0;
    cmd_vel.angular.z = vel;

    vel_pub_->publish(cmd_vel);

    // check if we are done
    if (dist_left >= (0.0 - goal_tolerance_angle_)) {
      break;
    }
  }

  return TaskStatus::SUCCEEDED;
}

void Spin::getAnglesFromQuaternion(
  const geometry_msgs::msg::Quaternion & quaternion,
  double & yaw, double & pitch, double & roll)
{
  tf2::Matrix3x3(
    tf2::Quaternion(
      quaternion.x, quaternion.y, quaternion.z, quaternion.w)).getEulerYPR(yaw, pitch, roll);
}

bool Spin::getRobotYaw(double & yaw)
{
  auto current_pose = std::make_shared<geometry_msgs::msg::PoseWithCovarianceStamped>();

  if (!robot_->getCurrentPose(current_pose)) {
    RCLCPP_ERROR(get_logger(), "Current robot pose is not available.");
    return false;
  }

  double pitch, roll;
  getAnglesFromQuaternion(current_pose->pose.pose.orientation, yaw, pitch, roll);

  return true;
}

}  // namespace nav2_behaviors
