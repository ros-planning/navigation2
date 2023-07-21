// Copyright (c) 2022 Samsung R&D Institute Russia
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

#include <gtest/gtest.h>

#include <math.h>
#include <cmath>
#include <chrono>
#include <memory>
#include <utility>
#include <vector>
#include <string>
#include <limits>

#include "rclcpp/rclcpp.hpp"
#include "nav2_util/lifecycle_node.hpp"
#include "sensor_msgs/msg/laser_scan.hpp"
#include "sensor_msgs/msg/point_cloud2.hpp"
#include "sensor_msgs/msg/range.hpp"
#include "sensor_msgs/point_cloud2_iterator.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "geometry_msgs/msg/polygon_stamped.hpp"

#include "tf2_ros/transform_broadcaster.h"

#include "nav2_collision_monitor/types.hpp"
#include "nav2_collision_monitor/collision_detector_node.hpp"

using namespace std::chrono_literals;

static constexpr double EPSILON = 1e-5;

static const char BASE_FRAME_ID[]{"base_link"};
static const char SOURCE_FRAME_ID[]{"base_source"};
static const char ODOM_FRAME_ID[]{"odom"};
static const char FOOTPRINT_TOPIC[]{"footprint"};
static const char SCAN_NAME[]{"Scan"};
static const char POINTCLOUD_NAME[]{"PointCloud"};
static const char RANGE_NAME[]{"Range"};
static const char STATE_TOPIC[]{"collision_detector_state"};
static const int MAX_POINTS{1};
static const double SIMULATION_TIME_STEP{0.01};
static const double TRANSFORM_TOLERANCE{0.5};
static const double SOURCE_TIMEOUT{5.0};

enum PolygonType
{
  POLYGON_UNKNOWN = 0,
  POLYGON = 1,
  CIRCLE = 2
};

enum SourceType
{
  SOURCE_UNKNOWN = 0,
  SCAN = 1,
  POINTCLOUD = 2,
  RANGE = 3
};

class CollisionDetectorWrapper : public nav2_collision_monitor::CollisionDetector
{
public:
  void start()
  {
    ASSERT_EQ(on_configure(get_current_state()), nav2_util::CallbackReturn::SUCCESS);
    ASSERT_EQ(on_activate(get_current_state()), nav2_util::CallbackReturn::SUCCESS);
  }

  void stop()
  {
    ASSERT_EQ(on_deactivate(get_current_state()), nav2_util::CallbackReturn::SUCCESS);
    ASSERT_EQ(on_cleanup(get_current_state()), nav2_util::CallbackReturn::SUCCESS);
    ASSERT_EQ(on_shutdown(get_current_state()), nav2_util::CallbackReturn::SUCCESS);
  }

  void configure()
  {
    ASSERT_EQ(on_configure(get_current_state()), nav2_util::CallbackReturn::SUCCESS);
  }

  void cant_configure()
  {
    ASSERT_EQ(on_configure(get_current_state()), nav2_util::CallbackReturn::FAILURE);
  }

  bool correctDataReceived(const double expected_dist, const rclcpp::Time & stamp)
  {
    for (std::shared_ptr<nav2_collision_monitor::Source> source : sources_) {
      std::vector<nav2_collision_monitor::Point> collision_points;
      source->getData(stamp, collision_points);
      if (collision_points.size() != 0) {
        const double dist = std::hypot(collision_points[0].x, collision_points[0].y);
        if (std::fabs(dist - expected_dist) <= EPSILON) {
          return true;
        }
      }
    }
    return false;
  }
};  // CollisionDetectorWrapper

class Tester : public ::testing::Test
{
public:
  Tester();
  ~Tester();

  // Configuring
  void setCommonParameters();
  void addPolygon(
    const std::string & polygon_name, const PolygonType type,
    const double size, const std::string & at);
  void addSource(const std::string & source_name, const SourceType type);
  void setVectors(
    const std::vector<std::string> & polygons,
    const std::vector<std::string> & sources);

  // Setting TF chains
  void sendTransforms(const rclcpp::Time & stamp);

  // Publish robot footprint
  void publishFootprint(const double radius, const rclcpp::Time & stamp);

