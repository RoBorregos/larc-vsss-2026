from PIL import ImageGrab, Image
from .. import constants
import numpy as np
import time
import math



def get_frame_from_buffer(image_buffer, scale_factor=0.5):
    w, h = image_buffer.size
    new_size = (int(w * scale_factor), int(h * scale_factor))

    img_resized = image_buffer.resize(new_size, Image.LANCZOS)

    frame = np.array(img_resized)
    frame = frame[:, :, ::-1].copy()
    return frame

def distance_to_goal(robot, side):
    rx = robot.x
    ry = robot.y

    target_y = constants.DISPLAY_CANVAS_HEIGHT / 2
    target_x = None
    if side == "LEFT":
        target_x = constants.LEFT_PADDING
    elif side == "RIGHT":
        target_y = constants.DISPLAY_CANVAS_WIDTH - constants.LEFT_PADDING
    else:
        print("Unknown team color")
        exit(1)

    dx = target_x - rx
    dy = target_y - ry

    return math.hypot(dx, dy)

def distance_between(robot, ball):
    rx = robot.x
    ry = robot.y

    bx = ball.x
    by = ball.y

    dx = bx - rx
    dy = by - ry
    return math.hypot(dx, dy)

def angle_between(robot, ball):
    dx = ball.x - robot.x
    dy = ball.y - robot.y

    return math.atan2(dy, dx)

def angle_between_relative(robot, ball):
    dx = ball.x - robot.x
    dy = ball.y - robot.y

    absolute_angle = math.atan2(dy, dx)
    relative_angle = absolute_angle - robot.theta
    relative_angle = (relative_angle + math.pi) % (2 * math.pi) - math.pi

    return relative_angle


def get_dynamic_threshold(distance):
    d_min, angle_max = constants.DISTANCE_THRESHOLD_MIN, constants.ANGLE_THRESHOLD_MAX
    d_max, angle_min = constants.DISTANCE_THRESHOLD_MAX, constants.ANGLE_THRESHOLD_MIN

    if distance <= d_min:
        return angle_max
    if distance >= d_max:
        return angle_min

    threshold = angle_max + (distance - d_min) * (angle_min - angle_max) / (d_max - d_min)

    return threshold

def millis():
    return int(time.perf_counter() * 1000)


def clamp(value, min_val, max_val):
    return max(min_val, min(max_val, value))
