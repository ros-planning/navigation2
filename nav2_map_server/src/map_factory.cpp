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

#include "nav2_map_server/map_factory.hpp"

#include <string>
#include <memory>
#include "nav2_map_server/occ_grid_server.hpp"

namespace nav2_map_server
{

std::shared_ptr<MapServer> MapFactory::createMap(rclcpp::Node::SharedPtr & node)
{
  // Get the required parameters: first the YAML filename
  auto yaml_filename_param = rclcpp::Parameter("map_type", std::string(""));
  node->get_parameter("yaml_filename", yaml_filename_param);
  std::string yaml_filename = yaml_filename_param.as_string();

  // Then the map type
  auto map_type_param = rclcpp::Parameter("map_type", std::string("occupancy"));
  node->get_parameter("map_type", map_type_param);
  std::string map_type = map_type_param.as_string();

  // Create the specified type of map server
  if (map_type == "occupancy") {
    return std::make_shared<OccGridServer>(node, yaml_filename);
  }

  RCLCPP_ERROR(node->get_logger(), "Cannot load map %s of type %s",
    yaml_filename.c_str(), map_type.c_str());
  throw std::runtime_error("Map type not supported");
}

}  // namespace nav2_map_server
