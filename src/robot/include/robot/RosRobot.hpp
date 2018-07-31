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

#ifndef ROBOT__ROSROBOT_HPP_
#define ROBOT__ROSROBOT_HPP_

#include <string>
#include "robot/Robot.hpp"

class RosRobot : public Robot
{
public:
  /**
   * Construct a RosRobot with a provided URDF file.
   *
   * @param[in] filename The filename of the URDF file describing this robot.
   */
  explicit RosRobot(const std::string & urdf_filename);
  RosRobot() = delete;
  ~RosRobot();

  void enterSafeState() override;
};

#endif  // ROBOT__ROSROBOT_HPP_
