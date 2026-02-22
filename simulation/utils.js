/** @typedef {import('./robot.js').Robot} Robot */
/** @typedef {import('./ball.js').Ball} Ball */
import * as constants from './constants.js';

/**
 * Calculates the Euclidean distance between a robot and the ball.
 * @param {import('./robot.js').Robot} robot
 * @param {import('./ball.js').Ball} ball
 * @returns {number} The distance in centimeters.
 */
export function distance_to_ball(robot, ball) {
    const {x: rx, y: ry} = robot.get_position();
    const {x: bx, y: by} = ball.get_position();

    const dx = bx - rx;
    const dy = by - ry;

    return Math.hypot(dx, dy);
}

/**
 * @param {Robot} robot
 * @param {HTMLCanvasElement} canvas
 * @param {'left' | 'right'} side
 */
export function distance_to_goal(robot, canvas, side) {
    const {x: rx, y: ry} = robot.get_position();

    const targetY_cm = (canvas.height / 2) / constants.SCALE;
    let targetX_cm;

    if (side === 'left') {
        targetX_cm = 0;
    } else if (side === 'right') {
        targetX_cm = canvas.width / constants.SCALE;
    } else {
        throw new Error('Unknown side');
    }

    const dx = targetX_cm - rx;
    const dy = targetY_cm - ry;

    return Math.hypot(dx, dy);
}

/**
 * Calculates the angle from the robot to the ball.
 * @param {import('./robot.js').Robot} robot
 * @param {import('./ball.js').Ball} ball
 * @returns {number} Angle in radians between -Math.PI and Math.PI (-180 to 180 degrees).
 */
export function angle_between(robot, ball) {
    const {x: rx, y: ry} = robot.get_position();
    const {x: bx, y: by} = ball.get_position();

    const dx = bx - rx;
    const dy = by - ry;

    return Math.atan2(dy, dx);
}
