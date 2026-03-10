import math
from typing import List
from ..field.ball import Ball
from ..field.robot import Robot
from . import roles
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
    if (current_time - last_print_time) >= 0.5:
        print(f"ball is going to be in: ({ball.pred_x:.2f}, {ball.pred_y:.2f})")
        last_print_time = current_time

    if attacker:
        dx = ball.x - attacker.x
        dy = ball.y - attacker.y
        distance = math.hypot(dx, dy)

        if distance < 0.15:
            attacker.kicker(True)
        else:
            attacker.kicker(False)

        fx, fy = field(attacker, ball.x, ball.y, enemy_robots, team_robots, ball)
        speed, angle = resultant_vector(fx, fy, constants.BASE_SPEED)
        attacker.move(speed, angle)