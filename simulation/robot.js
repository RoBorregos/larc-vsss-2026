import * as constants from './constants.js';

export class Robot {
    attacker_cost;
    defender_cost;
    constructor({x, y, id, color}) {
        this.id = id;
        this.x = x;
        this.y = y;
        this.color = color;
        this.theta = 0;
        this.vx = 0;
        this.vy = 0;
        this.w = 0;
        this.role = 'unset';
    }

    get_position () {
        const x = this.x;
        const y = this.y;

        return {x, y};
    }

    /**
     * Sets the robot's velocity based on a speed and a direction angle.
     * @param {number} speed - Linear speed in cm/s.
     * @param {number} angle - Direction of movement in radians.
     */
    move(speed, angle) {
        angle *= Math.PI/ 180;
        this.vx = speed * Math.cos(angle);
        this.vy = speed * Math.sin(angle);
    }

    /**
     * Updates the robot's position and handles physics like friction and boundaries.
     */
    update() {
        this.x += this.vx * constants.DT;
        this.y += this.vy * constants.DT;

        this.theta = 0;
        this.w = 0;

        this.vx *= constants.FRICTION;
        this.vy *= constants.FRICTION;

        this.x = Math.max(constants.ROBOT_RADIUS, Math.min(constants.FIELD_LENGTH - constants.ROBOT_RADIUS, this.x));
        this.y = Math.max(constants.ROBOT_RADIUS, Math.min(constants.FIELD_WIDTH - constants.ROBOT_RADIUS, this.y));
    }

    /**
     * Renders the robot as a square with a heading line.
     * @param {CanvasRenderingContext2D} ctx - The drawing context.
     * @param {string} color - The fill color for the robot body.
     */
    draw(ctx) {
        ctx.save();

        ctx.translate(this.x * constants.SCALE, this.y * constants.SCALE);
        ctx.rotate(this.theta);

        const size = constants.ROBOT_RADIUS * 2 * constants.SCALE;
        const offset = -size / 2;

        ctx.fillStyle = this.color;
        ctx.fillRect(offset, offset, size, size);

        ctx.strokeStyle = "white";
        ctx.lineWidth = 3;
        ctx.beginPath();
        ctx.moveTo(0, 0);
        ctx.lineTo(constants.ROBOT_RADIUS * constants.SCALE, 0);
        ctx.stroke();

        ctx.restore();
    }
}