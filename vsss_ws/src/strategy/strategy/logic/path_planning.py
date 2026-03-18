import math
from .. import constants
from . import utils
import numpy as np

def approach_side(robot, target_x, target_y, obstacle_x, obstacle_y):
    pos_robot = np.array([robot.x, robot.y])
    pos_target = np.array([target_x, target_y])
    pos_obstacle = np.array([obstacle_x, obstacle_y])

    vector_target = pos_robot - pos_target
    vector_obstacle = pos_obstacle - pos_robot

    cross = (vector_target[0] * vector_obstacle[1]) - (vector_target[1] * vector_obstacle[0]);

    if cross > 0:
        return -1
    else:
        return 1


def attractive_vector(robot, target_x, target_y):
    # Displacement vector
    dx = target_x - robot.x
    dy = target_y - robot.y
    dist = math.hypot(dx, dy)

    if dist == 0: return 0.0, 0.0

    # Vector gain and normalization
    dx /= dist
    dy /= dist
    return constants.ATTRACTIVE_GAIN * dx, constants.ATTRACTIVE_GAIN * dy


def is_obstacle_blocking(robot, obstacle, target_x, target_y):
    # Target vector
    tx = target_x - robot.x
    ty = target_y - robot.y
    t_norm = math.hypot(tx, ty)

    if t_norm == 0: return False

    tx /= t_norm
    ty /= t_norm

    # Position of the obstacle relative to the robot
    ox = obstacle.x - robot.x
    oy = obstacle.y - robot.y

    # Check if it is in front or behind
    projection = ox * tx + oy * ty

    if projection <= 0 or projection > t_norm:
        return False

    closest_x = robot.x + projection * tx
    closest_y = robot.y + projection * ty

    perp_dist = math.hypot(obstacle.x - closest_x, obstacle.y - closest_y)
    return perp_dist < constants.BLOCKING_WIDTH


def rolling_vector(robot, obstacle, target_x, target_y):
    dx = robot.x - obstacle.x
    dy = robot.y - obstacle.y
    dist = math.hypot(dx, dy)

    # This is to make the vortex radius activate before the influence radius of an obstacle
    vortex_radius = constants.INFLUENCE_RADIUS * constants.EXPAND_INFLUENCE_RADIUS
    if dist > vortex_radius or dist == 0:
        return 0.0, 0.0

    # Make the robot
    rx = dx / dist
    ry = dy / dist
    side = approach_side(robot, target_x, target_y, obstacle.x, obstacle.y)

    tang_x = -ry * side
    tang_y = rx * side

    influence = 1 - dist / vortex_radius
    ratio = utils.clamp(influence, 0, 1)

    tang_strength = constants.VORTEX_GAIN * ratio
    radial_strength = constants.ENEMY_REPULSIVE_GAIN * constants.EXPAND_REPULSIVE_GAIN * ratio

    fx = tang_x * tang_strength + rx * radial_strength
    fy = tang_y * tang_strength + ry * radial_strength

    return fx, fy

def repulsion(robot, obstacle, influence_radius, canvas=None):
    dx = robot.x - obstacle.x
    dy = robot.y - obstacle.y
    dist = math.hypot(dx, dy)

    if canvas is not None:
        tag_name = f"repulsion_field_{obstacle.id}"
        canvas.delete(tag_name)

        obs_px, obs_py = obstacle.pixel_to_world(obstacle.x, obstacle.y)
        inf_rad_pix = influence_radius * constants.METER_TO_CM * constants.DISPLAY_SCALE

        # Number of level curves
        steps = 6
        for i in range(steps):
            r_level = inf_rad_pix * (1 - (i / steps))

            ratio_level = r_level / inf_rad_pix
            norm_strength = max(0, 1 - ratio_level)

            red = int(255 * norm_strength)
            color = f'#{red:02x}0000'
            canvas.create_oval(
                obs_px - r_level, obs_py - r_level,
                obs_px + r_level, obs_py + r_level,
                outline=color,
                dash=(2, 2) if i == 0 else None,
                tags=tag_name
            )
            canvas.tag_raise(tag_name)

    if dist > influence_radius or dist == 0:
        return 0.0, 0.0

    rx = dx / dist
    ry = dy / dist

    ratio = dist / influence_radius
    strength = constants.ENEMY_REPULSIVE_GAIN * utils.clamp(1 - ratio, 0, 1)
    return rx * strength, ry * strength

