import math
from .. import constants


ATTRACTIVE_GAIN = 3.0 # Strength of attraction toward the target point
REPULSIVE_GAIN = 1.2 # Strength of repulsion from enemy robots
WALL_GAIN = 0.1 # Strength of repulsion from field walls
INFLUENCE_RADIUS = 0.17 # Distance where enemy robots start influencing the field
WALL_MARGIN = 0.06 # Distance from wall where wall repulsion starts acting
BEHIND_BALL_OFFSET = 0.06 # Distance behind the ball where the attacker aims to position
MAX_FIELD = 4.0 # Maximum magnitude allowed for the total vector field
BLOCKING_WIDTH = 0.18 # Width of the corridor considered for blocking detection
VORTEX_GAIN = 3.0 # Strength of the tangential component in the rolling vector when blocking is detected
TEAM_REPULSION_GAIN = 0.35 # Strength of repulsion from team robots
TEAM_INFLUENCE_RADIUS = 0.12 # Distance where team robots start influencing the field
ROBOT_VORTEX_SIDE = {} # Variable to store the assigned vortex side for each robot when blocking is detected


FIELD_WIDTH = constants.DISPLAY_FIELD_WIDTH
FIELD_HEIGHT = constants.DISPLAY_FIELD_HEIGHT


def defender_target(ball, attacking_right=True):

    goal_x = (-FIELD_WIDTH / 2 + 0.05) if attacking_right else (FIELD_WIDTH / 2 - 0.05)

    max_y = constants.GOAL["y"] + constants.GOAL["MIDPOINT_OFFSET"]
    min_y = constants.GOAL["y"] - constants.GOAL["MIDPOINT_OFFSET"]

    target_y = max(min(ball.y, max_y), min_y)

    return goal_x, target_y


def helper_target(ball, attacking_right=True):

    lateral_offset = 0.30
    back_offset = 0.30

    target_x = ball.x - back_offset if attacking_right else ball.x + back_offset
    target_y = ball.y - lateral_offset if ball.y >= 0 else ball.y + lateral_offset

    return target_x, target_y


def clamp(value, min_val, max_val):
    return max(min_val, min(max_val, value))


def ball_target(ball, attacking_right=True):

    goal_x = FIELD_WIDTH / 2 if attacking_right else -FIELD_WIDTH / 2
    goal_y = 0

    dx = goal_x - ball.x
    dy = goal_y - ball.y
    norm = math.hypot(dx, dy)

    if norm == 0:
        return ball.x, ball.y

    ux = dx / norm
    uy = dy / norm

    return (
        ball.x - ux * BEHIND_BALL_OFFSET,
        ball.y - uy * BEHIND_BALL_OFFSET
    )


def approach_side(robot, ball, attacking_right=True):

    if robot.id in ROBOT_VORTEX_SIDE:
        return ROBOT_VORTEX_SIDE[robot.id]

    goal_x = FIELD_WIDTH / 2 if attacking_right else -FIELD_WIDTH / 2
    goal_y = 0

    bgx = goal_x - ball.x
    bgy = goal_y - ball.y

    brx = robot.x - ball.x
    bry = robot.y - ball.y

    cross = bgx * bry - bgy * brx

    side = 1 if cross > 0 else -1

    ROBOT_VORTEX_SIDE[robot.id] = side

    return side


def attractive_vector(robot, target_x, target_y):

    dx = target_x - robot.x
    dy = target_y - robot.y

    dist = math.hypot(dx, dy)

    if dist == 0:
        return 0.0, 0.0

    dx /= dist
    dy /= dist

    return ATTRACTIVE_GAIN * dx, ATTRACTIVE_GAIN * dy


def blocking(robot, obstacle, target_x, target_y):

    tx = target_x - robot.x
    ty = target_y - robot.y

    t_norm = math.hypot(tx, ty)

    if t_norm == 0:
        return False

    tx /= t_norm
    ty /= t_norm

    ox = obstacle.x - robot.x
    oy = obstacle.y - robot.y

    projection = ox * tx + oy * ty

    if projection <= 0:
        return False

    closest_x = robot.x + projection * tx
    closest_y = robot.y + projection * ty

    perp_dist = math.hypot(obstacle.x - closest_x, obstacle.y - closest_y)

    return perp_dist < BLOCKING_WIDTH


