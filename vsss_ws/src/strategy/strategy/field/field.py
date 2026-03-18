from .. import constants


def draw_visual_circle(canvas, draw_context, x_cm, y_cm, color):
    x = constants.LEFT_PADDING + x_cm * constants.DISPLAY_SCALE
    y = constants.UPPER_PADDING + y_cm * constants.DISPLAY_SCALE
    r = constants.VISUAL_CIRCLE_RADIUS * constants.DISPLAY_SCALE
    bbox = [x - r, y - r, x + r, y + r]

    # Tkinter
    canvas.create_oval(*bbox, fill=color, outline="")
    # PIL
    draw_context.ellipse(bbox, fill=color, outline=None)


def draw_visual_cross(canvas, draw_context, x_cm, y_cm, color):
    x = constants.LEFT_PADDING + x_cm * constants.DISPLAY_SCALE
    y = constants.UPPER_PADDING + y_cm * constants.DISPLAY_SCALE
    size = (constants.VISUAL_CROSS_SIZE * constants.DISPLAY_SCALE) / 2
    coords = [x - size, y - size, x + size, y + size]
    coords2 = [x + size, y - size, x - size, y + size]

    # Tkinter
    canvas.create_line(*coords, fill=color, width=2)
    canvas.create_line(*coords2, fill=color, width=2)
    # PIL
    draw_context.line(coords, fill=color, width=2)
    draw_context.line(coords2, fill=color, width=2)


