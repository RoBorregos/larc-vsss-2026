import * as constants from './constants.js';

export const ball = {
    x: 75,
    y: 65,
    vx: 0,
    vy: 0,

    get_position () {
        const x = this.x;
        const y = this.y;

        return {x, y};
    },

    update() {
        this.x += this.vx * constants.DT;
        this.y += this.vy * constants.DT;

        this.vx *= constants.FRICTION;
        this.vy *= constants.FRICTION;

        if (this.x < constants.BALL_RADIUS || this.x > constants.FIELD_LENGTH - constants.BALL_RADIUS) {
            this.vx *= -1;
            this.x = this.x < constants.BALL_RADIUS ? constants.BALL_RADIUS : constants.FIELD_LENGTH - constants.BALL_RADIUS;
        }

        if (this.y < constants.BALL_RADIUS || this.y > constants.FIELD_WIDTH - constants.BALL_RADIUS) {
            this.vy *= -1;
            this.y = this.y < constants.BALL_RADIUS ? constants.BALL_RADIUS : constants.FIELD_WIDTH - constants.BALL_RADIUS;
        }
    },

    draw(ctx) {
        ctx.beginPath();
        ctx.arc(
            this.x * constants.SCALE,
            this.y * constants.SCALE,
            constants.BALL_RADIUS * constants.SCALE,
            0,
            2 * Math.PI
        );
        ctx.fillStyle = "orange";
        ctx.fill();
        ctx.closePath();
    }
};