def rolling_vector(robot, obstacle, ball, attacking_right=True):

    dx = robot.x - obstacle.x
    dy = robot.y - obstacle.y

    dist = math.hypot(dx, dy)

    if dist > INFLUENCE_RADIUS * 1.5 or dist == 0:
        return 0.0, 0.0

    rx = dx / dist
    ry = dy / dist

    side = approach_side(robot, ball, attacking_right)

    tang_x = -ry * side
    tang_y = rx * side

    ratio = max(0.0, 1 - dist / (INFLUENCE_RADIUS * 1.5))

    tang_strength = VORTEX_GAIN * ratio
    radial_strength = REPULSIVE_GAIN * 0.15 * ratio

    fx = tang_x * tang_strength + rx * radial_strength
    fy = tang_y * tang_strength + ry * radial_strength

    return fx, fy


def repulsion(robot, obstacle):

    dx = robot.x - obstacle.x
    dy = robot.y - obstacle.y

    dist = math.hypot(dx, dy)

    if dist > INFLUENCE_RADIUS or dist == 0:
        return 0.0, 0.0

    rx = dx / dist
    ry = dy / dist

    strength = REPULSIVE_GAIN * (1 - dist / INFLUENCE_RADIUS)

    return rx * strength, ry * strength


def team_repulsion(robot, mate):

    dx = robot.x - mate.x
    dy = robot.y - mate.y

    dist = math.hypot(dx, dy)

    if dist > TEAM_INFLUENCE_RADIUS or dist == 0:
        return 0.0, 0.0

    rx = dx / dist
    ry = dy / dist

    strength = TEAM_REPULSION_GAIN * (1 - dist / TEAM_INFLUENCE_RADIUS)

    return rx * strength, ry * strength


def wall_repulsion(robot):

    fx = 0.0
    fy = 0.0

    half_width = FIELD_WIDTH / 2
    half_height = FIELD_HEIGHT / 2

    dx_left = robot.x + half_width
    dx_right = half_width - robot.x

    dy_bottom = robot.y + half_height
    dy_top = half_height - robot.y

    if dx_left < WALL_MARGIN:
        fx += WALL_GAIN * (WALL_MARGIN - dx_left) / WALL_MARGIN

    if dx_right < WALL_MARGIN:
        fx -= WALL_GAIN * (WALL_MARGIN - dx_right) / WALL_MARGIN

    if dy_bottom < WALL_MARGIN:
        fy += WALL_GAIN * (WALL_MARGIN - dy_bottom) / WALL_MARGIN

    if dy_top < WALL_MARGIN:
        fy -= WALL_GAIN * (WALL_MARGIN - dy_top) / WALL_MARGIN

    return fx, fy


def field(robot, target_x, target_y, enemies, teammates, ball, attacking_right=True):

    total_x = 0.0
    total_y = 0.0

    blocking_detected = False

    for enemy in enemies:

        if blocking(robot, enemy, target_x, target_y):

            blocking_detected = True
            fx, fy = rolling_vector(robot, enemy, ball, attacking_right)

        else:

            fx, fy = repulsion(robot, enemy)

        total_x += fx
        total_y += fy


    # reset vortex side if no obstacle is blocking
    if not blocking_detected:

        if robot.id in ROBOT_VORTEX_SIDE:
            del ROBOT_VORTEX_SIDE[robot.id]


    ax, ay = attractive_vector(robot, target_x, target_y)

    for mate in teammates:

        if mate.id != robot.id:

            fx, fy = team_repulsion(robot, mate)

            total_x += fx
            total_y += fy

    if blocking_detected:
        ax *= 0.4
        ay *= 0.4

    total_x += ax
    total_y += ay

    wx, wy = wall_repulsion(robot)

    total_x += wx
    total_y += wy

    magnitude = math.hypot(total_x, total_y)

    if magnitude > MAX_FIELD:

        total_x = (total_x / magnitude) * MAX_FIELD
        total_y = (total_y / magnitude) * MAX_FIELD

    return total_x, total_y


def resultant_vector(fx, fy, base_speed):

    magnitude = math.hypot(fx, fy)

    if magnitude == 0:
        return 0, 0

    angle = math.atan2(fy, fx)

    speed = clamp(magnitude, 0, base_speed)

    return speed, angle