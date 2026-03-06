import math
from typing import List
from ..field.ball import Ball
from ..field.robot import Robot
from . import roles
from .utils import distance_to_goal, distance_to_ball, angle_between
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

        if abs(angle_deg) >= constants.ANGLE_THRESHOLD:
            negative = angle_deg < 0
            offset = math.radians(constants.ANGLE_OFFSET)
            move_angle += offset * (-1 if negative else 1)

        attacker.move(constants.BASE_SPEED, move_angle)

    if defender:
        x, y = defender.x, defender.y

        dx = 0
        dy = 0

        side = 'LEFT'

        if side == 'LEFT':
            if x > -constants.GOAL["x"] + constants.GOAL["RIGHT_PADDING"]:
                dx = -1
            elif x < constants.GOAL["x"] - constants.GOAL["LEFT_PADDING"]:
                dx = 1

        if y < constants.GOAL["y"] - constants.GOAL["MIDPOINT_OFFSET"]:
            dy = 1
        elif y > constants.GOAL["y"] + constants.GOAL["MIDPOINT_OFFSET"]:
            dy = -1

        if dx != 0 or dy != 0:
            move_angle = math.atan2(dy, dx)
            defender.move(constants.BASE_SPEED, move_angle)
        else:
            defender.move(0, 0)
