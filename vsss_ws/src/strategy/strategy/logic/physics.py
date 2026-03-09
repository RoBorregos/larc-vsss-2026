from .. import constants
from ..field.ball import Ball
import math

def check_wall_collision(entity, goal_callback, canvas):
    field_bbox = [
        constants.LEFT_PADDING,
        constants.UPPER_PADDING,
        constants.LEFT_PADDING + constants.DISPLAY_FIELD_WIDTH * constants.DISPLAY_SCALE,
        constants.UPPER_PADDING + constants.DISPLAY_FIELD_HEIGHT * constants.DISPLAY_SCALE
    ]

    field_height_px = constants.DISPLAY_FIELD_HEIGHT * constants.DISPLAY_SCALE
    goal_half_width = (constants.DISPLAY_GOAL_WIDTH / 2)

    goal_top = (field_height_px / 2) - goal_half_width + constants.UPPER_PADDING
    goal_bottom = (field_height_px / 2) + goal_half_width + constants.UPPER_PADDING

    screen_x, screen_y = entity.pixel_to_world(entity.real_x, entity.real_y)

    min_x = field_bbox[0] + entity.screen_radius
    max_x = field_bbox[2] - entity.screen_radius
    min_y = field_bbox[1] + entity.screen_radius
    max_y = field_bbox[3] - entity.screen_radius

    is_ball = (entity.id == constants.BALL_ID)
    in_goal_zone = goal_top < screen_y < goal_bottom

    if screen_x < min_x:
        if is_ball and in_goal_zone:
            goal_callback(canvas)
        else:
            new_real_x, _ = entity.world_to_pixel(min_x, screen_y)
            entity.real_x = new_real_x
            entity.real_vx = 0

    elif screen_x > max_x:
        if is_ball and in_goal_zone:
            goal_callback(canvas)
        else:
            new_real_x, _ = entity.world_to_pixel(max_x, screen_y)
            entity.real_x = new_real_x
            entity.real_vx = 0

    if screen_y < min_y:
        _, new_real_y = entity.world_to_pixel(screen_x, min_y)
        entity.real_y = new_real_y
        entity.real_vy = 0
    elif screen_y > max_y:
        _, new_real_y = entity.world_to_pixel(screen_x, max_y)
        entity.real_y = new_real_y
        entity.real_vy = 0

def get_axes(entity):
    if entity.id == constants.BALL_ID:
        return []

    angle = entity.theta
    return [
        (math.cos(angle), math.sin(angle)),
        (-math.sin(angle), math.cos(angle))
    ]


def project_entity_axes(entity, axis):
    # Split vectors into x and y components
    ax, ay = axis
    cx, cy = entity.real_x, entity.real_y

    if entity.id == constants.BALL_ID:
        # Project the ball center onto the axis using the dot product
        projection_center = cx * ax + cy * ay
        return [projection_center - entity.real_radius, projection_center + entity.real_radius]

    # The apothem of the robot
    r = entity.real_radius
    angle = entity.theta

    # Calculate the vectors from the robot center to the edges
    dx = math.cos(angle) * r
    dy = math.sin(angle) * r
    ux = -math.sin(angle) * r
    uy = math.cos(angle) * r

    # Combinate the field coordinates, with the vectors of the robot center to the edges
    corners = [
        (cx + dx + ux, cy + dy + uy),
        (cx + dx - ux, cy + dy - uy),
        (cx - dx + ux, cy - dy + uy),
        (cx - dx - ux, cy - dy - uy)
    ]

    # Project each of the 4 corners onto the axis using the Dot Product
    projections = [px * ax + py * ay for px, py in corners]
    return [min(projections), max(projections)]


# This function features the Hyperplane Separation Theorem
# https://en.wikipedia.org/wiki/Hyperplane_separation_theorem
def handle_collision(entity_a, entity_b):
    # Get the list of all the axes
    axes = get_axes(entity_a) + get_axes(entity_b)

    # Get the euclidian distance
    dx = entity_b.real_x - entity_a.real_x
    dy = entity_b.real_y - entity_a.real_y
    dist_sq = dx ** 2 + dy ** 2

    # Early return if they are the same entity (preventing also division by 0)
    if dist_sq == 0: return

    dist = math.sqrt(dist_sq)

    # If a ball is involved, add the axis connecting the two centers
    if entity_a.id == constants.BALL_ID or entity_b.id == constants.BALL_ID:
        axes.append((dx / dist, dy / dist))

    # Initialize overlap with infinity to find the Minimum Separation Vector (MSV)
    overlap = float('inf')
    smallest_axis = (0, 0)

    for axis in axes:
        # Project both entities onto the current axis to check for gaps
        min_a, max_a = project_entity_axes(entity_a, axis)
        min_b, max_b = project_entity_axes(entity_b, axis)

        # Calculate the overlap magnitude on this specific axis
        current_overlap = min(max_a, max_b) - max(min_a, min_b)

        # SAT Principle: If there is no overlap on any axis, there is no collision
        if current_overlap <= 0:
            return

        # Keep track of the axis with the smallest overlap to resolve collision efficiently
        if current_overlap < overlap:
            overlap = current_overlap
            smallest_axis = axis

    # Normalize the resolution vector (nx, ny)
    nx, ny = smallest_axis
    if (dx * nx + dy * ny) < 0:
        nx, ny = -nx, -ny

    # Static resolution: Push entities apart based on their weights to prevent sinking
    total_w = entity_a.weight + entity_b.weight
    entity_a.real_x -= nx * (overlap * (entity_b.weight / total_w))
    entity_a.real_y -= ny * (overlap * (entity_b.weight / total_w))
    entity_b.real_x += nx * (overlap * (entity_a.weight / total_w))
    entity_b.real_y += ny * (overlap * (entity_a.weight / total_w))

    # Dynamic resolution: Calculate the impulse for the bounce
    rvx = entity_b.real_vx - entity_a.real_vx
    rvy = entity_b.real_vy - entity_a.real_vy
    vel_normal = (rvx * nx + rvy * ny)

    # Apply impulse only if entities are moving towards each other
    if vel_normal < 0:
        impulse = -(1 + constants.RESTITUTION) * vel_normal / (1 / entity_a.weight + 1 / entity_b.weight)

        entity_a.real_vx -= (impulse / entity_a.weight) * nx
        entity_a.real_vy -= (impulse / entity_a.weight) * ny
        entity_b.real_vx += (impulse / entity_b.weight) * nx
        entity_b.real_vy += (impulse / entity_b.weight) * ny

def resolve_physics(entities, goal_callback, canvas):
    num_entities = len(entities)

    for i in range(num_entities):
        entity_a = entities[i]
        entity_a.real_vx *= constants.FRICTION
        entity_a.real_vy *= constants.FRICTION
        check_wall_collision(entity_a, goal_callback, canvas)

        for j in range(i + 1, num_entities):
            entity_b = entities[j]
            handle_collision(entity_a, entity_b)