export const DT = 0.016;
export const FRICTION = 0.995;

export const ROBOT_RADIUS = 3.75;
export const BALL_RADIUS = 2.135;

export const FIELD_LENGTH = 150;
export const FIELD_WIDTH  = 130;

export const MID_X = FIELD_LENGTH / 2;

export const DEFENDER_OFFSET_X = 25;
export const DEFENDER_OFFSET_Y = 0;

export const DEFENDER_MAX_FORWARD = MID_X + 5;

export const VISUAL_CIRCLE_DIAMETER = 2.5;
export const VISUAL_CIRCLE_RADIUS = VISUAL_CIRCLE_DIAMETER/2;
export const VISUAL_CROSS_SIZE = 3.5;

export const SCALE = 5;

export const START_POSITION = {
    robot1: {x: 10, y: 65, id: 0, color: 'blue'},
    robot2: {x: 34, y: 25, id: 1, color: 'green'},
    robot3: {x: 50, y: 105, id: 2, color: 'red'},
    ball: {x: 75, y: 65}
}

export const ANGLE_THRESHOLD = 15;
export const ANGLE_OFFSET = 40;
export const BASE_SPEED = 15;

export const LEFT = 180;
export const RIGHT = 0;
export const DOWN = 90
export const UP = 270;

export const GOAL = {
    LEFT_PADDING: 7,
    RIGHT_PADDING: 15,
    MIDPOINT_OFFSET: 10
}

export const PUSH_FORCE = 60;