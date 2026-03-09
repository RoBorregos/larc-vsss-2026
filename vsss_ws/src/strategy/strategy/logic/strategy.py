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

    if attacker:

        dx = ball.x - attacker.x
        dy = ball.y - attacker.y
        distance = math.hypot(dx, dy)

        if distance < 0.15:
            attacker.kicker(True)
        else:
            attacker.kicker(False)

        tx, ty = ball_target(ball)

        fx, fy = field(attacker, tx, ty, enemy_robots, team_robots, ball)

        speed, angle = resultant_vector(fx, fy, constants.BASE_SPEED)
        attacker.move(speed, angle)