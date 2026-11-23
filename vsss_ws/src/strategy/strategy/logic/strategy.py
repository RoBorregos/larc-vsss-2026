import math
import time
from ..field.ball import Ball
from ..field.robot import Robot
from ..constants import PhysicalProperties
from .. import settings
from . import roles
from .utils import distance_between, angle_between, angle_between_relative, get_dynamic_threshold, clamp
from .path_planning import field, is_obstacle_blocking, resultant_vector

_FIELD = PhysicalProperties.Field
_last_print_time: float = 0.0


def strategy(ball: Ball, team_robots: list[Robot], enemy_robots: list[Robot]) -> None:
    global _last_print_time
    cfg = settings.strategy

    roles.select(team_robots, ball, "LEFT" if cfg.attacking_right else "RIGHT")

    attacker = roles.get_attacker(team_robots)
    defender = roles.get_defender(team_robots)
    helper = roles.get_helper(team_robots)

    now = time.time()
    if (now - _last_print_time) >= cfg.ball_prediction_print_interval_s:
        print(f"ball is going to be in: ({ball.pred_x:.2f}, {ball.pred_y:.2f})")
        _last_print_time = now

    goal_x = _FIELD.WIDTH_MM / 2.0
    goal_half = _FIELD.GOAL_WIDTH_MM / 2.0

    # --- ATACANTE ---
    if attacker:
        global_angle = angle_between(attacker, ball)
        relative_angle = angle_between_relative(attacker, ball)
        distance = distance_between(attacker, ball)

        can_kick = (
            distance < cfg.distance_to_ball_threshold_mm
            and abs(relative_angle) < cfg.angle_threshold_min_rad
        )
        attacker.kicker(can_kick)

        speed = cfg.base_speed_mm_s
        target_x = ball.x - cfg.behind_ball_offset_mm * math.cos(attacker.theta)
        target_y = ball.y - cfg.behind_ball_offset_mm * math.sin(attacker.theta)

        obstacles = [e for e in (*team_robots, *enemy_robots) if e.id != attacker.id]
        path_to_target_blocked = any(
            is_obstacle_blocking(attacker, obs, target_x, target_y) for obs in obstacles
        )

        if (
            abs(relative_angle) > get_dynamic_threshold(distance)
            or distance > cfg.distance_to_ball_threshold_mm
            or path_to_target_blocked
        ):
            fx, fy = field(attacker, target_x, target_y, enemy_robots, team_robots, ball)
            speed, move_angle = resultant_vector(fx, fy, cfg.base_speed_mm_s)
        else:
            move_angle = global_angle

        # Apuntar a la portería (esquivando si hay un enemigo bloqueando)
        aim_x, aim_y = goal_x, 0.0
        blocking_enemy = next(
            (enemy for enemy in enemy_robots if is_obstacle_blocking(ball, enemy, goal_x, 0.0)),
            None,
        )

        if blocking_enemy is not None:
            dist_top = abs(blocking_enemy.y - goal_half)
            dist_bottom = abs(blocking_enemy.y + goal_half)
            chosen_post = (goal_x, goal_half) if dist_top < dist_bottom else (goal_x, -goal_half)

            aim_x = (blocking_enemy.x + chosen_post[0]) / 2.0
            aim_y = (blocking_enemy.y + chosen_post[1]) / 2.0

        goal_angle = math.atan2(aim_y - attacker.y, aim_x - attacker.x)
        # Nota: dejé 0.0 como tenías; cámbialo a math.degrees(goal_angle) si quieres que gire hacia el hueco
        attacker.move(speed, move_angle, 0.0)

    # --- DEFENSA / PORTERO ---
    if defender:
        left_limit = -goal_x - cfg.goalkeeper_left_padding_mm
        right_limit = -goal_x + cfg.goalkeeper_right_padding_mm
        target_x = (left_limit + right_limit) / 2.0

        if defender.x < left_limit or defender.x > right_limit:
            fx, fy = field(defender, target_x, 0.0, enemy_robots, team_robots, ball)
            speed, move_angle = resultant_vector(fx, fy, cfg.base_speed_mm_s)
        else:
            target_y = clamp(ball.y, -goal_half, goal_half)
            dx = target_x - defender.x
            dy = target_y - defender.y

            if math.hypot(dx, dy) > cfg.goalkeeper_y_threshold_mm:
                move_angle = math.atan2(dy, dx)
                speed = cfg.base_speed_mm_s
            else:
                move_angle = 0.0
                speed = 0.0

        defender.move(speed, move_angle, 0.0)

    # --- HELPER ---
    if helper and attacker:
        target_x = attacker.x - cfg.helper_follow_distance_mm
        offset = -cfg.helper_follow_distance_mm / 2.0 if ball.y >= attacker.y else cfg.helper_follow_distance_mm / 2.0
        target_y = attacker.y + offset

        distance = math.hypot(target_x - helper.x, target_y - helper.y)
        move_angle, speed = 0.0, 0.0

        if distance > cfg.helper_min_distance_to_attacker_mm:
            fx, fy = field(helper, target_x, target_y, enemy_robots, team_robots, ball)
            speed, move_angle = resultant_vector(fx, fy, cfg.base_speed_mm_s)

        helper.move(speed, move_angle, 0.0)