  // Main topic/data working routines
  void publishScan(const double dist, const rclcpp::Time & stamp);
  void publishPointCloud(const double dist, const rclcpp::Time & stamp);
  void publishRange(const double dist, const rclcpp::Time & stamp);
  bool waitData(
    const double expected_dist,
    const std::chrono::nanoseconds & timeout,
    const rclcpp::Time & stamp);
  bool waitState(const std::chrono::nanoseconds & timeout);
  void stateCallback(nav2_msgs::msg::CollisionDetectorState::SharedPtr msg);

protected:
  // CollisionMonitor node
  std::shared_ptr<CollisionDetectorWrapper> cd_;

  // Data source publishers
  rclcpp::Publisher<sensor_msgs::msg::LaserScan>::SharedPtr scan_pub_;
  rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr pointcloud_pub_;
  rclcpp::Publisher<sensor_msgs::msg::Range>::SharedPtr range_pub_;

  rclcpp::Subscription<nav2_msgs::msg::CollisionDetectorState>::SharedPtr state_sub_;
  nav2_msgs::msg::CollisionDetectorState::SharedPtr state_msg_;


};  // Tester

Tester::Tester()
{
  cd_ = std::make_shared<CollisionDetectorWrapper>();

  scan_pub_ = cd_->create_publisher<sensor_msgs::msg::LaserScan>(
    SCAN_NAME, rclcpp::QoS(rclcpp::KeepLast(1)).transient_local().reliable());
  pointcloud_pub_ = cd_->create_publisher<sensor_msgs::msg::PointCloud2>(
    POINTCLOUD_NAME, rclcpp::QoS(rclcpp::KeepLast(1)).transient_local().reliable());
  range_pub_ = cd_->create_publisher<sensor_msgs::msg::Range>(
    RANGE_NAME, rclcpp::QoS(rclcpp::KeepLast(1)).transient_local().reliable());

  state_sub_ = cd_->create_subscription<nav2_msgs::msg::CollisionDetectorState>(
    STATE_TOPIC, rclcpp::SystemDefaultsQoS(),
    std::bind(&Tester::stateCallback, this, std::placeholders::_1));
}

Tester::~Tester()
{
  scan_pub_.reset();
  pointcloud_pub_.reset();
  range_pub_.reset();

  cd_.reset();
}

bool Tester::waitState(const std::chrono::nanoseconds & timeout)
{
  rclcpp::Time start_time = cd_->now();
  while (rclcpp::ok() && cd_->now() - start_time <= rclcpp::Duration(timeout)) {
    if (state_msg_) {
      return true;
    }
    rclcpp::spin_some(cd_->get_node_base_interface());
    std::this_thread::sleep_for(10ms);
  }
  return false;
}

void Tester::stateCallback(nav2_msgs::msg::CollisionDetectorState::SharedPtr msg)
{
  state_msg_ = msg;
}


void Tester::setCommonParameters()
{
  cd_->declare_parameter(
    "base_frame_id", rclcpp::ParameterValue(BASE_FRAME_ID));
  cd_->set_parameter(
    rclcpp::Parameter("base_frame_id", BASE_FRAME_ID));
  cd_->declare_parameter(
    "odom_frame_id", rclcpp::ParameterValue(ODOM_FRAME_ID));
  cd_->set_parameter(
    rclcpp::Parameter("odom_frame_id", ODOM_FRAME_ID));

  cd_->declare_parameter(
    "transform_tolerance", rclcpp::ParameterValue(TRANSFORM_TOLERANCE));
  cd_->set_parameter(
    rclcpp::Parameter("transform_tolerance", TRANSFORM_TOLERANCE));
  cd_->declare_parameter(
    "source_timeout", rclcpp::ParameterValue(SOURCE_TIMEOUT));
  cd_->set_parameter(
    rclcpp::Parameter("source_timeout", SOURCE_TIMEOUT));
}

