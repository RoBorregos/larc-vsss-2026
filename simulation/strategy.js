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

        if (side === 'left') {
            if (x > constants.GOAL.RIGHT_PADDING) {
                defender.move(constants.BASE_SPEED, constants.LEFT);
            } else if (x < constants.GOAL.LEFT_PADDING) {
                defender.move(constants.BASE_SPEED, constants.RIGHT);
            }
        } else {
            if (x < (field_width - constants.GOAL.RIGHT_PADDING)) {
                defender.move(constants.BASE_SPEED, constants.RIGHT);
            } else if (x > (field_width - constants.GOAL.LEFT_PADDING)) {
                defender.move(constants.BASE_SPEED, constants.LEFT);
            }
        }

        if (y < (canvas_midpoint - constants.GOAL.MIDPOINT_OFFSET)) {
            defender.move(constants.BASE_SPEED, constants.DOWN);
        } else if (y > (canvas_midpoint + constants.GOAL.MIDPOINT_OFFSET)) {
            defender.move(constants.BASE_SPEED, constants.UP);
        }
    }
}