def draw_field(canvas, draw_context, width, height):
    # Field border
    rect_field = [
        constants.LEFT_PADDING,
        constants.UPPER_PADDING,
        constants.LEFT_PADDING + width,
        constants.UPPER_PADDING + height
    ]
    canvas.create_rectangle(*rect_field, fill="#000000", outline="white", width=2)
    draw_context.rectangle(rect_field, fill="#000000", outline="white", width=2)

    # Top Left Corner
    corner_tl_x1 = constants.LEFT_PADDING + constants.DISPLAY_CORNER_LENGTH * constants.DISPLAY_SCALE
    corner_tl_y1 = constants.UPPER_PADDING
    corner_tl_x2 = constants.LEFT_PADDING
    corner_tl_y2 = constants.UPPER_PADDING + constants.DISPLAY_CORNER_LENGTH * constants.DISPLAY_SCALE
    draw_context.line([(corner_tl_x1, corner_tl_y1), (corner_tl_x2, corner_tl_y2)], fill="white", width=2)
    canvas.create_line(corner_tl_x1, corner_tl_y1, corner_tl_x2, corner_tl_y2, fill="white", width=2)

    # Top Right Corner
    corner_tr_x1 = (constants.LEFT_PADDING
                    + constants.DISPLAY_FIELD_WIDTH * constants.DISPLAY_SCALE
                    - constants.DISPLAY_CORNER_LENGTH * constants.DISPLAY_SCALE)
    corner_tr_y1 = constants.UPPER_PADDING
    corner_tr_x2 = constants.LEFT_PADDING + constants.DISPLAY_FIELD_WIDTH * constants.DISPLAY_SCALE
    corner_tr_y2 = constants.UPPER_PADDING + constants.DISPLAY_CORNER_LENGTH * constants.DISPLAY_SCALE
    draw_context.line([(corner_tr_x1, corner_tr_y1), (corner_tr_x2, corner_tr_y2)], fill="white", width=2)
    canvas.create_line(corner_tr_x1, corner_tr_y1, corner_tr_x2, corner_tr_y2, fill="white", width=2)

    # Bottom Left Corner
    corner_bl_x1 = constants.LEFT_PADDING + constants.DISPLAY_CORNER_LENGTH * constants.DISPLAY_SCALE
    corner_bl_y1 = constants.UPPER_PADDING + constants.DISPLAY_FIELD_HEIGHT * constants.DISPLAY_SCALE
    corner_bl_x2 = constants.LEFT_PADDING
    corner_bl_y2 = (constants.UPPER_PADDING
                    + constants.DISPLAY_FIELD_HEIGHT * constants.DISPLAY_SCALE
                    - constants.DISPLAY_CORNER_LENGTH * constants.DISPLAY_SCALE)

    draw_context.line([(corner_bl_x1, corner_bl_y1), (corner_bl_x2, corner_bl_y2)], fill="white", width=2)
    canvas.create_line(corner_bl_x1, corner_bl_y1, corner_bl_x2, corner_bl_y2, fill="white", width=2)

    # Bottom Right Corner
    corner_br_x1 = (constants.LEFT_PADDING
                    + constants.DISPLAY_FIELD_WIDTH * constants.DISPLAY_SCALE
                    - constants.DISPLAY_CORNER_LENGTH * constants.DISPLAY_SCALE)
    corner_br_y1 = constants.UPPER_PADDING + constants.DISPLAY_FIELD_HEIGHT * constants.DISPLAY_SCALE
    corner_br_x2 = constants.LEFT_PADDING + constants.DISPLAY_FIELD_WIDTH * constants.DISPLAY_SCALE
    corner_br_y2 = (constants.UPPER_PADDING
                    + constants.DISPLAY_FIELD_HEIGHT * constants.DISPLAY_SCALE
                    - constants.DISPLAY_CORNER_LENGTH * constants.DISPLAY_SCALE)

    draw_context.line([(corner_br_x1, corner_br_y1), (corner_br_x2, corner_br_y2)], fill="white", width=2)
    canvas.create_line(corner_br_x1, corner_br_y1, corner_br_x2, corner_br_y2, fill="white", width=2)

    # Center line
    line_center = [
        constants.LEFT_PADDING + width / 2,
        constants.UPPER_PADDING,
        constants.LEFT_PADDING + width / 2,
        constants.UPPER_PADDING + height
    ]
    canvas.create_line(*line_center, fill="white", width=2)
    draw_context.line(line_center, fill="white", width=2)

    # Central circle
    r_center = 20 * constants.DISPLAY_SCALE
    cx, cy = constants.LEFT_PADDING + width / 2, constants.UPPER_PADDING + height / 2
    bbox_center = [cx - r_center, cy - r_center, cx + r_center, cy + r_center]

    canvas.create_oval(*bbox_center, outline="white", width=2)
    draw_context.ellipse(bbox_center, outline="white", width=2)

    # Areas
    area_depth = 15 * constants.DISPLAY_SCALE
    area_height = 70 * constants.DISPLAY_SCALE
    y_start = constants.UPPER_PADDING + height / 2 - area_height / 2
    y_end = constants.UPPER_PADDING + height / 2 + area_height / 2

    # Left area
    rect_area_l = [constants.LEFT_PADDING, y_start, constants.LEFT_PADDING + area_depth, y_end]
    canvas.create_rectangle(*rect_area_l, outline="white", width=2)
    draw_context.rectangle(rect_area_l, outline="white", width=2)

    # Right area
    rect_area_r = [
        constants.LEFT_PADDING + width - area_depth, y_start, constants.LEFT_PADDING + width, y_end]
    canvas.create_rectangle(*rect_area_r, outline="white", width=2)
    draw_context.rectangle(rect_area_r, outline="white", width=2)

    # Left goal
    rect_goal_l = [
        constants.LEFT_PADDING - constants.DISPLAY_GOAL_DEPTH,
        constants.DISPLAY_GOAL_WIDTH / 2 + height / 2,
        constants.LEFT_PADDING,
        -constants.DISPLAY_GOAL_WIDTH / 2 + height / 2
    ]
    canvas.create_rectangle(*rect_goal_l, outline="white", width=2)
    draw_context.rectangle(rect_goal_l, outline="white", width=2)

    # Right goal
    rect_goal_r = [
        constants.LEFT_PADDING + width + constants.DISPLAY_GOAL_DEPTH,
        constants.DISPLAY_GOAL_WIDTH / 2 + height / 2,
        constants.LEFT_PADDING + width,
        -constants.DISPLAY_GOAL_WIDTH / 2 + height / 2
    ]
    canvas.create_rectangle(*rect_goal_r, outline="white", width=2)
    draw_context.rectangle(rect_goal_r, outline="white", width=2)

    # Visual marks
    circles = [(16, 105), (16, 25), (56, 105), (56, 25), (134, 25), (134, 105), (94, 105), (94, 25)]
    crosses = [(38, 105), (38, 65), (38, 25), (112, 105), (112, 65), (112, 25)]

    for x_c, y_c in circles:
        draw_visual_circle(canvas, draw_context, x_c, y_c, "white")

    for x_c, y_c in crosses:
        draw_visual_cross(canvas, draw_context, x_c, y_c, "white")