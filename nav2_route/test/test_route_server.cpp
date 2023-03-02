// Copyright (c) 2023, Samsung Research America
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

#include <math.h>
#include <memory>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "rclcpp/rclcpp.hpp"
#include "tf2_ros/transform_broadcaster.h"
#include "tf2_ros/create_timer_ros.h"
#include "tf2_ros/transform_listener.h"
#include "nav2_util/lifecycle_node.hpp"
#include "nav2_util/service_client.hpp"
#include "nav2_core/route_exceptions.hpp"
#include "nav2_route/route_tracker.hpp"
#include "nav2_route/route_server.hpp"
#include "ament_index_cpp/get_package_share_directory.hpp"

class RclCppFixture
{
public:
  RclCppFixture() {rclcpp::init(0, nullptr);}
  ~RclCppFixture() {rclcpp::shutdown();}
};
RclCppFixture g_rclcppfixture;

using namespace nav2_route;  // NOLINT

class RouteServerWrapper : public RouteServer
{
public:
  explicit RouteServerWrapper(const rclcpp::NodeOptions & options)
  : RouteServer(options)
  {}

  void lifecycleCycle()
  {
    on_configure(rclcpp_lifecycle::State());
    on_activate(rclcpp_lifecycle::State());
    on_deactivate(rclcpp_lifecycle::State());
    on_cleanup(rclcpp_lifecycle::State());
    on_shutdown(rclcpp_lifecycle::State());
  }

  void startup()
  {
    on_configure(rclcpp_lifecycle::State());
    on_activate(rclcpp_lifecycle::State());
  }

  void configure()
  {
    on_configure(rclcpp_lifecycle::State());
  }

  void activate()
  {
    on_activate(rclcpp_lifecycle::State());
  }

  void shutdown()
  {
    on_deactivate(rclcpp_lifecycle::State());
    on_cleanup(rclcpp_lifecycle::State());
  }

  void testPrint(
    const std::shared_ptr<const nav2_msgs::action::ComputeRoute::Goal> goal,
    const std::exception & ex)
  {
    exceptionWarning(goal, ex);
  }

  rclcpp::Duration findPlanningDurationWrapper(const rclcpp::Time & start)
  {
    return findPlanningDuration(start);
  }

  void populateActionResultWrapper(
    std::shared_ptr<nav2_msgs::action::ComputeRoute::Result> result,
    const Route & route,
    const nav_msgs::msg::Path & path,
    const rclcpp::Duration & planning_duration)
  {
    return populateActionResult(result, route, path, planning_duration);
  }

  bool isRequestValidWrapper()
  {
    return isRequestValid(compute_and_track_route_server_);
  }
};

TEST(RouteServerTest, test_lifecycle)
{
  rclcpp::NodeOptions options;
  auto server = std::make_shared<RouteServerWrapper>(options);
  server->lifecycleCycle();
  server.reset();
}

TEST(RouteServerTest, test_set_srv)
{
  std::string pkg_share_dir = ament_index_cpp::get_package_share_directory("nav2_route");
  std::string real_filepath = pkg_share_dir + "/graphs/aws_graph.geojson";

  rclcpp::NodeOptions options;
  auto server = std::make_shared<RouteServerWrapper>(options);
  server->declare_parameter("graph_filepath", rclcpp::ParameterValue(real_filepath));
  auto node_thread = std::make_unique<nav2_util::NodeThread>(server);
  auto node2 = std::make_shared<rclcpp::Node>("my_node2");

  server->startup();
  auto srv_client =
    nav2_util::ServiceClient<nav2_msgs::srv::SetRouteGraph>(
    "set_route_graph", node2);
  auto req = std::make_shared<nav2_msgs::srv::SetRouteGraph::Request>();
  req->graph_filepath = "non/existent/path.json";
  auto resp = srv_client.invoke(req, std::chrono::nanoseconds(1000000000));
  EXPECT_FALSE(resp->success);

  auto req2 = std::make_shared<nav2_msgs::srv::SetRouteGraph::Request>();
  req2->graph_filepath = real_filepath;
  auto resp2 = srv_client.invoke(req2, std::chrono::nanoseconds(1000000000));
  EXPECT_TRUE(resp2->success);

  server->shutdown();
  node_thread.reset();
  server.reset();
}

inline Route getRoute()
{
  static Node node1, node2, node3;
  node1.nodeid = 1;
  node1.coords.x = 0.0;
  node1.coords.y = 0.0;
  node2.nodeid = 2;
  node2.coords.x = 10.0;
  node2.coords.y = 0.0;
  node3.nodeid = 3;
  node3.coords.x = 20.0;
  node3.coords.y = 0.0;

  static DirectionalEdge edge1, edge2;
  edge1.edgeid = 5;
  edge1.start = &node1;
  edge1.end = &node2;
  edge2.edgeid = 6;
  edge2.start = &node2;
  edge2.end = &node3;

  Route route;
  route.start_node = &node1;
  route.edges.push_back(&edge1);
  route.edges.push_back(&edge2);
  return route;
}

