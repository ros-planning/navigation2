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

#include "dwb_controller/dwb_controller.hpp"
#include <string>
#include <chrono>
#include <memory>
#include "dwb_core/exceptions.hpp"
#include "nav_2d_utils/conversions.hpp"

using namespace std::chrono_literals;
using std::shared_ptr;
using nav2_tasks::TaskStatus;
using dwb_core::DWBLocalPlanner;
using dwb_core::CostmapROSPtr;

#define NO_OP_DELETER [] (auto) {}

namespace nav2_dwb_controller
{

DwbController::DwbController(rclcpp::executor::Executor & executor)
: Node("DwbController"),
  tfBuffer_(get_clock()),
  tfListener_(tfBuffer_),
  travel_direction_(TravelDirection::Stopped)
{
  auto temp_node = std::shared_ptr<rclcpp::Node>(this, [](auto) {});

  cm_ = std::make_shared<nav2_costmap_2d::Costmap2DROS>("local_costmap", tfBuffer_);
  executor.add_node(cm_);
  odom_sub_ = std::make_shared<nav_2d_utils::OdomSubscriber>(*this);
  vel_pub_ =
    this->create_publisher<geometry_msgs::msg::Twist>("/cmd_vel", 1);

  auto nh = shared_from_this();
  planner_.initialize(nh, shared_ptr<tf2_ros::Buffer>(&tfBuffer_, NO_OP_DELETER), cm_);

  task_server_ = std::make_unique<nav2_tasks::FollowPathTaskServer>(temp_node);
  task_server_->setExecuteCallback(
    std::bind(&DwbController::followPath, this, std::placeholders::_1));
}

DwbController::~DwbController()
{
}

TaskStatus
DwbController::followPath(const nav2_tasks::FollowPathCommand::SharedPtr command)
{
  RCLCPP_INFO(get_logger(), "Starting controller");
  try {
    auto path = nav_2d_utils::pathToPath2D(*command);

    planner_.setPlan(path);
    RCLCPP_INFO(get_logger(), "Initialized");

    rclcpp::Rate loop_rate(10);
    while (rclcpp::ok()) {
      nav_2d_msgs::msg::Pose2DStamped pose2d;
      if (!getRobotPose(pose2d)) {
        RCLCPP_INFO(get_logger(), "No pose. Stopping robot");
        publishZeroVelocity();
      } else {
        checkRegion(pose2d);
        if (isGoalReached(pose2d)) {
          break;
        }
        auto velocity = odom_sub_->getTwist();
        auto cmd_vel_2d = planner_.computeVelocityCommands(pose2d, velocity);
        publishVelocity(cmd_vel_2d);
        RCLCPP_INFO(get_logger(), "Publishing velocity at time %.2f", now().seconds());

        // Check if this task has been canceled
        if (task_server_->cancelRequested()) {
          RCLCPP_INFO(this->get_logger(), "execute: task has been canceled");
          task_server_->setCanceled();
          publishZeroVelocity();
          return TaskStatus::CANCELED;
        }

        // Check if there is an update to the path to follow
        if (task_server_->updateRequested()) {
          // Get the new, updated path
          auto path_cmd = std::make_shared<nav2_tasks::FollowPathCommand>();
          task_server_->getCommandUpdate(path_cmd);
          task_server_->setUpdated();

          // and pass it to the local planner
          auto path = nav_2d_utils::pathToPath2D(*path_cmd);
          planner_.setPlan(path);
        }
      }
      loop_rate.sleep();
    }
  } catch (nav_core2::PlannerException & e) {
    RCLCPP_INFO(this->get_logger(), e.what());
    publishZeroVelocity();
    return TaskStatus::FAILED;
  }

  nav2_tasks::FollowPathResult result;
  task_server_->setResult(result);
  publishZeroVelocity();

  return TaskStatus::SUCCEEDED;
}

void DwbController::publishVelocity(const nav_2d_msgs::msg::Twist2DStamped & velocity)
{
  auto cmd_vel = nav_2d_utils::twist2Dto3D(velocity.velocity);
  if (cmd_vel.linear.x > 0) {
    travel_direction_ = TravelDirection::MovingForward;
  } else {
    travel_direction_ = TravelDirection::MovingBackwards;
  }
  vel_pub_->publish(cmd_vel);
}

void DwbController::publishZeroVelocity()
{
  travel_direction_ = TravelDirection::Stopped;
  nav_2d_msgs::msg::Twist2DStamped velocity;
  velocity.velocity.x = 0;
  velocity.velocity.y = 0;
  velocity.velocity.theta = 0;
  publishVelocity(velocity);
}

bool DwbController::isGoalReached(const nav_2d_msgs::msg::Pose2DStamped & pose2d)
{
  nav_2d_msgs::msg::Twist2D velocity = odom_sub_->getTwist();
  return planner_.isGoalReached(pose2d, velocity);
}

bool DwbController::getRobotPose(nav_2d_msgs::msg::Pose2DStamped & pose2d)
{
  geometry_msgs::msg::PoseStamped current_pose;
  if (!cm_->getRobotPose(current_pose)) {
    RCLCPP_ERROR(this->get_logger(), "Could not get robot pose");
    return false;
  }
  pose2d = nav_2d_utils::poseStampedToPose2D(current_pose);
  return true;
}

bool DwbController::checkRegion(nav_2d_msgs::msg::Pose2DStamped & pose2d)
{
  nav2_world_model::FreeSpaceServiceRequest request;

  // TODO(orduno) get from robot class
  double robot_width = 0.22;

  // Define the region size
  request.width = robot_width;
  request.height = robot_width * 3;

  // Define the reference point
  request.reference.x = pose2d.pose.x;
  request.reference.y = pose2d.pose.y;

  // Translate to set the edge of the region in front of the robot
  request.offset.x = 0.0;
  request.offset.y = robot_width / 2.0 + request.height / 2.0;

  // Rotate to match the robot travel direction
  request.rotation = pose2d.pose.theta;
  if (travel_direction_ == TravelDirection::MovingBackwards) {
    request.rotation += M_PI;
  }

  return world_model_.confirmFreeSpace(request);
}

}  // namespace nav2_dwb_controller
