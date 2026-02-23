/** @typedef {import('./robot.js').Robot} Robot */
/** @typedef {import('./ball.js').Ball} Ball */

import * as constants from './constants.js';
import {angle_between} from "./utils.js";
import * as roles from './roles.js';
/**
 * @param {Robot[]} robots
 * @param {Ball} ball
 * @param {HTMLCanvasElement} canvas
 * @param {'left' | 'right'} side
 */
export function strategy(robots, ball, canvas, side) {
    roles.select(robots, ball, canvas, side);

    const attacker = roles.get_attacker(robots);
    const defender = roles.get_defender(robots);

    if (attacker) {
        let move_angle = angle_between(attacker, ball) * 180 / Math.PI;

        if (Math.abs(move_angle) >= constants.ANGLE_THRESHOLD) {
            const negative = move_angle < 0;

            move_angle += constants.ANGLE_OFFSET * (negative ? -1 : 1);
        }
        attacker.move(constants.BASE_SPEED, move_angle);
    }

    if (defender) {
        const { x, y } = defender.get_position();
        const canvas_midpoint = (canvas.height / constants.SCALE) / 2;
        const field_width = canvas.width / constants.SCALE;

        let dx = 0;
        let dy = 0;

        if (side === 'left') {
            if (x > constants.GOAL.RIGHT_PADDING) {
                dx = -1;
            } else if (x < constants.GOAL.LEFT_PADDING) {
                dx = 1;
            }
        } else {
            if (x < (field_width - constants.GOAL.RIGHT_PADDING)) {
                dx = 1;
            } else if (x > (field_width - constants.GOAL.LEFT_PADDING)) {
                dx = -1;
            }
        }

        if (y < (canvas_midpoint - constants.GOAL.MIDPOINT_OFFSET)) {
            dy = 1;
        } else if (y > (canvas_midpoint + constants.GOAL.MIDPOINT_OFFSET)) {
            dy = -1;
        }

        if (dx !== 0 || dy !== 0) {
            const move_angle = Math.atan2(dy, dx) * 180 / Math.PI;
            defender.move(constants.BASE_SPEED, move_angle);
        }
    }
}