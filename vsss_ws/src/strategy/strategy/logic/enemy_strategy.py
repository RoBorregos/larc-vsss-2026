from ..field.ball import Ball
from ..field.robot import Robot
from .. import settings
from . import roles
from .utils import angle_between

rotation_angle = 0.0


def enemy_strategy(ball: Ball, enemy_robots: list[Robot], side: str = "RIGHT") -> None:
    global rotation_angle
    cfg = settings.strategy

    roles.select(enemy_robots, ball, side)

    attacker = roles.get_attacker(enemy_robots)
    defender = roles.get_defender(enemy_robots)

    if attacker:
        move_angle = angle_between(attacker, ball)

        if abs(move_angle) >= cfg.angle_threshold_min_rad:
            sign = -1.0 if move_angle > 0 else 1.0
            move_angle += cfg.angle_offset_rad * sign

        attacker.move(cfg.base_speed_mm_s, move_angle, 180.0)

    if defender:
        rotation_angle = (rotation_angle + 10.0) % 360.0
        defender.move(0.0, 0.0, rotation_angle)