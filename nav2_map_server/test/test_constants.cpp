/*
 * Copyright (c) 2008, Willow Garage, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Willow Garage, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* Author: Brian Gerkey */

/* This file contains global constants shared among tests */

/* Note that these must be changed if the test image changes */

#include "test_constants/test_constants.h"

#include <vector>

const unsigned int g_valid_image_width = 10;
const unsigned int g_valid_image_height = 10;
// Note that the image content is given in row-major order, with the
// lower-left pixel first.  This is different from a graphics coordinate
// system, which starts with the upper-left pixel.  The loadMapFromFile
// call converts from the latter to the former when it loads the image, and
// we want to compare against the result of that conversion.
const char g_valid_image_content[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  100, 100, 100, 100, 0, 0, 100, 100, 100, 0,
  100, 100, 100, 100, 0, 0, 100, 100, 100, 0,
  100, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  100, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  100, 0, 0, 0, 0, 0, 100, 100, 0, 0,
  100, 0, 0, 0, 0, 0, 100, 100, 0, 0,
  100, 0, 0, 0, 0, 0, 100, 100, 0, 0,
  100, 0, 0, 0, 0, 0, 100, 100, 0, 0,
  100, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

const char * g_valid_map_name = "testmap";
const char * g_valid_png_file = "testmap.png";
const char * g_valid_bmp_file = "testmap.bmp";
const char * g_valid_pgm_file = "testmap.pgm";
const char * g_valid_yaml_file = "testmap.yaml";
const char * g_tmp_dir = "/tmp";
const char * g_valid_pcd_map_name = "testpcd";
const char * g_valid_pcd_file = "testcloud.pcd";
const char * g_valid_pcd_yaml_file = "testpcd.yaml";

const double g_valid_image_res = 0.1;
const size_t g_valid_pcd_width = 2;
const size_t g_valid_pcd_data_size = 32;  // xyz-rgb : (32[float]/8[unit8])*2[rows]*4[elements]
const std::vector<double> g_valid_origin{2.0, 3.0, 1.0};

const std::vector<std::vector<double>> g_valid_pcd_content{
  {1.0, 2.0, 3.0, 0},
  {4.0, 5.0, 6.0, 0}
};

const double g_default_free_thresh = 0.196;
const double g_default_occupied_thresh = 0.65;
