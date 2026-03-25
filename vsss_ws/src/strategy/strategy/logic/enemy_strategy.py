import math
from typing import List
from ..field.ball import Ball
from ..field.robot import Robot
from . import roles
from .utils import distance_to_goal, distance_between, angle_between, angle_between_relative, get_dynamic_threshold, clamp
from .. import constants
from .path_planning import *
import time

rotation_angle = 0

def enemy_strategy(ball: Ball, enemy_robots: List[Robot], side = 'RIGHT'):
    global rotation_angle

    roles.select(enemy_robots, ball, 'LEFT')

    attacker = roles.get_attacker(enemy_robots)
    defender = roles.get_defender(enemy_robots)
    helper = roles.get_helper(enemy_robots)

    if attacker:
        move_angle = angle_between(attacker, ball)
        angle_deg = math.degrees(move_angle)

        if abs(angle_deg) >= constants.ANGLE_THRESHOLD_MIN:
            negative = angle_deg > 0
            offset = math.radians(constants.ANGLE_OFFSET)
            move_angle += offset * (-1 if negative else 1)

        attacker.move(constants.BASE_SPEED, move_angle, 180)

    if defender:
        rotation_angle += 10
        defender.move(0, 0, rotation_angle)