void Tester::addPolygon(
  const std::string & polygon_name, const PolygonType type,
  const double size, const std::string & at)
{
  if (type == POLYGON) {
    cd_->declare_parameter(
      polygon_name + ".type", rclcpp::ParameterValue("polygon"));
    cd_->set_parameter(
      rclcpp::Parameter(polygon_name + ".type", "polygon"));

    if (at != "approach") {
      const std::vector<double> points {
        size, size, size, -size, -size, -size, -size, size};
      cd_->declare_parameter(
        polygon_name + ".points", rclcpp::ParameterValue(points));
      cd_->set_parameter(
        rclcpp::Parameter(polygon_name + ".points", points));
    } else {  // at == "approach"
      cd_->declare_parameter(
        polygon_name + ".footprint_topic", rclcpp::ParameterValue(FOOTPRINT_TOPIC));
      cd_->set_parameter(
        rclcpp::Parameter(polygon_name + ".footprint_topic", FOOTPRINT_TOPIC));
    }
  } else if (type == CIRCLE) {
    cd_->declare_parameter(
      polygon_name + ".type", rclcpp::ParameterValue("circle"));
    cd_->set_parameter(
      rclcpp::Parameter(polygon_name + ".type", "circle"));

    cd_->declare_parameter(
      polygon_name + ".radius", rclcpp::ParameterValue(size));
    cd_->set_parameter(
      rclcpp::Parameter(polygon_name + ".radius", size));
  } else {  // type == POLYGON_UNKNOWN
    cd_->declare_parameter(
      polygon_name + ".type", rclcpp::ParameterValue("unknown"));
    cd_->set_parameter(
      rclcpp::Parameter(polygon_name + ".type", "unknown"));
  }

  cd_->declare_parameter(
    polygon_name + ".action_type", rclcpp::ParameterValue(at));
  cd_->set_parameter(
    rclcpp::Parameter(polygon_name + ".action_type", at));

  cd_->declare_parameter(
    polygon_name + ".max_points", rclcpp::ParameterValue(MAX_POINTS));
  cd_->set_parameter(
    rclcpp::Parameter(polygon_name + ".max_points", MAX_POINTS));

  cd_->declare_parameter(
    polygon_name + ".simulation_time_step", rclcpp::ParameterValue(SIMULATION_TIME_STEP));
  cd_->set_parameter(
    rclcpp::Parameter(polygon_name + ".simulation_time_step", SIMULATION_TIME_STEP));

  cd_->declare_parameter(
    polygon_name + ".visualize", rclcpp::ParameterValue(false));
  cd_->set_parameter(
    rclcpp::Parameter(polygon_name + ".visualize", false));

  cd_->declare_parameter(
    polygon_name + ".polygon_pub_topic", rclcpp::ParameterValue(polygon_name));
  cd_->set_parameter(
    rclcpp::Parameter(polygon_name + ".polygon_pub_topic", polygon_name));
}

void Tester::addSource(
  const std::string & source_name, const SourceType type)
{
  if (type == SCAN) {
    cd_->declare_parameter(
      source_name + ".type", rclcpp::ParameterValue("scan"));
    cd_->set_parameter(
      rclcpp::Parameter(source_name + ".type", "scan"));
  } else if (type == POINTCLOUD) {
    cd_->declare_parameter(
      source_name + ".type", rclcpp::ParameterValue("pointcloud"));
    cd_->set_parameter(
      rclcpp::Parameter(source_name + ".type", "pointcloud"));

    cd_->declare_parameter(
      source_name + ".min_height", rclcpp::ParameterValue(0.1));
    cd_->set_parameter(
      rclcpp::Parameter(source_name + ".min_height", 0.1));
    cd_->declare_parameter(
      source_name + ".max_height", rclcpp::ParameterValue(1.0));
    cd_->set_parameter(
      rclcpp::Parameter(source_name + ".max_height", 1.0));
  } else if (type == RANGE) {
    cd_->declare_parameter(
      source_name + ".type", rclcpp::ParameterValue("range"));
    cd_->set_parameter(
      rclcpp::Parameter(source_name + ".type", "range"));

    cd_->declare_parameter(
      source_name + ".obstacles_angle", rclcpp::ParameterValue(M_PI / 200));
    cd_->set_parameter(
      rclcpp::Parameter(source_name + ".obstacles_angle", M_PI / 200));
  } else {  // type == SOURCE_UNKNOWN
    cd_->declare_parameter(
      source_name + ".type", rclcpp::ParameterValue("unknown"));
    cd_->set_parameter(
      rclcpp::Parameter(source_name + ".type", "unknown"));
  }

  cd_->declare_parameter(
    source_name + ".topic", rclcpp::ParameterValue(source_name));
  cd_->set_parameter(
    rclcpp::Parameter(source_name + ".topic", source_name));
}

void Tester::setVectors(
  const std::vector<std::string> & polygons,
  const std::vector<std::string> & sources)
{
  cd_->declare_parameter("polygons", rclcpp::ParameterValue(polygons));
  cd_->set_parameter(rclcpp::Parameter("polygons", polygons));

  cd_->declare_parameter("observation_sources", rclcpp::ParameterValue(sources));
  cd_->set_parameter(rclcpp::Parameter("observation_sources", sources));
}

