import math
from ..constants import PhysicalProperties
from .. import settings

_FIELD = PhysicalProperties.Field
_BALL = PhysicalProperties.Ball
_ROBOT = PhysicalProperties.Robot


def _is_ball(entity) -> bool:
    return entity.id == _BALL.ID


def _get_half_extent_mm(entity) -> float:
    return entity.radius_mm if _is_ball(entity) else entity.apothem_mm


def _relative_angle_to_ball(robot, ball) -> float:
    angle_to_ball = math.atan2(ball.real_y - robot.real_y, ball.real_x - robot.real_x)
    diff = angle_to_ball - robot.real_theta
    return (diff + math.pi) % (2 * math.pi) - math.pi


def check_wall_collision(entity, goal_callback, canvas) -> None:
    half_w = _FIELD.WIDTH_MM / 2
    half_h = _FIELD.HEIGHT_MM / 2
    goal_half_w = _FIELD.GOAL_WIDTH_MM / 2
    extent = _get_half_extent_mm(entity)

    min_x, max_x = -half_w + extent, half_w - extent
    min_y, max_y = -half_h + extent, half_h - extent

    is_ball = _is_ball(entity)
    in_goal_zone = -goal_half_w < entity.real_y < goal_half_w
    restitution = entity.restitution

    # Paredes izquierda / derecha (o gol)
    if entity.real_x < min_x:
        if is_ball and in_goal_zone:
            goal_callback(canvas)
        else:
            entity.real_x = min_x
            entity.real_vx = abs(entity.real_vx) * restitution
    elif entity.real_x > max_x:
        if is_ball and in_goal_zone:
            goal_callback(canvas)
        else:
            entity.real_x = max_x
            entity.real_vx = -abs(entity.real_vx) * restitution

    # Paredes inferior / superior
    if entity.real_y < min_y:
        entity.real_y = min_y
        entity.real_vy = abs(entity.real_vy) * restitution
    elif entity.real_y > max_y:
        entity.real_y = max_y
        entity.real_vy = -abs(entity.real_vy) * restitution


def get_axes(entity) -> list[tuple[float, float]]:
    if _is_ball(entity):
        return []

    cos_t = math.cos(entity.real_theta)
    sin_t = math.sin(entity.real_theta)
    return [(cos_t, sin_t), (-sin_t, cos_t)]


def project_entity_axes(entity, axis: tuple[float, float]) -> tuple[float, float]:
    ax, ay = axis
    cx, cy = entity.real_x, entity.real_y

    if _is_ball(entity):
        center_proj = cx * ax + cy * ay
        return (center_proj - entity.radius_mm, center_proj + entity.radius_mm)

    r = entity.apothem_mm
    cos_t = math.cos(entity.real_theta)
    sin_t = math.sin(entity.real_theta)

    dx, dy = cos_t * r, sin_t * r
    ux, uy = -sin_t * r, cos_t * r

    corners = (
        (cx + dx + ux, cy + dy + uy),
        (cx + dx - ux, cy + dy - uy),
        (cx - dx + ux, cy - dy + uy),
        (cx - dx - ux, cy - dy - uy),
    )
    projections = [px * ax + py * ay for px, py in corners]
    return (min(projections), max(projections))


def kick_ball(ball, robot) -> None:
    # Impulso convertido a cambio de velocidad (mm/s) según la masa de la pelota
    # Si prefieres definir directamente la velocidad de patada en mm/s, usa: kick_speed = _ROBOT.KICK_SPEED_MM_S
    kick_speed_mm_s = (_ROBOT.KICK_IMPULSE * 1000.0) / ball.mass_g

    ball.real_vx += math.cos(robot.real_theta) * kick_speed_mm_s
    ball.real_vy += math.sin(robot.real_theta) * kick_speed_mm_s
    robot.kick_request = False


def handle_collision(entity_a, entity_b) -> None:
    axes = get_axes(entity_a) + get_axes(entity_b)

    dx = entity_b.real_x - entity_a.real_x
    dy = entity_b.real_y - entity_a.real_y
    dist_sq = dx * dx + dy * dy
    if dist_sq == 0:
        return

    if _is_ball(entity_a) or _is_ball(entity_b):
        dist = math.sqrt(dist_sq)
        axes.append((dx / dist, dy / dist))

    overlap = float("inf")
    smallest_axis = (0.0, 0.0)

    for axis in axes:
        min_a, max_a = project_entity_axes(entity_a, axis)
        min_b, max_b = project_entity_axes(entity_b, axis)
        current_overlap = min(max_a, max_b) - max(min_a, min_b)

        if current_overlap <= 0:
            return

        if current_overlap < overlap:
            overlap = current_overlap
            smallest_axis = axis

    # Revisar patada si uno de los dos es la pelota
    tol = settings.strategy.kick_angle_tolerance_rad
    if _is_ball(entity_a) and entity_b.kick_request and abs(_relative_angle_to_ball(entity_b, entity_a)) < tol:
        kick_ball(entity_a, entity_b)
    elif _is_ball(entity_b) and entity_a.kick_request and abs(_relative_angle_to_ball(entity_a, entity_b)) < tol:
        kick_ball(entity_b, entity_a)

    # Orientar la normal desde A hacia B
    nx, ny = smallest_axis
    if (dx * nx + dy * ny) < 0:
        nx, ny = -nx, -ny

    # Resolución estática (separación proporcional a las masas)
    mass_a = entity_a.mass_g
    mass_b = entity_b.mass_g
    total_mass = mass_a + mass_b
    if total_mass <= 0:
        return

    entity_a.real_x -= nx * (overlap * (mass_b / total_mass))
    entity_a.real_y -= ny * (overlap * (mass_b / total_mass))
    entity_b.real_x += nx * (overlap * (mass_a / total_mass))
    entity_b.real_y += ny * (overlap * (mass_a / total_mass))

    # Resolución dinámica (impulso de rebote)
    rvx = entity_b.real_vx - entity_a.real_vx
    rvy = entity_b.real_vy - entity_a.real_vy
    vel_normal = rvx * nx + rvy * ny

    if vel_normal < 0:
        restitution = min(entity_a.restitution, entity_b.restitution)
        inv_mass_sum = (1.0 / mass_a) + (1.0 / mass_b)
        impulse = -(1.0 + restitution) * vel_normal / inv_mass_sum

        entity_a.real_vx -= (impulse / mass_a) * nx
        entity_a.real_vy -= (impulse / mass_a) * ny
        entity_b.real_vx += (impulse / mass_b) * nx
        entity_b.real_vy += (impulse / mass_b) * ny


def resolve_physics(entities: list, goal_callback, canvas) -> None:
    for i, entity_a in enumerate(entities):
        check_wall_collision(entity_a, goal_callback, canvas)
        for entity_b in entities[i + 1 :]:
            handle_collision(entity_a, entity_b)