// Copyright (c) 2020 Samsung R&D Institute Russia
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

/* OccupancyGrid map input-output library */

#ifndef NAV2_MAP_SERVER__MAPIO_HPP_
#define NAV2_MAP_SERVER__MAPIO_HPP_

#include <string>
#include <vector>

#include "nav2_map_server/map_mode.hpp"
#include "nav_msgs/msg/occupancy_grid.hpp"

/* Map input part */

namespace nav2_map_server
{

struct LoadParameters
{
  std::string image_file_name;
  double resolution{0};
  std::vector<double> origin{0, 0, 0};
  double free_thresh;
  double occupied_thresh;
  MapMode mode;
  bool negate;
};

/**
 * @brief Load and parse the given YAML file
 * @param yaml_filename Name of the map file passed though parameter
 * @return Map loading parameters obtained from YAML file
 * @throw YAML::Exception
 */
LoadParameters loadMapYaml(const std::string & yaml_filename);

/**
 * @brief Load the image from map file and generate an OccupancyGrid
 * @param load_parameters Parameters of loading map
 * @param map Output loaded map
 * @throw std::exception
 */
void loadMapFromFile(
  const LoadParameters & load_parameters,
  nav_msgs::msg::OccupancyGrid & map);

/**
 * @brief Load the map YAML, image from map file and
 * generate an OccupancyGrid
 * @param yaml_file Name of input YAML file
 * @param map Output loaded map
 * @return true or false
 */
bool loadMapFromYaml(
  const std::string & yaml_file,
  nav_msgs::msg::OccupancyGrid & map);


/* Map output part */

struct SaveParameters
{
  std::string map_file_name{""};
  std::string image_format{""};
  int free_thresh{0};
  int occupied_thresh{0};
  MapMode mode{MapMode::Trinary};
};

/**
 * @brief Write OccupancyGrid map to file
 * @param map OccupancyGrid map data
 * @param save_parameters Map saving parameters.
 * NOTE: save_parameters could be updated during function execution.
 * @return true or false
 */
bool saveMapToFile(
  const nav_msgs::msg::OccupancyGrid & map,
  SaveParameters & save_parameters);

}  // namespace nav2_map_server

#endif  // NAV2_MAP_SERVER__MAPIO_HPP_