void Tester::sendTransforms(const rclcpp::Time & stamp)
{
  std::shared_ptr<tf2_ros::TransformBroadcaster> tf_broadcaster =
    std::make_shared<tf2_ros::TransformBroadcaster>(cd_);

  geometry_msgs::msg::TransformStamped transform;
  transform.transform.rotation.x = 0.0;
  transform.transform.rotation.y = 0.0;
  transform.transform.rotation.z = 0.0;
  transform.transform.rotation.w = 1.0;

  // Fill TF buffer ahead for future transform usage in CollisionMonitor::process()
  const rclcpp::Duration ahead = 1000ms;
  for (rclcpp::Time t = stamp; t <= stamp + ahead; t += rclcpp::Duration(50ms)) {
    transform.header.stamp = t;

    // base_frame -> source_frame transform
    transform.header.frame_id = BASE_FRAME_ID;
    transform.child_frame_id = SOURCE_FRAME_ID;
    tf_broadcaster->sendTransform(transform);

    // odom_frame -> base_frame transform
    transform.header.frame_id = ODOM_FRAME_ID;
    transform.child_frame_id = BASE_FRAME_ID;
    tf_broadcaster->sendTransform(transform);
  }
}

void Tester::publishScan(const double dist, const rclcpp::Time & stamp)
{
  std::unique_ptr<sensor_msgs::msg::LaserScan> msg =
    std::make_unique<sensor_msgs::msg::LaserScan>();

  msg->header.frame_id = SOURCE_FRAME_ID;
  msg->header.stamp = stamp;

  msg->angle_min = 0.0;
  msg->angle_max = 2 * M_PI;
  msg->angle_increment = M_PI / 180;
  msg->time_increment = 0.0;
  msg->scan_time = 0.0;
  msg->range_min = 0.1;
  msg->range_max = dist + 1.0;
  std::vector<float> ranges(360, dist);
  msg->ranges = ranges;

  scan_pub_->publish(std::move(msg));
}

void Tester::publishPointCloud(const double dist, const rclcpp::Time & stamp)
{
  std::unique_ptr<sensor_msgs::msg::PointCloud2> msg =
    std::make_unique<sensor_msgs::msg::PointCloud2>();
  sensor_msgs::PointCloud2Modifier modifier(*msg);

  msg->header.frame_id = SOURCE_FRAME_ID;
  msg->header.stamp = stamp;

  modifier.setPointCloud2Fields(
    3, "x", 1, sensor_msgs::msg::PointField::FLOAT32,
    "y", 1, sensor_msgs::msg::PointField::FLOAT32,
    "z", 1, sensor_msgs::msg::PointField::FLOAT32);
  modifier.resize(2);

  sensor_msgs::PointCloud2Iterator<float> iter_x(*msg, "x");
  sensor_msgs::PointCloud2Iterator<float> iter_y(*msg, "y");
  sensor_msgs::PointCloud2Iterator<float> iter_z(*msg, "z");

  // Point 0: (dist, 0.01, 0.2)
  *iter_x = dist;
  *iter_y = 0.01;
  *iter_z = 0.2;
  ++iter_x; ++iter_y; ++iter_z;

  // Point 1: (dist, -0.01, 0.2)
  *iter_x = dist;
  *iter_y = -0.01;
  *iter_z = 0.2;

  pointcloud_pub_->publish(std::move(msg));
}

void Tester::publishRange(const double dist, const rclcpp::Time & stamp)
{
  std::unique_ptr<sensor_msgs::msg::Range> msg =
    std::make_unique<sensor_msgs::msg::Range>();

  msg->header.frame_id = SOURCE_FRAME_ID;
  msg->header.stamp = stamp;

  msg->radiation_type = 0;
  msg->field_of_view = M_PI / 10;
  msg->min_range = 0.1;
  msg->max_range = dist + 0.1;
  msg->range = dist;

  range_pub_->publish(std::move(msg));
}

bool Tester::waitData(
  const double expected_dist,
  const std::chrono::nanoseconds & timeout,
  const rclcpp::Time & stamp)
{
  rclcpp::Time start_time = cd_->now();
  while (rclcpp::ok() && cd_->now() - start_time <= rclcpp::Duration(timeout)) {
    if (cd_->correctDataReceived(expected_dist, stamp)) {
      return true;
    }
    rclcpp::spin_some(cd_->get_node_base_interface());
    std::this_thread::sleep_for(10ms);
  }
  return false;
}


