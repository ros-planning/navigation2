from trajectory_generator import TrajectoryGenerator
import numpy as np
from collections import defaultdict

import math
import time

class LatticeGenerator:

    def __init__(self, config):
        self.grid_separation = config["gridSeparation"]
        self.trajectory_generator = TrajectoryGenerator(config)
        self.turning_radius = config["turningRadius"]
        self.max_level = round(config["maxLength"] / self.grid_separation)
        self.number_of_headings = config["numberOfHeadings"]
        
    def angle_difference(self, angle_1, angle_2):

        difference = abs(angle_1 - angle_2)
        
        if difference > math.pi:
            # If difference > 180 return the shorter distance between the angles
            difference = 2*math.pi - difference
        
        return difference

    def get_coords_at_level(self, level):
        positions = []

        max_point_coord = self.grid_separation * level

        for i in range(level):
            varying_point_coord = self.grid_separation * i

            # Varying y-coord
            positions.append((max_point_coord, varying_point_coord))
            
            # Varying x-coord
            positions.append((varying_point_coord, max_point_coord))


        # Append the corner
        positions.append((max_point_coord, max_point_coord))

        return np.array(positions)

    def get_heading_discretization(self):
        max_val = int((((self.number_of_headings + 4)/4) -1) / 2)

        outer_edge_x = []
        outer_edge_y = []

        for i in range(-max_val, max_val+1):
            outer_edge_x += [i, i]
            outer_edge_y += [-max_val, max_val]

            if i != max_val and i != -max_val:
                outer_edge_y += [i, i]
                outer_edge_x += [-max_val, max_val]

        return [np.rad2deg(np.arctan2(j, i)) for i, j in zip(outer_edge_x, outer_edge_y)]

    def point_to_line_distance(self, p1, p2, q):
        '''
        Return minimum distance from q to line segment defined by p1, p2.
            Projects q onto line segment p1, p2 and returns the distance
        '''

        # Get back the l2-norm without the square root
        l2 = np.inner(p1-p2, p1-p2)

        if l2 == 0:
            return np.linalg.norm(p1 - q)

        # Ensure t lies in [0, 1]
        t = max(0, min(1, np.dot(q - p1, p2 - p1) / l2))
        projected_point = p1 + t * (p2 - p1)

        return np.linalg.norm(q - projected_point)

    def is_minimal_path(self, xs, ys, minimal_spanning_trajectories):

        yaws = [np.arctan2((yf - yi), (xf - xi)) for xi, yi, xf, yf in zip(xs[:-1], ys[:-1], xs[1:], ys[1:])]

        distance_threshold = 0.5 * self.grid_separation
        rotation_threshold = 0.5 * np.deg2rad(360 / self.number_of_headings)

        for x1, y1, x2, y2, yaw in zip(xs[:-1], ys[:-1], xs[1:], ys[1:], yaws[:-1]):

            p1 = np.array([x1, y1])
            p2 = np.array([x2, y2])

            for prior_end_point in minimal_spanning_trajectories:

                # TODO: point_to_line_distance gives direct distance which means the distance_threshold represents a circle 
                # around each point. Change so that we calculate manhattan distance? <- d_t will represent a box instead
                if self.point_to_line_distance(p1, p2, prior_end_point[:-1]) < distance_threshold \
                    and self.angle_difference(yaw, prior_end_point[-1]) < rotation_threshold:
                    return False
        
        return True
            

    def generate_minimal_spanning_set(self):
        result = defaultdict(list)

        all_headings = self.get_heading_discretization()

        initial_headings = sorted(list(filter(lambda x: 0 <= x and x < 90, all_headings)))

        start_level = int(np.floor(self.turning_radius * np.cos(np.arctan(1/2) - np.pi / 2) / self.grid_separation))

        for start_heading in initial_headings:
            
            minimal_trajectory_set = []
            current_level = start_level

            # To get target headings: sort headings radially and remove those that are more than 90 degrees away
            target_headings = sorted(all_headings, key=lambda x: (abs(x - start_heading),-x))
            target_headings = list(filter(lambda x : abs(start_heading - x) <= 90, target_headings))
                
            while current_level <= self.max_level:

                # Generate x,y coordinates for current level
                positions = self.get_coords_at_level(current_level)

                for target_point in positions:
                    for target_heading in target_headings:
                        xs, ys, _ = self.trajectory_generator.generate_trajectory(target_point, start_heading, target_heading)

                        if len(xs) != 0:

                            # Check if path overlaps something in minimal spanning set
                            if(self.is_minimal_path(xs, ys, minimal_trajectory_set)):
                                new_end_pose = np.array([target_point[0], target_point[1], np.deg2rad(target_heading)])
                                minimal_trajectory_set.append(new_end_pose)

                                result[start_heading].append((target_point, target_heading))

                current_level += 1


        return result


    def run(self):
        minimal_set_endpoints = self.generate_minimal_spanning_set()

        # Copy the 0 degree trajectories to 90 degerees
        end_points_for_90 = []

        for end_point, end_angle in minimal_set_endpoints[0.0]:
            x, y = end_point

            end_points_for_90.append((np.array([y, x]), 90 - end_angle))

        minimal_set_endpoints[90.0] = end_points_for_90

        # Generate the paths for all trajectories
        minmal_set_trajectories = defaultdict(list)
       
        for start_angle in minimal_set_endpoints.keys():

            for end_point, end_angle in minimal_set_endpoints[start_angle]:
                xs, ys, traj_params = self.trajectory_generator.generate_trajectory(end_point, start_angle, end_angle, step_distance=self.grid_separation)

                xs = xs.round(5)
                ys = ys.round(5)

                flipped_xs = [-x for x in xs]
                flipped_ys = [-y for y in ys]

                yaws_quad1 = [np.arctan2((yf - yi), (xf - xi)) for xi, yi, xf, yf in zip(xs[:-1], ys[:-1], xs[1:], ys[1:])] + [np.deg2rad(end_angle)]
                yaws_quad2 = [np.arctan2((yf - yi), (xf - xi)) for xi, yi, xf, yf in zip(flipped_xs[:-1], ys[:-1], flipped_xs[1:], ys[1:])] + [np.pi - np.deg2rad(end_angle)]
                yaws_quad3 = [np.arctan2((yf - yi), (xf - xi)) for xi, yi, xf, yf in zip(flipped_xs[:-1], flipped_ys[:-1], flipped_xs[1:], flipped_ys[1:])]  + [-np.pi + np.deg2rad(end_angle)]
                yaws_quad4 = [np.arctan2((yf - yi), (xf - xi)) for xi, yi, xf, yf in zip(xs[:-1], flipped_ys[:-1], xs[1:], flipped_ys[1:])] + [-np.deg2rad(end_angle)]

                arc_length = 2 * np.pi * traj_params.radius * abs(start_angle - end_angle) / 360.0
                straight_length = traj_params.start_to_arc_distance + traj_params.arc_to_end_distance
                trajectory_length = arc_length + traj_params.start_to_arc_distance + traj_params.arc_to_end_distance

                trajectory_info = (traj_params.radius, trajectory_length, arc_length, straight_length)

                
                # Special cases for trajectories that run straight across the axis
                if start_angle == 0 and end_angle == 0:
                    quadrant_1 = (start_angle, end_angle, *trajectory_info, list(zip(xs, ys, yaws_quad1)))
                    quadrant_2 = (180 - start_angle, 180 - end_angle, *trajectory_info, list(zip(flipped_xs, ys, yaws_quad2)))
                    
                    minmal_set_trajectories[quadrant_1[0]].append(quadrant_1)
                    minmal_set_trajectories[quadrant_2[0]].append(quadrant_2)
                
                elif (start_angle == 90 and end_angle == 90):
                    quadrant_1 = (start_angle, end_angle, *trajectory_info, list(zip(xs, ys, yaws_quad1)))
                    quadrant_4 = (-start_angle, -end_angle, *trajectory_info, list(zip(xs, flipped_ys, yaws_quad4)))

                    minmal_set_trajectories[quadrant_1[0]].append(quadrant_1)
                    minmal_set_trajectories[quadrant_4[0]].append(quadrant_4)
                else:
                    # Quadrants move counter-clockwise from top right (i.e. positive x and positive y)

                    quadrant_1 = (start_angle, end_angle, *trajectory_info, list(zip(xs, ys, yaws_quad1)))
                    quadrant_2 = (180 - start_angle, 180 - end_angle, *trajectory_info, list(zip(flipped_xs, ys, yaws_quad2)))
                    quadrant_3 = (start_angle - 180, end_angle - 180, *trajectory_info, list(zip(flipped_xs, flipped_ys, yaws_quad3)))
                    quadrant_4 = (-start_angle, -end_angle, *trajectory_info, list(zip(xs, flipped_ys, yaws_quad4)))
                    
                    minmal_set_trajectories[quadrant_1[0]].append(quadrant_1)
                    minmal_set_trajectories[quadrant_2[0]].append(quadrant_2)
                    minmal_set_trajectories[quadrant_3[0]].append(quadrant_3)
                    minmal_set_trajectories[quadrant_4[0]].append(quadrant_4)

        return minmal_set_trajectories