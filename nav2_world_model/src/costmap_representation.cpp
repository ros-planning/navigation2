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

#include "nav2_world_model/costmap_representation.hpp"
#include "nav2_costmap_2d/cost_values.hpp"

namespace nav2_world_model
{

using nav2_costmap_2d::MapLocation;

CostmapRepresentation::CostmapRepresentation(
  const std::string name,
  rclcpp::Node::SharedPtr & node,
  rclcpp::executor::Executor & executor,
  rclcpp::Clock::SharedPtr & clock)
: WorldRepresentation(name, node),
  clock_(clock),
  tfBuffer_(clock_),
  tfListener_(tfBuffer_)
{
  costmap_ros_ = std::make_shared<nav2_costmap_2d::Costmap2DROS>(name_, tfBuffer_);
  costmap_ = costmap_ros_->getCostmap();
  executor.add_node(costmap_ros_);

  marker_publisher_ = node_->create_publisher<visualization_msgs::msg::Marker>(
    "world_model_cell", 1);
}

GetCostmap::Response
CostmapRepresentation::getCostmap(const GetCostmap::Request & /*request*/)
{
  GetCostmap::Response response;
  response.map.metadata.size_x = costmap_->getSizeInCellsX();
  response.map.metadata.size_y = costmap_->getSizeInCellsY();
  response.map.metadata.resolution = costmap_->getResolution();
  response.map.metadata.layer = "Master";
  response.map.metadata.map_load_time = costmap_ros_->now();
  response.map.metadata.update_time = costmap_ros_->now();

  tf2::Quaternion quaternion;
  // TODO(bpwilcox): Grab correct orientation information
  quaternion.setRPY(0.0, 0.0, 0.0);  // set roll, pitch, yaw
  response.map.metadata.origin.position.x = costmap_->getOriginX();
  response.map.metadata.origin.position.y = costmap_->getOriginY();
  response.map.metadata.origin.position.z = 0.0;
  response.map.metadata.origin.orientation = tf2::toMsg(quaternion);

  response.map.header.stamp = costmap_ros_->now();
  response.map.header.frame_id = "map";

  unsigned char * data = costmap_->getCharMap();
  auto data_length = response.map.metadata.size_x * response.map.metadata.size_y;
  response.map.data.resize(data_length);
  response.map.data.assign(data, data + data_length);

  return response;
}

ProcessRegion::Response
CostmapRepresentation::confirmFreeSpace(const ProcessRegion::Request & request)
{
  ProcessRegion::Response response;
  response.was_successful = checkIfFree(request);
  return response;
}

ProcessRegion::Response
CostmapRepresentation::clearArea(const ProcessRegion::Request & /*request*/)
{
  // TODO(orduno)
  ProcessRegion::Response response;
  response.was_successful = false;
  return response;
}

bool CostmapRepresentation::checkIfFree(const ProcessRegion::Request & request) const
{
  std::vector<MapLocation> polygon_cells;

  // Get all the cell locations inside the region
  // costmap_->convexFillCells(generateRectangleVertices(request), polygon_cells);

  // TODO(orduno) Alternatively we could only check the outline
  costmap_->polygonOutlineCells(generateRectangleVertices(request), polygon_cells);

  // Check if there's at least one cell not free
  for (const auto & cell : polygon_cells) {
    if (!isFree(cell)) {
      return false;
    }
  }

  return true;
}

std::vector<MapLocation> CostmapRepresentation::generateRectangleVertices(
  const ProcessRegion::Request & request) const
{
  // TODO(orduno) rotate vertices

  double center_x = request.center_location.x;
  double center_y = request.center_location.y;
  double width = request.width;
  double height = request.height;

  double top = center_y + height / 2;
  double down = center_y - height / 2;
  double right = center_x + width / 2;
  double left = center_x - width / 2;

  std::vector<MapLocation> vertices;
  unsigned int mx, my;

  // add the bottom left vertex
  costmap_->worldToMap(left, down, mx, my);
  vertices.push_back(MapLocation{mx, my});

  // add the top left vertex
  costmap_->worldToMap(left, top, mx, my);
  vertices.push_back(MapLocation{mx, my});

  // add the top right vertex
  costmap_->worldToMap(right, top, mx, my);
  vertices.push_back(MapLocation{mx, my});

  // add the bottom right vertex
  costmap_->worldToMap(right, down, mx, my);
  vertices.push_back(MapLocation{mx, my});

  return vertices;
}

bool CostmapRepresentation::isFree(const MapLocation & location) const
{
  bool isFree = (costmap_->getCost(location.x, location.y)
    < nav2_costmap_2d::INSCRIBED_INFLATED_OBSTACLE);

  std_msgs::msg::ColorRGBA color;
  color.r = 0.0;
  color.g = 0.0;
  color.b = 0.0;
  color.a = 0.5;

  isFree ? color.g = 1.0 : color.r = 1.0;

  double wx, wy;
  costmap_->mapToWorld(location.x, location.y, wx, wy);
  publishMarker(wx, wy, color);

  return isFree;
}

void CostmapRepresentation::publishMarker(
  const double wx, const double wy, const std_msgs::msg::ColorRGBA & color) const
{
  visualization_msgs::msg::Marker marker;
  marker.header.frame_id = "map";
  // marker.header.stamp = node_.now();

  // Set the namespace and id for this marker.  This serves to create a unique ID
  // Any marker sent with the same namespace and id will overwrite the old one
  marker.ns = "world_model_cell";

  static int index = 0;
  marker.id = index++;

  marker.type = visualization_msgs::msg::Marker::CUBE;
  marker.action = visualization_msgs::msg::Marker::ADD;

  marker.pose.position.x = wx;
  marker.pose.position.y = wy;
  marker.pose.position.z = 0.0;

  marker.pose.orientation.x = 0.0;
  marker.pose.orientation.y = 0.0;
  marker.pose.orientation.z = 0.0;
  marker.pose.orientation.w = 1.0;

  // Set the scale of the marker -- 1x1x1 here means 1m on a side
  marker.scale.x = costmap_->getResolution();
  marker.scale.y = costmap_->getResolution();
  marker.scale.z = costmap_->getResolution();

  marker.color = color;

  // Duration of zero indicates the object should last forever
  builtin_interfaces::msg::Duration duration;
  duration.sec = 1.5;
  duration.nanosec = 0;
  marker.lifetime = duration;

  // TODO(orduno) this is necessary?
  marker.frame_locked = false;

  marker_publisher_->publish(marker);
}

}  // namespace nav2_world_model