def wall_repulsion(robot):
    fx, fy = 0.0, 0.0


    half_width = constants.FIELD_WIDTH / 2
    half_height = constants.FIELD_HEIGHT / 2

    # Distances to wall
    dx_left = robot.x + half_width
    dx_right = half_width - robot.x
    dy_bottom = robot.y + half_height
    dy_top = half_height - robot.y

    # If distance < wall margin, add repulsion
    if dx_left < constants.WALL_MARGIN:
        fx += constants.WALL_GAIN * (constants.WALL_MARGIN - dx_left) / constants.WALL_MARGIN
    if dx_right < constants.WALL_MARGIN:
        fx -= constants.WALL_GAIN * (constants.WALL_MARGIN - dx_right) / constants.WALL_MARGIN
    if dy_bottom < constants.WALL_MARGIN:
        fy += constants.WALL_GAIN * (constants.WALL_MARGIN - dy_bottom) / constants.WALL_MARGIN
    if dy_top < constants.WALL_MARGIN:
        fy -= constants.WALL_GAIN * (constants.WALL_MARGIN - dy_top) / constants.WALL_MARGIN
    return fx, fy


def field(robot, target_x, target_y, enemies, teammates, ball, attacking_right=True):
    blocking_detected = False
    total_x = 0.0
    total_y = 0.0

    # Repulsion for each enemy (fx, fy) are accumulated in total_x and total_y
    for enemy in enemies:
        if is_obstacle_blocking(robot, enemy, target_x, target_y):
            blocking_detected = True
            fx, fy = rolling_vector(robot, enemy, target_x, target_y)
        else:
            fx, fy = repulsion(robot, enemy, constants.INFLUENCE_RADIUS)

        total_x += fx
        total_y += fy

    # For each teammate, find the repulsion and add to the total_x and total_y vector
    for mate in teammates:
        if mate.id != robot.id:
            if is_obstacle_blocking(robot, mate, target_x, target_y):
                blocking_detected = True
                fx, fy = rolling_vector(robot, mate, target_x, target_y)
            else:
                fx, fy = repulsion(robot, mate, constants.TEAM_INFLUENCE_RADIUS)

            total_x += fx
            total_y += fy

    if ball:
        if is_obstacle_blocking(robot, ball, target_x, target_y):
            fx, fy = rolling_vector(robot, ball, target_x, target_y)
        else:
            fx, fy = repulsion(robot, ball, constants.BALL_INFLUENCE_RADIUS)

        total_x += fx
        total_y += fy

    if not blocking_detected:
        if robot.id in constants.ROBOT_VORTEX_SIDE:
            del constants.ROBOT_VORTEX_SIDE[robot.id]


    # Get the attractive vector of the target
    ax, ay = attractive_vector(robot, target_x, target_y)

    if blocking_detected:
        ax *= constants.REDUCE_ATTRACTION_IF_BLOCKED
        ay *= constants.REDUCE_ATTRACTION_IF_BLOCKED

    total_x += ax
    total_y += ay

    # Wall repulsion to avoid hitting walls
    wx, wy = wall_repulsion(robot)

    total_x += wx
    total_y += wy
    magnitude = math.hypot(total_x, total_y)

    if magnitude > constants.MAX_FIELD:
        total_x = (total_x / magnitude) * constants.MAX_FIELD
        total_y = (total_y / magnitude) * constants.MAX_FIELD

    return total_x, total_y


def resultant_vector(fx, fy, base_speed):
    magnitude = math.hypot(fx, fy)

    if magnitude == 0:
        return 0, 0

    angle = math.atan2(fy, fx)
    speed = utils.clamp(magnitude, 0, base_speed)
    return speed, angle
