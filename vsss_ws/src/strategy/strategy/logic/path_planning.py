import math
from ..constants import PhysicalProperties, VisualProperties
from .. import settings
from . import utils

_FIELD = PhysicalProperties.Field
_ROBOT = PhysicalProperties.Robot
_DISPLAY = VisualProperties.Display


def approach_side(robot, target_x: float, target_y: float, obstacle_x: float, obstacle_y: float) -> int:
    vt_x = robot.x - target_x
    vt_y = robot.y - target_y
    vo_x = obstacle_x - robot.x
    vo_y = obstacle_y - robot.y

    cross = (vt_x * vo_y) - (vt_y * vo_x)
    return -1 if cross > 0 else 1


def attractive_vector(robot, target_x: float, target_y: float) -> tuple[float, float]:
    dx = target_x - robot.x
    dy = target_y - robot.y
    dist = math.hypot(dx, dy)
    if dist == 0:
        return 0.0, 0.0

    gain = settings.path_planning.attractive_gain
    return gain * (dx / dist), gain * (dy / dist)


def is_obstacle_blocking(entity, obstacle, target_x: float, target_y: float) -> bool:
    tx = target_x - entity.x
    ty = target_y - entity.y
    t_norm = math.hypot(tx, ty)
    if t_norm == 0:
        return False

    tx /= t_norm
    ty /= t_norm

    ox = obstacle.x - entity.x
    oy = obstacle.y - entity.y
    projection = ox * tx + oy * ty

    if projection <= 0 or projection > t_norm:
        return False

    closest_x = entity.x + projection * tx
    closest_y = entity.y + projection * ty
    perp_dist = math.hypot(obstacle.x - closest_x, obstacle.y - closest_y)

    cfg = settings.path_planning
    safety_mm = 2.0 * _ROBOT.APOTHEM_MM + cfg.obstacle_safety_margin_mm
    return perp_dist < (cfg.blocking_width_mm + safety_mm)


def rolling_vector(robot, obstacle, target_x: float, target_y: float) -> tuple[float, float]:
    cfg = settings.path_planning
    dx = robot.x - obstacle.x
    dy = robot.y - obstacle.y
    dist = math.hypot(dx, dy)

    vortex_radius_mm = cfg.influence_radius_mm * cfg.expand_influence_radius
    if dist > vortex_radius_mm or dist == 0:
        return 0.0, 0.0

    rx = dx / dist
    ry = dy / dist
    side = approach_side(robot, target_x, target_y, obstacle.x, obstacle.y)

    tang_x = -ry * side
    tang_y = rx * side

    ratio = utils.clamp(1.0 - dist / vortex_radius_mm, 0.0, 1.0)
    tang_strength = cfg.vortex_gain * ratio
    radial_strength = cfg.enemy_repulsive_gain * cfg.expand_repulsive_gain * ratio

    fx = tang_x * tang_strength + rx * radial_strength
    fy = tang_y * tang_strength + ry * radial_strength
    return fx, fy


def _draw_repulsion_debug(canvas, obstacle, influence_radius_mm: float) -> None:
    tag_name = f"repulsion_field_{obstacle.id}"
    canvas.delete(tag_name)

    obs_px, obs_py = obstacle.world_to_pixel(obstacle.x, obstacle.y)
    inf_rad_px = influence_radius_mm * _DISPLAY.SCALE_PX_PER_MM
    if inf_rad_px <= 0:
        return

    steps = 6
    for i in range(steps):
        r_level = inf_rad_px * (1.0 - (i / steps))
        norm_strength = max(0.0, 1.0 - (r_level / inf_rad_px))
        red = int(255 * norm_strength)
        color = f"#{red:02x}0000"

        canvas.create_oval(
            obs_px - r_level, obs_py - r_level,
            obs_px + r_level, obs_py + r_level,
            outline=color,
            dash=(2, 2) if i == 0 else None,
            tags=tag_name,
        )
        canvas.tag_raise(tag_name)


def repulsion(robot, obstacle, influence_radius_mm: float, canvas=None) -> tuple[float, float]:
    if influence_radius_mm <= 0:
        return 0.0, 0.0

    if canvas is not None:
        _draw_repulsion_debug(canvas, obstacle, influence_radius_mm)

    dx = robot.x - obstacle.x
    dy = robot.y - obstacle.y
    dist = math.hypot(dx, dy)

    if dist > influence_radius_mm or dist == 0:
        return 0.0, 0.0

    rx = dx / dist
    ry = dy / dist
    sigma = influence_radius_mm / 2.0
    strength = settings.path_planning.enemy_repulsive_gain * math.exp(-dist / sigma)
    return rx * strength, ry * strength


def wall_repulsion(robot) -> tuple[float, float]:
    cfg = settings.path_planning
    fx, fy = 0.0, 0.0

    half_w = _FIELD.WIDTH_MM / 2.0
    half_h = _FIELD.HEIGHT_MM / 2.0
    margin = cfg.wall_margin_mm
    gain = cfg.wall_gain

    dx_left = robot.x + half_w
    dx_right = half_w - robot.x
    dy_bottom = robot.y + half_h
    dy_top = half_h - robot.y

    if dx_left < margin:
        fx += gain * (margin - dx_left) / margin
    if dx_right < margin:
        fx -= gain * (margin - dx_right) / margin
    if dy_bottom < margin:
        fy += gain * (margin - dy_bottom) / margin
    if dy_top < margin:
        fy -= gain * (margin - dy_top) / margin

    return fx, fy


def field(robot, target_x: float, target_y: float, enemies, teammates, ball, attacking_right: bool = True) -> tuple[float, float]:
    cfg = settings.path_planning
    blocking_detected = False
    total_x = 0.0
    total_y = 0.0

    for enemy in enemies:
        if is_obstacle_blocking(robot, enemy, target_x, target_y):
            blocking_detected = True
            fx, fy = rolling_vector(robot, enemy, target_x, target_y)
        else:
            fx, fy = repulsion(robot, enemy, cfg.influence_radius_mm)
        total_x += fx
        total_y += fy

    for mate in teammates:
        if mate.id != robot.id:
            if is_obstacle_blocking(robot, mate, target_x, target_y):
                blocking_detected = True
                fx, fy = rolling_vector(robot, mate, target_x, target_y)
            else:
                fx, fy = repulsion(robot, mate, cfg.team_influence_radius_mm)
            total_x += fx
            total_y += fy

    if ball:
        if is_obstacle_blocking(robot, ball, target_x, target_y):
            fx, fy = rolling_vector(robot, ball, target_x, target_y)
        else:
            fx, fy = repulsion(robot, ball, cfg.ball_influence_radius_mm)
        total_x += fx
        total_y += fy

    ax, ay = attractive_vector(robot, target_x, target_y)
    if blocking_detected:
        ax *= cfg.reduce_attraction_if_blocked
        ay *= cfg.reduce_attraction_if_blocked

    wx, wy = wall_repulsion(robot)
    total_x += ax + wx
    total_y += ay + wy

    magnitude = math.hypot(total_x, total_y)
    if magnitude > cfg.max_field:
        scale = cfg.max_field / magnitude
        total_x *= scale
        total_y *= scale

    return total_x, total_y


def resultant_vector(fx: float, fy: float, base_speed_mm_s: float) -> tuple[float, float]:
    magnitude = math.hypot(fx, fy)
    if magnitude == 0:
        return 0.0, 0.0

    angle = math.atan2(fy, fx)
    speed = utils.clamp(magnitude * base_speed_mm_s, 0.0, base_speed_mm_s)
    return speed, angle