TEST_F(Tester, testIncorrectPolygonType)
{
  setCommonParameters();
  addPolygon("UnknownShape", POLYGON_UNKNOWN, 1.0, "none");
  addSource(SCAN_NAME, SCAN);
  setVectors({"UnknownShape"}, {SCAN_NAME});

  // Check that Collision Monitor node can not be configured for this parameters set
  cd_->cant_configure();
}

TEST_F(Tester, testIncorrectSourceType)
{
  setCommonParameters();
  addPolygon("DetectionRegion", POLYGON, 1.0, "none");
  addSource("UnknownSource", SOURCE_UNKNOWN);
  setVectors({"DetectionRegion"}, {"UnknownSource"});

  // Check that Collision Monitor node can not be configured for this parameters set
  cd_->cant_configure();
}

TEST_F(Tester, testPolygonsNotSet)
{
  setCommonParameters();
  addPolygon("DetectionRegion", POLYGON, 1.0, "none");
  addSource(SCAN_NAME, SCAN);

  // Check that Collision Monitor node can not be configured for this parameters set
  cd_->cant_configure();
}

TEST_F(Tester, testSourcesNotSet)
{
  setCommonParameters();
  addPolygon("DetectionRegion", POLYGON, 1.0, "none");
  addSource(SCAN_NAME, SCAN);
  cd_->declare_parameter("polygons", rclcpp::ParameterValue(std::vector<std::string>{"DetectionRegion"}));
  cd_->set_parameter(rclcpp::Parameter("polygons", std::vector<std::string>{"DetectionRegion"}));

  // Check that Collision Monitor node can not be configured for this parameters set
  cd_->cant_configure();
}

TEST_F(Tester, testSuccessfulConfigure)
{
  setCommonParameters();
  addPolygon("DetectionRegion", POLYGON, 1.0, "none");
  addSource(SCAN_NAME, SCAN);
  setVectors({"DetectionRegion"}, {SCAN_NAME});

  // Check that Collision Monitor node can not be configured for this parameters set
  cd_->configure();
}

TEST_F(Tester, testProcessNonActive)
{
  rclcpp::Time curr_time = cd_->now();

  setCommonParameters();
  addPolygon("DetectionRegion", POLYGON, 1.0, "none");
  addSource(SCAN_NAME, SCAN);
  setVectors({"DetectionRegion"}, {SCAN_NAME});

  // Configure Collision Detector node, but not activate
  cd_->configure();

  // ... and check that the detector state was not published
  ASSERT_FALSE(waitState(1000ms));

  // Stop Collision Monitor node
  cd_->stop();
}

TEST_F(Tester, testProcessActive)
{
  rclcpp::Time curr_time = cd_->now();

  setCommonParameters();
  addPolygon("DetectionRegion", POLYGON, 1.0, "none");
  addSource(SCAN_NAME, SCAN);
  setVectors({"DetectionRegion"}, {SCAN_NAME});

  // Configure Collision Detector node, but not activate
  cd_->start();
  // ... and check that state is published
  ASSERT_TRUE(waitState(1000ms));

  // Stop Collision Monitor node
  cd_->stop();
}

TEST_F(Tester, testdetection)
{
  rclcpp::Time curr_time = cd_->now();

  // Set Collision Monitor parameters.
  // Making two polygons: outer polygon for slowdown and inner for robot stop.
  setCommonParameters();
  addPolygon("DetectionRegion", POLYGON, 2.0, "none");
  addSource(SCAN_NAME, SCAN);
  setVectors({"DetectionRegion"}, {SCAN_NAME});

  // Start Collision Monitor node
  cd_->start();

  // Share TF
  sendTransforms(curr_time);
  
  // Obstacle is in slowdown robot zone
  publishScan(1.5, curr_time);
  
  ASSERT_TRUE(waitData(1.5, 500ms, curr_time));
  ASSERT_TRUE(waitState(1000ms));
  ASSERT_EQ(state_msg_->detections[0], true);

  // Stop Collision Monitor node
  cd_->stop();
}

int main(int argc, char ** argv)
{
  // Initialize the system
  testing::InitGoogleTest(&argc, argv);
  rclcpp::init(argc, argv);

  // Actual testing
  bool test_result = RUN_ALL_TESTS();

  // Shutdown
  rclcpp::shutdown();

  return test_result;
}