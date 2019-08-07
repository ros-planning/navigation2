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

#ifndef NAV2_UTIL__GEOMETRY_UTILS_HPP_
#define NAV2_UTIL__GEOMETRY_UTILS_HPP_

#include "geometry_msgs/msg/quaternion.hpp"

namespace nav2_util
{

class geometry_utils
{
public:
  static geometry_msgs::msg::Quaternion orientationAroundZAxis(double angle);
};

}  // namespace nav2_util

#endif  // NAV2_UTIL__GEOMETRY_UTILS_HPP_
