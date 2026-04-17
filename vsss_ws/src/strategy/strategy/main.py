import rclpy
import tkinter as tk
import threading

from breezy.doc_generate.conf import project

from . import constants
from strategy.field import field
from PIL import Image, ImageDraw
from .logic.strategy import strategy
from .logic.enemy_strategy import enemy_strategy
from .logic.object_handler import ObjectHandler
from .ros_handler import RosHandler
from .logic.utils import *
from .logic import path_planning
from .ui_sidebar import TeamSidebar

simulation = False

# Window chrome slightly off the pitch black so the field reads as one surface
CHROME_BG = "#0a0a0a"


def on_canvas_click(event, ball, team_robots):
    x, y = ball.world_to_pixel(event.x, event.y)
    ball.real_x = x
    ball.real_y = y
    ball.real_vx = 0
    ball.real_vy = 0

    for robot in team_robots:
        robot.stop()

def main():
    rclpy.init()

    root = tk.Tk()
    root.title("VSSS Simulator")
    root.configure(bg=CHROME_BG)

    strategy_stopped = tk.BooleanVar(master=root, value=False)

    workspace = tk.Frame(root, bg=CHROME_BG)
    workspace.pack(fill=tk.BOTH, expand=True)

    sidebar = TeamSidebar(workspace, initial_team="YELLOW", run_var=strategy_stopped)
    sidebar.pack(side=tk.LEFT, fill=tk.Y, padx=(10, 0), pady=10)

    field_column = tk.Frame(workspace, bg=CHROME_BG)
    field_column.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=10, pady=10)

    canvas_w = constants.DISPLAY_CANVAS_WIDTH * constants.DISPLAY_SCALE
    canvas_h = constants.DISPLAY_CANVAS_HEIGHT * constants.DISPLAY_SCALE

    canvas = tk.Canvas(
        field_column,
        width=canvas_w,
        height=canvas_h,
        bg="black",
        highlightthickness=1,
        highlightbackground="#2a2a2a",
    )
    canvas.pack()

    image_buffer = Image.new("RGB", (int(canvas_w), int(canvas_h)), "black")
    draw_context = ImageDraw.Draw(image_buffer)

    infield_objects = [ # Legacy structure because of time
        {"id": 20, "x": 0, "y": 0, "theta": 0},
        {"id": 0, "x": -0.3, "y": 0.3, "theta": 0},
        {"id": 1, "x": -0.4, "y": 0.4, "theta": 0},
        {"id": 2, "x": -0.3, "y": -0.3, "theta": 0},
        {"id": 10, "x": 0.3, "y": 0.3, "theta": 0},
        {"id": 11, "x": 0.6, "y": 0, "theta": 0},
        {"id": 12, "x": 0.3, "y": -0.3, "theta": 0},
    ]

    ros_handler = RosHandler(infield_objects)
    object_handler = ObjectHandler(infield_objects, ros_handler, simulation)

    executor = rclpy.executors.MultiThreadedExecutor()
    executor.add_node(ros_handler)

    ros_thread = threading.Thread(target=executor.spin, daemon=True)
    ros_thread.start()

    last_pub_time = millis()
    pub_hz = 10

    def handle_field_click(event):
        team_robots = (
            object_handler.get_yellow_robots()
            if sidebar.team == "YELLOW"
            else object_handler.get_blue_robots()
        )
        if sidebar.team == "YELLOW":
            constants.ATTACKING_RIGHT = True
        else:
            constants.ATTACKING_RIGHT = False

        on_canvas_click(event, object_handler.get_ball(), team_robots)

    canvas.bind("<Button-1>", handle_field_click)

    def loop():
        nonlocal last_pub_time
        current_time = millis()

        if not object_handler.in_goal_callback:
            canvas.delete("all")
            image_buffer.paste("black", [0, 0, int(canvas_w), int(canvas_h)])
            field.draw_field(
                canvas,
                draw_context,
                constants.DISPLAY_FIELD_WIDTH * constants.DISPLAY_SCALE,
                constants.DISPLAY_FIELD_HEIGHT * constants.DISPLAY_SCALE
            )

        team_robots = (
            object_handler.get_yellow_robots()
            if sidebar.team == "YELLOW"
            else object_handler.get_blue_robots()
        )
        enemy_robots = (
            object_handler.get_blue_robots()
            if sidebar.team == "YELLOW"
            else object_handler.get_yellow_robots()
        )
        ball = object_handler.get_ball()

        if strategy_stopped.get():
            object_handler.stop_robots()
        else:
            strategy(ball, team_robots, enemy_robots)
            if simulation:
                enemy_strategy(ball, enemy_robots)

        for robot in team_robots:
            path_planning.repulsion(robot, robot, constants.TEAM_INFLUENCE_RADIUS, canvas)
        for robot in enemy_robots:
            path_planning.repulsion(robot, robot, constants.INFLUENCE_RADIUS, canvas)
        path_planning.repulsion(ball, ball, constants.BALL_INFLUENCE_RADIUS, canvas)

        object_handler.update(canvas, simulation)

        object_handler.draw(canvas, draw_context, simulation)

        if (
            current_time - last_pub_time >= (1000 / pub_hz)
            and simulation
        ):
            cv_image = get_frame_from_buffer(image_buffer, scale_factor=0.5)
            ros_handler.publish_image(cv_image)
            last_pub_time = current_time

        root.after(20, loop)


    loop()
    root.mainloop()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
