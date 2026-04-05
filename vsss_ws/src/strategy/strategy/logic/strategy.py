import math
from typing import List
from ..field.ball import Ball
from ..field.robot import Robot
from . import roles
from .utils import distance_between, angle_between, angle_between_relative, get_dynamic_threshold, clamp
from .. import constants
from .path_planning import *
import time

last_print_time = 0

def strategy(ball: Ball, team_robots: List[Robot], enemy_robots: List[Robot]):
    global last_print_time

    roles.select(team_robots, ball, 'LEFT')

    attacker = roles.get_attacker(team_robots)
    defender = roles.get_defender(team_robots)
    helper = roles.get_helper(team_robots)

    current_time = time.time()
    if (current_time - last_print_time) >= constants.DISPLAY_BALL_PREDICTION_INTERVAL:
        print(f"ball is going to be in: ({ball.pred_x:.2f}, {ball.pred_y:.2f})")
        last_print_time = current_time

    if attacker:
        global_angle = angle_between(attacker, ball)
        relative_angle = angle_between_relative(attacker, ball)
        relative_angle_deg = math.degrees(relative_angle)
        distance = distance_between(attacker, ball)

        if distance < constants.DISTANCE_TO_BALL_THRESHOLD and abs(relative_angle_deg) < constants.ANGLE_THRESHOLD_MIN:
            attacker.kicker(True)
        else:
            attacker.kicker(False)

        dx_goal = constants.ZONE_GOAL["x"] - ball.x
        dy_goal = constants.ZONE_GOAL["y"] - ball.y

        dist_goal = math.hypot(dx_goal, dy_goal)

        if dist_goal != 0:
            dx_goal /= dist_goal
            dy_goal /= dist_goal

        target_x = ball.x - constants.BEHIND_BALL_OFFSET * dx_goal
        target_y = ball.y - constants.BEHIND_BALL_OFFSET * dy_goal

        has_possession = getattr(attacker, "has_ball", False)

        goal_x = constants.ZONE_GOAL["x"]
        goal_half = constants.ZONE_GOAL["MIDPOINT_OFFSET"]

        top_angle = math.atan2(goal_half - ball.y, goal_x - ball.x)
        bottom_angle = math.atan2(-goal_half - ball.y, goal_x - ball.x)

        angle_min = min(top_angle, bottom_angle)
        angle_max = max(top_angle, bottom_angle)

        blocked_intervals = []

        for enemy in enemy_robots:
            dx = enemy.x - ball.x
            dy = enemy.y - ball.y
            dist = math.hypot(dx, dy)

            if dist == 0:
                continue

            angle_to_enemy = math.atan2(dy, dx)

            robot_radius = constants.ROBOT_RADIUS + constants.OBSTACLE_SAFETY_MARGIN
            angle_width = math.atan2(robot_radius, dist)

            block_start = max(angle_to_enemy - angle_width, angle_min)
            block_end = min(angle_to_enemy + angle_width, angle_max)

            if block_start < block_end:
                blocked_intervals.append((block_start, block_end))

        blocked_intervals.sort()

        free_intervals = []
        current_start = angle_min

        for start, end in blocked_intervals:
            if start > current_start:
                free_intervals.append((current_start, start))
            current_start = max(current_start, end)

        if current_start < angle_max:
            free_intervals.append((current_start, angle_max))

        best_interval = None
        best_score = -float('inf')

        robot_angle = attacker.theta

        for interval in free_intervals:
            width = interval[1] - interval[0]
            mid_angle = (interval[0] + interval[1]) / 2

            angle_diff = math.atan2(math.sin(mid_angle - robot_angle),
                                    math.cos(mid_angle - robot_angle))

            score = width - 0.5 * abs(angle_diff)

            if score > best_score:
                best_score = score
                best_interval = interval

        if best_interval:
            best_angle = (best_interval[0] + best_interval[1]) / 2
        else:
            best_angle = 0

        dx_goal = math.cos(best_angle)
        dy_goal = math.sin(best_angle)

        t = (goal_x - ball.x) / dx_goal if dx_goal != 0 else 0

        aim_x = goal_x
        aim_y = ball.y + t * dy_goal

        aim_y = clamp(aim_y, -goal_half + 0.02, goal_half - 0.02)

        if not has_possession:
            final_target_x = target_x
            final_target_y = target_y
        else:
            final_target_x = aim_x
            final_target_y = aim_y

        fx, fy = field(attacker, final_target_x, final_target_y, enemy_robots, team_robots, ball, role='attacker')
        speed, move_angle = resultant_vector(fx, fy, constants.BASE_SPEED)

        if not has_possession:

            dx_ball = ball.x - attacker.x
            dy_ball = ball.y - attacker.y

            dx_goal = constants.ZONE_GOAL["x"] - attacker.x
            dy_goal = constants.ZONE_GOAL["y"] - attacker.y

            dx_orient = constants.W_BALL * dx_ball + constants.W_GOAL * dx_goal
            dy_orient = constants.W_BALL * dy_ball + constants.W_GOAL * dy_goal
            
        else:
            if distance < constants.POSSESSION_DISTANCE_ENTER:
                dx_orient = aim_x - attacker.x
                dy_orient = aim_y - attacker.y
            else:
                dx_orient = final_target_x - attacker.x
                dy_orient = final_target_y - attacker.y

        goal_angle = math.atan2(dy_orient, dx_orient)

        attacker.move(speed, move_angle, math.degrees(goal_angle))
        
    if defender:
        dx = 0
        dy = 0

        left_limit  = -constants.ZONE_GOAL["x"] - constants.ZONE_GOAL["LEFT_PADDING"]
        right_limit = -constants.ZONE_GOAL["x"] + constants.ZONE_GOAL["RIGHT_PADDING"]

        
    
        if left_limit > defender.x or right_limit < defender.x:
            target_x = (left_limit + right_limit) / 2
            target_y = 0

            fx, fy = field(defender, target_x, target_y, enemy_robots, team_robots, ball, role='defender')
            speed, move_angle = resultant_vector(fx, fy, constants.BASE_SPEED)

        
            global_angle = math.atan2(fy, fx)

        else:
            
            top_limit = constants.ZONE_GOAL["MIDPOINT_OFFSET"]
            bottom_limit = -constants.ZONE_GOAL["MIDPOINT_OFFSET"]

            target_y = clamp(ball.y, bottom_limit, top_limit)
            target_x = (left_limit + right_limit) / 2

            dx = target_x - defender.x
            dy = target_y - defender.y

            distance = math.hypot(dx, dy)
            math.degrees(global_angle)
            if distance > constants.GOALKEEPER_Y_THRESHOLD:
                move_angle = math.atan2(dy, dx)
                speed = constants.BASE_SPEED
            else:
                move_angle = 0
                speed = 0

      
        dx_ball = ball.x - defender.x
        dy_ball = ball.y - defender.y
        global_angle = math.atan2(dy_ball, dx_ball)

        defender.move(speed, move_angle, math.degrees(global_angle))
        
    if helper:
    
        target_x = attacker.x - constants.HELPER_FOLLOW_DISTANCE
        
        if ball.y >= attacker.y:
            offset = -constants.HELPER_FOLLOW_DISTANCE / 2
        else:
            offset = constants.HELPER_FOLLOW_DISTANCE / 2

        target_y = attacker.y + offset

        move_dx = target_x - helper.x
        move_dy = target_y - helper.y
        distance = math.hypot(move_dx, move_dy)

        move_angle = 0
        speed = 0

        if distance > constants.HELPER_MINIMUM_DISTANCE_TO_ATTACKER:
            fx, fy = field(helper, target_x, target_y, enemy_robots, team_robots, ball, role='helper')
            speed, move_angle = resultant_vector(fx, fy, constants.BASE_SPEED)

        helper.move(speed, move_angle, 0)