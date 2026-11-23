import math
import time
import numpy as np
from PIL import Image

from ..constants import PhysicalProperties
from .. import settings

_FIELD = PhysicalProperties.Field


def get_frame_from_buffer(image_buffer, scale_factor: float = 0.5):
    w, h = image_buffer.size
    new_size = (int(w * scale_factor), int(h * scale_factor))
    img_resized = image_buffer.resize(new_size, Image.LANCZOS)
    frame = np.array(img_resized)
    return frame[:, :, ::-1].copy()


def distance_to_goal(robot, side: str) -> float:
    target_y = 0.0
    if side == "LEFT":
        target_x = -_FIELD.WIDTH_MM / 2
    elif side == "RIGHT":
        target_x = _FIELD.WIDTH_MM / 2
    else:
        raise ValueError(f"Unknown goal side: {side}")

    return math.hypot(target_x - robot.x, target_y - robot.y)


def distance_between(robot, ball) -> float:
    return math.hypot(ball.x - robot.x, ball.y - robot.y)


def angle_between(robot, ball) -> float:
    return math.atan2(ball.y - robot.y, ball.x - robot.x)


def angle_between_relative(robot, ball) -> float:
    absolute_angle = math.atan2(ball.y - robot.y, ball.x - robot.x)
    relative_angle = absolute_angle - robot.theta
    return (relative_angle + math.pi) % (2 * math.pi) - math.pi


def get_dynamic_threshold(distance_mm: float) -> float:
    s = settings.strategy

    if distance_mm <= s.distance_threshold_min_mm:
        return s.angle_threshold_max_rad
    if distance_mm >= s.distance_threshold_max_mm:
        return s.angle_threshold_min_rad

    return s.angle_threshold_max_rad + (
        (distance_mm - s.distance_threshold_min_mm)
        * (s.angle_threshold_min_rad - s.angle_threshold_max_rad)
        / (s.distance_threshold_max_mm - s.distance_threshold_min_mm)
    )


def millis() -> int:
    return int(time.perf_counter() * 1000)


def clamp(value: float, min_val: float, max_val: float) -> float:
    return max(min_val, min(max_val, value))