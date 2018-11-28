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

#ifndef NAV2_BEHAVIORS__BEHAVIOR_HPP_
#define NAV2_BEHAVIORS__BEHAVIOR_HPP_


#include <memory>
#include <string>
#include <cmath>
#include <chrono>
#include <ctime>
#include <thread>

#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "nav2_tasks/task_status.hpp"
#include "nav2_tasks/task_server.hpp"
#include "nav2_robot/robot.hpp"

namespace nav2_behaviors
{

using namespace std::chrono_literals;

template<class CommandMsg, class ResultMsg>
const char * getTaskName();

template<class CommandMsg, class ResultMsg>
class Behavior : public rclcpp::Node
{
public:
  explicit Behavior(const std::string & node_name)
  : Node(node_name),
    task_server_(nullptr),
    taskName_(nav2_tasks::getTaskName<CommandMsg, ResultMsg>())
  {
    auto temp_node = std::shared_ptr<rclcpp::Node>(this, [](auto) {});

    robot_ = std::make_unique<nav2_robot::Robot>(temp_node);

    task_server_ = std::make_unique<nav2_tasks::TaskServer<CommandMsg, ResultMsg>>(
      temp_node, false);

    task_server_->setExecuteCallback(std::bind(&Behavior::run, this, std::placeholders::_1));

    // Start listening for incoming Spin task requests
    task_server_->startWorkerThread();

    RCLCPP_INFO(get_logger(), "Initialized the %s server", taskName_.c_str());
  }

  virtual ~Behavior() {}

  // Derived classes can override this method to catch the command and perform some checks
  // before getting into the main loop. The method will only be called
  // once and should return SUCCEEDED otherwise behavior will return FAILED.
  virtual nav2_tasks::TaskStatus onRun(const typename /*nav2_tasks::*/ CommandMsg::SharedPtr command) = 0;

  // This is the method derived classes should mainly implement
  // and will be called cyclically while it returns RUNNING.
  // Implement the behavior such that it runs some unit of work on each call
  // and provides a status.
  virtual nav2_tasks::TaskStatus onCycleUpdate(/*nav2_tasks::*/ ResultMsg & result) = 0;

  // Runs the behavior
  nav2_tasks::TaskStatus run(const typename /*nav2_tasks::*/ CommandMsg::SharedPtr command)
  {
    RCLCPP_INFO(get_logger(), "Attempting behavior");


    ResultMsg result;
    nav2_tasks::TaskStatus status;

    status = onRun(command);

    if (status == nav2_tasks::TaskStatus::SUCCEEDED) {
      status = cycle(result);
    }

    task_server_->setResult(result);

    return status;
  }

protected:

  nav2_tasks::TaskStatus cycle(ResultMsg & result)
  {
    auto time_since_msg = std::chrono::system_clock::now();
    auto current_time = std::chrono::system_clock::now();

    nav2_tasks::TaskStatus status;

    while (rclcpp::ok()) {
      if (task_server_->cancelRequested()) {
        RCLCPP_INFO(get_logger(), "%s cancelled", taskName_.c_str());
        task_server_->setCanceled();
        return nav2_tasks::TaskStatus::CANCELED;
      }

      // Log a message every second
      current_time = std::chrono::system_clock::now();
      if (current_time - time_since_msg >= 1s) {
        RCLCPP_INFO(get_logger(), "running...");
        time_since_msg = std::chrono::system_clock::now();
      }

      status = onCycleUpdate(result);

      if (status == nav2_tasks::TaskStatus::SUCCEEDED) {
        RCLCPP_INFO(get_logger(), "Behavior completed successfully");
        break;
      }

      if (status == nav2_tasks::TaskStatus::FAILED) {
        RCLCPP_WARN(get_logger(), "Behavior was not completed");
        break;
      }

      if (status == nav2_tasks::TaskStatus::CANCELED) {
        RCLCPP_WARN(get_logger(), "onCycleUpdate() should not check for task cancellation,"
         " it will be checked by the base class.");
        break;
      }
    }

    current_time = std::chrono::system_clock::now();
    RCLCPP_INFO(get_logger(), "Behavior ran for %d seconds", current_time);

    return status;
  }

  std::shared_ptr<nav2_robot::Robot> robot_;

  typename std::unique_ptr<nav2_tasks::TaskServer<CommandMsg, ResultMsg>> task_server_;

  std::string taskName_;
};

}  // namespace nav2_behaviors

#endif  // NAV2_BEHAVIORS__BEHAVIOR_HPP_
