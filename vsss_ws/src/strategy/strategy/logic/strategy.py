import math
from typing import List
from ..field.ball import Ball
from ..field.robot import Robot
from . import roles
from .utils import distance_to_goal, distance_between, angle_between, angle_between_relative, get_dynamic_threshold, clamp
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

        if distance < constants.DISTANCE_TO_BALL_THRESHOLD and math.fabs(relative_angle_deg) < constants.ANGLE_THRESHOLD_MIN:
            attacker.kicker(True)
        else:
            attacker.kicker(False)

        speed = constants.BASE_SPEED

        target_x = ball.x - constants.BEHIND_BALL_OFFSET * math.cos(attacker.theta)
        target_y = ball.y - constants.BEHIND_BALL_OFFSET * math.sin(attacker.theta)

        path_to_target_blocked = False
        entities = [*team_robots, *enemy_robots]
        for entity in entities:
            entity_blocking = is_obstacle_blocking(attacker, entity, target_x, target_y)
            if entity_blocking:
                path_to_target_blocked = True

        if (math.fabs(relative_angle_deg) > get_dynamic_threshold(distance)
            or distance > constants.DISTANCE_TO_BALL_THRESHOLD
            or path_to_target_blocked):

            fx, fy = field(attacker, target_x, target_y, enemy_robots, team_robots, ball)
            speed, move_angle = resultant_vector(fx, fy, constants.BASE_SPEED)
        else:
            move_angle = global_angle


        goal_x = constants.ZONE_GOAL["x"]
        goal_half = constants.ZONE_GOAL["MIDPOINT_OFFSET"]

        top_post = (goal_x,  goal_half)
        bottom_post = (goal_x, -goal_half)

        aim_x = goal_x
        aim_y = 0
        
        blocking_enemy = None
        # --- Find obstacle blocking ---
        for enemy in enemy_robots:
            if is_obstacle_blocking(ball, enemy, goal_x, 0):
                blocking_enemy = enemy
                break

        # --- If blocked, aim to a corner ---
        if blocking_enemy is not None:

            dist_top = abs(blocking_enemy.y - goal_half)
            dist_bottom = abs(blocking_enemy.y + goal_half)

            if dist_top < dist_bottom:
                chosen_post = top_post
            else:
                chosen_post = bottom_post

            # Midpoint between obstacle and corner
            aim_x = (blocking_enemy.x + chosen_post[0]) / 2
            aim_y = (blocking_enemy.y + chosen_post[1]) / 2

        # --- Final orientation ---
        dx_goal = aim_x - attacker.x
        dy_goal = aim_y - attacker.y
        goal_angle = math.atan2(dy_goal, dx_goal)

        attacker.move(speed, move_angle, math.degrees(goal_angle))

    if defender:
        dx = 0
        dy = 0

        left_limit  = -constants.ZONE_GOAL["x"] - constants.ZONE_GOAL["LEFT_PADDING"]
        right_limit = -constants.ZONE_GOAL["x"] + constants.ZONE_GOAL["RIGHT_PADDING"]

        
    
        if left_limit > defender.x or right_limit < defender.x:
            target_x = (left_limit + right_limit) / 2
            target_y = 0

            fx, fy = field(defender, target_x, target_y, enemy_robots, team_robots, ball)
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
            fx, fy = field(helper, target_x, target_y, enemy_robots, team_robots, ball)
            speed, move_angle = resultant_vector(fx, fy, constants.BASE_SPEED)

        helper.move(speed, move_angle, 0)
