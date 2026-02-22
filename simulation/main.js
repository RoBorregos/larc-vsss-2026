import * as constants from './constants.js';
import {Robot} from './robot.js';
import {ball} from './ball.js'
import {drawField} from "./field.js";
import {collision} from "./collisions.js";
import {strategy} from "./strategy.js";

const canvas = document.getElementById("field");
const ctx = canvas.getContext("2d");

canvas.width  = constants.FIELD_LENGTH * constants.SCALE;
canvas.height = constants.FIELD_WIDTH  * constants.SCALE;

const robot1 = new Robot(10, 65, 0, 'blue');
const robot2 = new Robot(34, 25, 1, 'green');
const robot3 = new Robot(50, 105, 2, 'red');

canvas.addEventListener('mousedown', (event) => {
    const rect = canvas.getBoundingClientRect();
    const mouseX = event.clientX - rect.left;
    const mouseY = event.clientY - rect.top;
    const worldX = mouseX / constants.SCALE;
    const worldY = mouseY / constants.SCALE;
    ball.set_position(worldX, worldY);
    ball.vx = 0;
    ball.vy = 0;
});

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

    strategy([robot1, robot2, robot3], ball, canvas, 'left');

    ball.draw(ctx);
    robot1.draw(ctx);
    robot2.draw(ctx);
    robot3.draw(ctx);

    requestAnimationFrame(loop);
}

// window.addEventListener('keydown', (event) => {
//     if (event.key === 'ArrowRight') {
//         loop();
//     }
// });

loop();