TEST(RouteServerTest, test_minor_utils)
{
  rclcpp::NodeOptions options;
  auto server = std::make_shared<RouteServerWrapper>(options);

  // Find planning duration should print and provide duration
  rclcpp::Time start(1000, 0, RCL_ROS_TIME);
  auto dur = server->findPlanningDurationWrapper(start);
  EXPECT_TRUE(dur.seconds() > 1e2);

  // This should print the goal info regarding error
  nav2_msgs::action::ComputeRoute::Goal goal_raw;
  auto goal = std::make_shared<const nav2_msgs::action::ComputeRoute::Goal>(goal_raw);
  std::runtime_error ex("Hi:-)");
  server->testPrint(goal, ex);

  // Populate the result message with content
  auto result = std::make_shared<nav2_msgs::action::ComputeRoute::Result>();
  Route route = getRoute();
  nav_msgs::msg::Path path;
  path.poses.resize(406);
  server->populateActionResultWrapper(result, route, path, dur);
  EXPECT_EQ(result->path.poses.size(), path.poses.size());
  EXPECT_EQ(result->route.edge_ids.size(), route.edges.size());

  server.reset();
}

TEST(RouteServerTest, test_request_valid)
{
  rclcpp::NodeOptions options;
  auto server = std::make_shared<RouteServerWrapper>(options);

  // Should be nullptr
  EXPECT_FALSE(server->isRequestValidWrapper());

  // Should be inactive
  server->configure();
  EXPECT_FALSE(server->isRequestValidWrapper());

  // Should be active
  server->activate();
  EXPECT_TRUE(server->isRequestValidWrapper());

  server.reset();
}

TEST(RouteServerTest, test_complete_action_api)
{
  std::string pkg_share_dir = ament_index_cpp::get_package_share_directory("nav2_route");
  std::string real_file = pkg_share_dir + "/graphs/aws_graph.geojson";

  rclcpp::NodeOptions options;
  auto server = std::make_shared<RouteServerWrapper>(options);
  server->declare_parameter("graph_filepath", rclcpp::ParameterValue(real_file));
  auto node_thread = std::make_unique<nav2_util::NodeThread>(server);
  server->startup();

  // Compute a simple route action request
  auto node2 = std::make_shared<rclcpp::Node>("my_node2");
  auto compute_client =
    rclcpp_action::create_client<nav2_msgs::action::ComputeRoute>(node2, "compute_route");

  nav2_msgs::action::ComputeRoute::Goal goal;
  goal.start_id = 1u;
  goal.goal_id = 1u;
  goal.use_poses = false;
  auto future_goal = compute_client->async_send_goal(goal);

  rclcpp::spin_until_future_complete(node2, future_goal);
  auto goal_handle = future_goal.get();
  auto result_future = compute_client->async_get_result(goal_handle);
  rclcpp::spin_until_future_complete(node2, result_future);
  auto result = result_future.get().result;
  EXPECT_EQ(result->route.edge_ids.size(), 0u);
  EXPECT_NEAR(result->route.route_cost, 0.0, 1e-6);
  EXPECT_EQ(result->route.nodes[0].nodeid, 1u);

  // Compute a route and tracking request
  auto track_client =
    rclcpp_action::create_client<nav2_msgs::action::ComputeAndTrackRoute>(
    node2, "compute_and_track_route");

  nav2_msgs::action::ComputeAndTrackRoute::Goal goal2;
  goal2.start_id = 1u;
  goal2.goal_id = 2u;
  goal2.use_poses = false;
  auto future_goal2 = track_client->async_send_goal(goal2);

  rclcpp::spin_until_future_complete(node2, future_goal2);
  auto goal_handle2 = future_goal2.get();

  // Preempt it
  rclcpp::Rate r(1.0);
  r.sleep();
  auto future_goal3 = track_client->async_send_goal(goal2);
  rclcpp::spin_until_future_complete(node2, future_goal3);
  auto goal_handle3 = future_goal3.get();

  // Cancel them all
  track_client->async_cancel_all_goals();

  auto result_future3 = track_client->async_get_result(goal_handle3);
  rclcpp::spin_until_future_complete(node2, result_future3);
  auto code = result_future3.get().code;
  EXPECT_EQ(code, rclcpp_action::ResultCode::CANCELED);

  // Make sure it still shuts down completely after real work
  server->shutdown();
  node_thread.reset();
  server.reset();
}
