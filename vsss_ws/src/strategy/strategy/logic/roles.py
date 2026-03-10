import math
from .. import constants
from .utils import distance_to_goal, distance_between, angle_between


def get_attacker(robots):
    return next((r for r in robots if getattr(r, 'role', None) == 'attacker'), None)


def get_defender(robots):
    return next((r for r in robots if getattr(r, 'role', None) == 'defender'), None)

def get_helper(robots):
    return next((r for r in robots if getattr(r, 'role', None) == 'helper'), None)

def select(robots, ball, side):
    if len(robots) != 3:
        raise ValueError('robots must be a list of length 3')

    for robot in robots:
        robot.defender_score = robot_defender_score(robot, side)
        robot.attacker_score = robot_attacker_score(robot, ball)

    sorted_by_attacker = sorted(robots, key=lambda r: r.attacker_score, reverse=True)

    attacker = sorted_by_attacker[0]
    attacker.role = 'attacker'

    remaining = sorted_by_attacker[1:]

    remaining.sort(key=lambda r: r.defender_score, reverse=True)
    defender = remaining[0]
    defender.role= "defender"

    helper=remaining[1]
    helper.role="helper"


def robot_defender_score(robot, side):
    distance_to_goal_weight = 0.6
    hysteresis_weight = 0.4

    distance = distance_to_goal(robot, side)

    min_distance = 0.05
    max_distance = constants.DISPLAY_FIELD_WIDTH / 100

    distance_score = (distance - min_distance) / (max_distance - min_distance)
    distance_score = 1 - max(0, min(1, distance_score))

    hysteresis_score = 1 if getattr(robot, 'role', None) == 'defender' else 0

    return (distance_score * distance_to_goal_weight) + \
        (hysteresis_score * hysteresis_weight)


def robot_attacker_score(robot, ball):
    distance_to_ball_weight = 0.6
    angle_to_ball_weight = 0.3
    hysteresis_weight = 0.1

    distance = distance_between(robot, ball)

    min_distance = 0.05
    max_distance = constants.DISPLAY_FIELD_WIDTH / 100

    distance_score = (distance - min_distance) / (max_distance - min_distance)
    distance_score = 1 - max(0, min(1, distance_score))

    angle = angle_between(robot, ball)
    angle_score = 1 - abs(angle) / math.pi

    hysteresis_score = 1 if getattr(robot, 'role', None) == 'attacker' else 0

    return (distance_score * distance_to_ball_weight) + \
        (angle_score * angle_to_ball_weight) + \
        (hysteresis_score * hysteresis_weight)