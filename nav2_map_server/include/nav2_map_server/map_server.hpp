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

#ifndef NAV2_MAP_SERVER__MAP_SERVER_HPP_
#define NAV2_MAP_SERVER__MAP_SERVER_HPP_

#include <string>
#include <memory>
#include "rclcpp/rclcpp.hpp"
#include "nav2_map_server/map_loader.hpp"
#include "nav_msgs/msg/occupancy_grid.hpp"
#include "nav_msgs/msg/map_meta_data.hpp"
#include "nav_msgs/srv/get_map.hpp"
#include "nav2_map_server/map_loader.hpp"

namespace nav2_map_server
{

class MapServer : public rclcpp::Node
{
public:
  explicit MapServer(const std::string & name);
  MapServer() = delete;

private:
  std::unique_ptr<MapLoader> createMapLoader();
  void initServices();

  std::shared_ptr<MapLoader> map_loader_;

  // A service to provide the ouccpancy grid (GetMap) and the message to return
  rclcpp::Service<nav_msgs::srv::GetMap>::SharedPtr occ_service_;

  // A topic on which the occupancy grid will be published
  rclcpp::Publisher<nav_msgs::msg::OccupancyGrid>::SharedPtr occ_pub_;

  // For now, publish the map periodically so that it is sure to be received on the
  // ROS1 side across the ROS1 bridge
  rclcpp::TimerBase::SharedPtr timer_;

  // The message to publish on the occupancy grid topic
  nav_msgs::msg::OccupancyGrid map_msg_;
};

}  // namespace nav2_map_server

#endif  // NAV2_MAP_SERVER__MAP_SERVER_HPP_
