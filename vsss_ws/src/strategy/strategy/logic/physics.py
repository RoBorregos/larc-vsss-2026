from .. import constants
from ..field.ball import Ball
import math

ball_id = 20

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

    is_ball = (entity.id == ball_id)
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

def handle_collision(entity_a, entity_b):
    dx = entity_b.real_x - entity_a.real_x
    dy = entity_b.real_y - entity_a.real_y
    dist_sq = dx ** 2 + dy ** 2

    min_dist = entity_a.real_radius + entity_b.real_radius

    if dist_sq > min_dist ** 2 or dist_sq == 0:
        return

    dist = math.sqrt(dist_sq)

    nx = dx / dist
    ny = dy / dist
    overlap = min_dist - dist

    total_weight = entity_a.weight + entity_b.weight
    ratio_a = entity_b.weight / total_weight
    ratio_b = entity_a.weight / total_weight

    entity_a.real_x -= nx * (overlap * ratio_a)
    entity_a.real_y -= ny * (overlap * ratio_a)
    entity_b.real_x += nx * (overlap * ratio_b)
    entity_b.real_y += ny * (overlap * ratio_b)

    rvx = entity_b.real_vx - entity_a.real_vx
    rvy = entity_b.real_vy - entity_a.real_vy

    vel_along_normal = (rvx * nx + rvy * ny)

    if vel_along_normal < 0:
        e = constants.RESTITUTION

        impulse = -(1 + e) * vel_along_normal
        impulse /= (1 / entity_a.weight + 1 / entity_b.weight)

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