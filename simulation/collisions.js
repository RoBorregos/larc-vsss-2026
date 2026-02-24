import * as constants from './constants.js';

export function collision(robot, ball) {
    const dx = ball.x - robot.x;
    const dy = ball.y - robot.y;
    const dist = Math.sqrt(dx * dx + dy * dy);

    if (dist < constants.ROBOT_RADIUS + constants.BALL_RADIUS) {
        let nx;
        let ny;

        if (dist === 0) {
            // Degenerate case: ball center exactly matches robot center.
            // Use ball's current velocity direction if available, otherwise a fixed axis.
            if (ball.vx !== 0 || ball.vy !== 0) {
                const speed = Math.sqrt(ball.vx * ball.vx + ball.vy * ball.vy) || 1;
                nx = ball.vx / speed;
                ny = ball.vy / speed;
            } else {
                nx = 1;
                ny = 0;
            }
        } else {
            nx = dx / dist;
            ny = dy / dist;
        }
        ball.vx = nx * constants.PUSH_FORCE;
        ball.vy = ny * constants.PUSH_FORCE;
    }
}
