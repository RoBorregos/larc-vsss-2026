import * as constants from './constants.js';

function drawVisualCircle(ctx, x_cm, y_cm, color) {
    ctx.beginPath();
    ctx.arc(
        x_cm * constants.SCALE,
        y_cm * constants.SCALE,
        constants.VISUAL_CIRCLE_RADIUS * constants.SCALE,
        0,
        2 * Math.PI
    );
    ctx.fillStyle = color;
    ctx.fill();
}

function drawVisualCross(ctx, x_cm, y_cm, color) {
    const x = x_cm * constants.SCALE;
    const y = y_cm * constants.SCALE;
    const size = (constants.VISUAL_CROSS_SIZE * constants.SCALE) / 2;

    ctx.strokeStyle = color;
    ctx.lineWidth = 2;

    ctx.beginPath();
    ctx.moveTo(x - size, y - size);
    ctx.lineTo(x + size, y + size);
    ctx.moveTo(x + size, y - size);
    ctx.lineTo(x - size, y + size);
    ctx.stroke();
}

export function drawField(ctx, canvas) {
    ctx.clearRect(0, 0, canvas.width, canvas.height);

    ctx.fillStyle = "#000000";
    ctx.fillRect(0, 0, canvas.width, canvas.height);

    ctx.strokeStyle = "white";
    ctx.lineWidth = 2;

    ctx.strokeRect(0, 0, canvas.width, canvas.height);

    ctx.beginPath();
    ctx.moveTo(canvas.width / 2, 0);
    ctx.lineTo(canvas.width / 2, canvas.height);
    ctx.stroke();

    const CENTER_CIRCLE_RADIUS = 20 * constants.SCALE;
    ctx.beginPath();
    ctx.arc(canvas.width / 2, canvas.height / 2, CENTER_CIRCLE_RADIUS, 0, 2 * Math.PI);
    ctx.stroke();

    const AREA_DEPTH = 15 * constants.SCALE;
    const AREA_HEIGHT = 70 * constants.SCALE;

    ctx.strokeRect(0,
        canvas.height / 2 - AREA_HEIGHT / 2,
        AREA_DEPTH,
        AREA_HEIGHT);

    ctx.strokeRect(canvas.width - AREA_DEPTH,
        canvas.height / 2 - AREA_HEIGHT / 2,
        AREA_DEPTH,
        AREA_HEIGHT);

    drawVisualCircle(ctx, 16, 105, "white");
    drawVisualCircle(ctx, 16, 25, "white");
    drawVisualCircle(ctx, 56, 105, "white");
    drawVisualCircle(ctx, 56, 25, "white");
    drawVisualCircle(ctx, 134, 25, "white");
    drawVisualCircle(ctx, 134, 105, "white");
    drawVisualCircle(ctx, 94, 105, "white");
    drawVisualCircle(ctx, 94, 25, "white");

    drawVisualCross(ctx, 38, 105, "white");
    drawVisualCross(ctx, 38, 65, "white");
    drawVisualCross(ctx, 38, 25, "white");
    drawVisualCross(ctx, 112, 105, "white");
    drawVisualCross(ctx, 112, 65, "white");
    drawVisualCross(ctx, 112, 25, "white");
}