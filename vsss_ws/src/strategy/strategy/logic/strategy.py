import math
from typing import List
from ..field.ball import Ball
from ..field.robot import Robot
from . import roles
from .utils import distance_to_goal, distance_between, angle_between, angle_between_relative, get_dynamic_threshold
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

        if math.fabs(relative_angle_deg) > get_dynamic_threshold(distance) or distance > constants.DISTANCE_TO_BALL_THRESHOLD:
            target_x = ball.x - constants.BEHIND_BALL_OFFSET * math.cos(attacker.theta)
            target_y = ball.y - constants.BEHIND_BALL_OFFSET * math.sin(attacker.theta)

            print(f"X offset {(constants.BEHIND_BALL_OFFSET * math.cos(attacker.theta)):0.2} Y offset {(constants.BEHIND_BALL_OFFSET * math.sin(attacker.theta)):0.2}")

            fx, fy = field(attacker, target_x, target_y, enemy_robots, team_robots, ball)
            speed, move_angle = resultant_vector(fx, fy, constants.BASE_SPEED)
        else:
            move_angle = global_angle
            print(f"Moving to {math.degrees(global_angle)}")

        attacker.move(speed, move_angle, math.degrees(global_angle))


    if defender:
        dx=0
        dy=0

        left_limit  = -constants.ZONE_GOAL["x"] - constants.ZONE_GOAL["LEFT_PADDING"]
        right_limit = -constants.ZONE_GOAL["x"] + constants.ZONE_GOAL["RIGHT_PADDING"]

        if left_limit > defender.x or right_limit < defender.x:
            target_x = (left_limit + right_limit) / 2
            target_y = 0

            fx, fy = field(attacker, target_x, target_y, enemy_robots, team_robots, ball)
            speed, move_angle = resultant_vector(fx, fy, constants.BASE_SPEED)
        else:
            error_y = ball.y - defender.y
            if abs(error_y) > constants.GOALKEEPER_Y_THRESHOLD:
                dy = 1 if error_y > 0 else -1

            if dx != 0 or dy != 0:
                move_angle = math.atan2(dy, dx)
                speed = constants.BASE_SPEED
            else:
                move_angle = 0
                speed = 0

        defender.move(speed, move_angle, 0)

    if helper:
        target_x = attacker.x - constants.HELPER_FOLLOW_DISTANCE
        target_y = attacker.y

        move_dx = target_x - helper.x
        move_dy = target_y - helper.y
        distance = math.hypot(move_dx, move_dy)

        move_angle = 0
        speed = 0
        if distance > constants.HELPER_MINIMUM_DISTANCE_TO_ATTACKER:
            fx, fy = field(helper, target_x, target_y, enemy_robots, team_robots, ball)
            speed, move_angle = resultant_vector(fx, fy, constants.BASE_SPEED)

        helper.move(speed, move_angle, 0)
