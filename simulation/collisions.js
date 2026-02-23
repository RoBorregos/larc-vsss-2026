import * as constants from './constants.js';

export function collision(robot, ball) {
    const dx = ball.x - robot.x;
    const dy = ball.y - robot.y;
    const dist = Math.sqrt(dx * dx + dy * dy) || 0.001;

    if (dist < constants.ROBOT_RADIUS + constants.BALL_RADIUS) {
        const nx = dx / dist;
        const ny = dy / dist;

        ball.vx = nx * constants.PUSH_FORCE;
        ball.vy = ny * constants.PUSH_FORCE;
    }
}
