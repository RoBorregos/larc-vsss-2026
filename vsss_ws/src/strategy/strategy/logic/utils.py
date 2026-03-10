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
    rx = robot.x
    ry = robot.y

    bx = ball.x
    by = ball.y

    dx = bx - rx
    dy = by - ry

    return math.atan2(dy, dx)


def millis():
    return int(time.perf_counter() * 1000)