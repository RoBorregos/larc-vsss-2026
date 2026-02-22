/** @typedef {import('./robot.js').Robot} Robot */
/** @typedef {import('./ball.js').Ball} Ball */

import {angle_between, distance_to_ball, distance_to_goal} from "./utils";

/**
 * @param {Robot[]} robots
 * @param {Ball} ball
 * @param {HTMLCanvasElement} canvas
 */
export function strategy(robots, ball, canvas) {
    select_roles(robots, ball, canvas);
}

/**
 * @param {Robot[]} robots
 * @param {Ball} ball
 * @param {HTMLCanvasElement} canvas
 */
function select_roles (robots, ball, canvas) {

    for (let robot of robots) {
        robot.defender_cost = robot_defender_cost(robot, canvas);
        robot.attacker_cost = robot_attacker_cost(robot, canvas);

        console.log(`Robot with id: ${robot.id}: attack: ${robot.attacker_cost} - defend: ${robot.defender_cost}`);
    }
}

/**
 * @param {Robot} robot
 * @param {HTMLCanvasElement} canvas
 */
function robot_defender_cost (robot, canvas) {
    const distance_to_goal_weight = 0.6;
    const hysteresis_weight = 0.4;

    // Distance to goal
    // < 5cm -> 100%
    // > 100cm -> 0%
    const distance = distance_to_goal(robot, canvas);
    let distance_score = (distance - 5) / (100 - 5);
    distance_score = Math.max(0, Math.min(1, distance_score));

    // Hysteresis bonus
    // role == 'defender'
    // role != 'defender
    const hysteresis_score = (robot.role === 'defender') ? 0 : 1;

    return (distance_score * distance_to_goal_weight) +
        (hysteresis_score * hysteresis_weight);
}

/**
 * @param {Robot} robot
 * @param {Ball} ball
 */
function robot_attacker_cost (robot, ball) {
    const distance_to_ball_weight = 0.5;
    const angle_to_ball_weight = 0.3;
    const hysteresis_weight = 0.2;

    // Distance to ball
    // < 5cm -> 100%
    // > 100 cm -> 0%
    const distance = distance_to_ball(robot, ball);
    let distance_score = (distance - 5) / (100 - 5);
    distance_score = Math.max(0, Math.min(1, distance_score));

    // Angle to ball
    // 0° -> 100%
    // 180/-180 -> 0%
    const angle = angle_between(robot, ball);
    const angle_score = Math.abs(angle) / Math.PI;

    // Hysteresis bonus
    // role == 'attacker' -> 100%
    // role != 'attacker' -> 0%;
    const hysteresis_score = (robot.role === 'attacker') ? 0 : 1;

    return (distance_score * distance_to_ball_weight) +
        (angle_score * angle_to_ball_weight) +
        (hysteresis_score * hysteresis_weight);
}

