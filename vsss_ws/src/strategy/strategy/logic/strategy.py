import math
from typing import List
from ..field.ball import Ball
from ..field.robot import Robot
from . import roles
from .. import constants
from .path_planning import *


def strategy(ball: Ball, team_robots: List[Robot], enemy_robots: List[Robot]):

    roles.select(team_robots, ball, 'LEFT')

    attacker = roles.get_attacker(team_robots)
    defender = roles.get_defender(team_robots)
    helper = roles.get_helper(team_robots)


    attacking_right = True
    
    if attacker:
        move_angle = angle_between(attacker, ball)
        angle_deg = math.degrees(move_angle)
        distance = distance_to_ball(attacker, ball)

        if distance < 0.15 and math.fabs(angle_deg) < constants.ANGLE_THRESHOLD:
            attacker.kicker(True)
        else:
            attacker.kicker(False)

        tx, ty = ball_target(ball, attacking_right)
        fx, fy = field(attacker, tx, ty, enemy_robots, team_robots, ball, attacking_right)
        speed, angle = resultant_vector(fx, fy, constants.BASE_SPEED)
        attacker.move(speed, angle)


    if defender:
        tx, ty = defender_target(ball, attacking_right)
        fx, fy = field(defender, tx, ty, enemy_robots, team_robots, ball, attacking_right)
        speed, angle = resultant_vector(fx, fy, constants.BASE_SPEED)
        defender.move(speed, angle)


    if helper:
        tx, ty = helper_target(ball, attacking_right)
        fx, fy = field(helper, tx, ty, enemy_robots, team_robots, ball, attacking_right)
        speed, angle = resultant_vector(fx, fy, constants.BASE_SPEED)
        helper.move(speed, angle)