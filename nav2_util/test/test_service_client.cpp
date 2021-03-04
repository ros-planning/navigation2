// Copyright (c) 2019 Intel Corporation
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

#include <memory>
#include <string>
#include "nav2_util/service_client.hpp"
#include "rclcpp/rclcpp.hpp"
#include "std_srvs/srv/empty.hpp"
#include "std_msgs/msg/empty.hpp"
#include "gtest/gtest.h"

using nav2_util::ServiceClient;
using std::string;

class RclCppFixture
{
public:
  RclCppFixture() {rclcpp::init(0, nullptr);}
  ~RclCppFixture() {rclcpp::shutdown();}
};
RclCppFixture g_rclcppfixture;

class TestServiceClient : public ServiceClient<std_srvs::srv::Empty>
{
public:
  TestServiceClient(
    const std::string & name,
    const rclcpp::Node::SharedPtr & provided_node = rclcpp::Node::SharedPtr())
  : ServiceClient(name, provided_node) {}

  string name() {return node_->get_name();}
  const rclcpp::Node::SharedPtr & getNode() {return node_;}
};

TEST(ServiceClient, can_ServiceClient_use_passed_in_node)
{
  auto node = rclcpp::Node::make_shared("test_node");
  TestServiceClient t("bar", node);
  ASSERT_EQ(t.getNode(), node);
  ASSERT_EQ(t.name(), "test_node");
}

TEST(ServiceClient, can_ServiceClient_invoke_in_callback)
{
  int a = 0;
  auto service_node = rclcpp::Node::make_shared("service_node");
  service_node->create_service<std_srvs::srv::Empty>(
    "empty_srv",
    [&a](std_srvs::srv::Empty::Request::SharedPtr, std_srvs::srv::Empty::Response::SharedPtr) {
      a = 1;
    });
  auto srv_thread = std::thread([&]() {rclcpp::spin(service_node);});

  auto pub_node = rclcpp::Node::make_shared("pub_node");
  auto pub = pub_node->create_publisher<std_msgs::msg::Empty>(
    "empty_topic",
    rclcpp::QoS(10).transient_local());
  pub->publish(std_msgs::msg::Empty());
  rclcpp::spin_some(pub_node);

  auto node = rclcpp::Node::make_shared("test_node");
  TestServiceClient t("empty_srv", node);
  node->create_subscription<std_msgs::msg::Empty>(
    "empty_topic",
    rclcpp::QoS(10),
    [&t](std_msgs::msg::Empty::SharedPtr) {
      auto req = std::make_shared<std_srvs::srv::Empty::Request>();
      auto res = t.invoke(req);
    });
  rclcpp::spin_some(node);

  ASSERT_EQ(a, 1);
}
