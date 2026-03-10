import math
from typing import List
from ..field.ball import Ball
from ..field.robot import Robot
from . import roles
from .utils import distance_to_goal, distance_between, angle_between
from .. import constants
import time

last_print_time = 0

def strategy(ball: Ball, team_robots: List[Robot], enemy_robots: List[Robot]):
    global last_print_time
    roles.select(team_robots, ball, 'LEFT')

    attacker = roles.get_attacker(team_robots)
    defender = roles.get_defender(team_robots)
    helper = roles.get_helper(team_robots)

    current_time = time.time()
    if (current_time - last_print_time) >= 0.5:
        print(f"ball is going to be in: ({ball.pred_x:.2f}, {ball.pred_y:.2f})")
        last_print_time = current_time

    if attacker:
        move_angle = angle_between(attacker, ball)
        angle_deg = math.degrees(move_angle)

        distance = distance_between(attacker, ball)
        goal_x = 1.50
        goal_y = max(-0.20, min(0.20, ball.y))

        if distance < 0.15 and math.fabs(angle_deg) < constants.ANGLE_THRESHOLD:
            attacker.kicker(True)
        else:
            attacker.kicker(False)

        goal_angle = math.atan2(goal_y - ball.y, goal_x - ball.x)

        if distance < 0.15 and math.fabs(angle_deg) < constants.ANGLE_THRESHOLD:
            move_angle = goal_angle

        else:
            if abs(angle_deg) >= constants.ANGLE_THRESHOLD:
                negative = angle_deg < 0
                offset = math.radians(constants.ANGLE_OFFSET)
                move_angle += offset * (-1 if negative else 1)
        attacker.move(constants.BASE_SPEED, move_angle)


    if defender:
        x,y=defender.x, defender.y

        dx=0
        dy=0

        left_limit  = -constants.ZONE_GOAL["x"] - constants.ZONE_GOAL["LEFT_PADDING"]
        right_limit = -constants.ZONE_GOAL["x"] + constants.ZONE_GOAL["RIGHT_PADDING"]

        if x > right_limit:
            dx = -1
        elif x < left_limit:
            dx = 1

        target_y = ball.y

        upper_limit = constants.ZONE_GOAL["y"] + constants.ZONE_GOAL["MIDPOINT_OFFSET"]
        lower_limit = constants.ZONE_GOAL["y"] - constants.ZONE_GOAL["MIDPOINT_OFFSET"]
        
        if target_y > upper_limit:
            target_y = upper_limit
        elif target_y < lower_limit:
            target_y = lower_limit

            
        error_y = ball.y - defender.y
    
        if abs(error_y) > constants.GOALKEEPER_Y_THRESHOLD:
            dy = 1 if error_y > 0 else -1

    
        if dx != 0 or dy != 0:
            move_angle = math.atan2(dy, dx)
            defender.move(constants.BASE_SPEED * 0.8, move_angle)
        else:
            defender.move(0, 0)
     
    if helper:
        target_x = attacker.x - constants.HELPER_FOLLOW_DISTANCE
        target_y = attacker.y

        move_dx = target_x -helper.x
        move_dy = target_y - helper.y

        distance = math.hypot(move_dx, move_dy)

        if distance > 0.03:
            move_angle= math.atan2 (move_dy, move_dx)
            helper.move(constants.BASE_SPEED*0.8, move_angle)
        else:
            helper.move(0,0)
     