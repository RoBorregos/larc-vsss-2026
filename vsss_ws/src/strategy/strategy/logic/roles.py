import math
from ..constants import PhysicalProperties
from .. import settings
from .utils import distance_to_goal, distance_between, angle_between_relative, clamp

_FIELD_WIDTH_MM = PhysicalProperties.Field.WIDTH_MM


def get_attacker(robots):
    return next((r for r in robots if getattr(r, "role", None) == "attacker"), None)


def get_defender(robots):
    return next((r for r in robots if getattr(r, "role", None) == "defender"), None)


def get_helper(robots):
    return next((r for r in robots if getattr(r, "role", None) == "helper"), None)


def _normalize_distance_score(distance_mm: float) -> float:
    min_d = settings.roles.min_score_distance_mm
    max_d = _FIELD_WIDTH_MM
    normalized = (distance_mm - min_d) / (max_d - min_d)
    return 1.0 - clamp(normalized, 0.0, 1.0)


def select(robots, ball, side: str) -> None:
    if len(robots) != 3:
        raise ValueError("robots must be a list of length 3")

    for robot in robots:
        robot.defender_score = robot_defender_score(robot, side)
        robot.attacker_score = robot_attacker_score(robot, ball)

    sorted_by_attacker = sorted(robots, key=lambda r: r.attacker_score, reverse=True)
    sorted_by_attacker[0].role = "attacker"

    remaining = sorted(sorted_by_attacker[1:], key=lambda r: r.defender_score, reverse=True)
    remaining[0].role = "defender"
    remaining[1].role = "helper"


def robot_defender_score(robot, side: str) -> float:
    cfg = settings.roles
    distance_score = _normalize_distance_score(distance_to_goal(robot, side))
    hysteresis_score = 1.0 if getattr(robot, "role", None) == "defender" else 0.0

    return (
        distance_score * cfg.defender_distance_weight
        + hysteresis_score * cfg.defender_hysteresis_weight
    )


def robot_attacker_score(robot, ball) -> float:
    cfg = settings.roles
    distance_score = _normalize_distance_score(distance_between(robot, ball))

    angle = angle_between_relative(robot, ball)
    angle_score = 1.0 - abs(angle) / math.pi

    hysteresis_score = 1.0 if getattr(robot, "role", None) == "attacker" else 0.0

    return (
        distance_score * cfg.attacker_distance_weight
        + angle_score * cfg.attacker_angle_weight
        + hysteresis_score * cfg.attacker_hysteresis_weight
    )