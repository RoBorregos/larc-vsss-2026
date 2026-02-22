import * as constants from './constants.js';
import {Robot} from './robot.js';
import {ball} from './ball.js'
import {drawField} from "./field.js";
import {collision} from "./collisions.js";
import {strategy} from "./strategy";

const canvas = document.getElementById("field");
const ctx = canvas.getContext("2d");

canvas.width  = constants.FIELD_LENGTH * constants.SCALE;
canvas.height = constants.FIELD_WIDTH  * constants.SCALE;

const robot1 = new Robot(10, 65, 0);
const robot2 = new Robot(34, 25, 1);
const robot3 = new Robot(50, 105, 2);

let angle_direction = 0;
let frame_id = 0;
function loop() {
    frame_id++;
    drawField(ctx, canvas);

    robot1.update();
    robot2.update();
    robot3.update();
    ball.update();

    collision(robot1, ball);
    collision(robot2, ball);
    collision(robot3, ball);

    strategy([robot1, robot2, robot3], ball, canvas);

    robot1.draw(ctx, "blue");
    robot2.draw(ctx, "red");
    robot3.draw(ctx, "green");
    ball.draw(ctx);

    requestAnimationFrame(loop);
}

loop();