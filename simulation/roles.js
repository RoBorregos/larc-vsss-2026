/** @typedef {import('./robot.js').Robot} Robot */
/** @typedef {import('./ball.js').Ball} Ball */

import {angle_between, distance_to_ball, distance_to_goal} from "./utils.js";

/**
 * @param {Robot[]} robots
 * @return {Robot | null}
 */
export function get_attacker(robots) {
    const attacker = robots.find(robot => robot.role === 'attacker');

    return attacker || null;
}

/**
 * @param {Robot[]} robots
 * @return {Robot | null}
 */
export function get_defender(robots) {
    const defender = robots.find(robot => robot.role === 'defender');

    return defender || null;
}

/**
 * @param {Robot[]} robots
 * @param {Ball} ball
 * @param {HTMLCanvasElement} canvas
 * @param {'left' | 'right'} side
 */
export function select(robots, ball, canvas, side) {
    if (robots.length !== 3) throw new Error('robots must be an array of length 3');
    for (let robot of robots) {
        robot.defender_cost = robot_defender_cost(robot, canvas, side);
        robot.attacker_cost = robot_attacker_cost(robot, ball);
    }

    const sortedByAttacker = [...robots].sort((a, b) => b.attacker_cost - a.attacker_cost);

    const attacker = sortedByAttacker[0];
    attacker.role = 'attacker';

    const remaining = sortedByAttacker.slice(1);

    remaining.sort((a, b) => b.defender_cost - a.defender_cost);

    remaining[0].role = 'defender';
    remaining[1].role = 'helper';
}

/**
 * @param {Robot} robot
 * @param {HTMLCanvasElement} canvas
 * @param {'left' | 'right'} side
 */
export function robot_defender_cost (robot, canvas, side) {
    const distance_to_goal_weight = 0.6;
    const hysteresis_weight = 0.4;

    // Distance to goal
    // < 5cm -> 100%
    // > 200cm -> 0%
    const distance = distance_to_goal(robot, canvas, side);
    let distance_score = (distance - 5) / (200 - 5);
    distance_score = 1 - Math.max(0, Math.min(1, distance_score));

    // Hysteresis bonus
    // role == 'defender'
    // role != 'defender'
    const hysteresis_score = (robot.role === 'defender') ? 1 : 0;

    return (distance_score * distance_to_goal_weight) +
        (hysteresis_score * hysteresis_weight);
}

/**
 * @param {Robot} robot
 * @param {Ball} ball
 */
export function robot_attacker_cost (robot, ball) {
    const distance_to_ball_weight = 0.6;
    const angle_to_ball_weight = 0.3;
    const hysteresis_weight = 0.1;

    // Distance to ball
    // < 5cm -> 100%
    // > 200 cm -> 0%
    const distance = distance_to_ball(robot, ball);
    let distance_score = (distance - 5) / (200 - 5);
    distance_score = 1 - Math.max(0, Math.min(1, distance_score));

    // Angle to ball
    // 0° -> 100%
    // 180/-180 -> 0%
    const angle = angle_between(robot, ball);
    const angle_score = 1 - Math.abs(angle) / Math.PI;

    // Hysteresis bonus
    // role == 'attacker' -> 100%
    // role != 'attacker' -> 0%;
    const hysteresis_score = (robot.role === 'attacker') ? 1 : 0;

    return (distance_score * distance_to_ball_weight) +
        (angle_score * angle_to_ball_weight) +
        (hysteresis_score * hysteresis_weight);
}

