import threading
import tkinter as tk
import rclpy
from PIL import Image, ImageDraw

from strategy.field import field
from .constants import PhysicalProperties, VisualProperties
from . import settings
from .logic.strategy import strategy
from .logic.enemy_strategy import enemy_strategy
from .logic.object_handler import ObjectHandler
from .logic.utils import get_frame_from_buffer, millis
from .logic import path_planning
from .ros_handler import RosHandler
from .ui_sidebar import TeamSidebar

_FIELD = PhysicalProperties.Field
_BALL_ID = PhysicalProperties.Ball.ID
_DISPLAY = VisualProperties.Display

SIMULATION = True
CHROME_BG = "#0a0a0a"


def on_canvas_click(event, ball, team_robots) -> None:
    if ball is None:
        return

    x_mm, y_mm = ball.pixel_to_world(event.x, event.y)
    ball.real_x = x_mm
    ball.real_y = y_mm
    ball.real_vx = 0.0
    ball.real_vy = 0.0

    for robot in team_robots:
        robot.stop()


def main() -> None:
    rclpy.init()

    root = tk.Tk()
    root.title("VSSS Simulator")
    root.configure(bg=CHROME_BG)

    strategy_stopped = tk.BooleanVar(master=root, value=False)

    workspace = tk.Frame(root, bg=CHROME_BG)
    workspace.pack(fill=tk.BOTH, expand=True)

    sidebar = TeamSidebar(
        workspace,
        initial_team="YELLOW",
        initial_attacking_right=True,
        initial_move_enemies=True,
        run_var=strategy_stopped,
    )
    sidebar.pack(side=tk.LEFT, fill=tk.Y, padx=(10, 0), pady=10)

    field_column = tk.Frame(workspace, bg=CHROME_BG)
    field_column.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=10, pady=10)

    canvas_w = int(_FIELD.WIDTH_MM * _DISPLAY.SCALE_PX_PER_MM + 2 * _DISPLAY.LEFT_PADDING_PX)
    canvas_h = int(_FIELD.HEIGHT_MM * _DISPLAY.SCALE_PX_PER_MM + 2 * _DISPLAY.UPPER_PADDING_PX)

    canvas = tk.Canvas(
        field_column,
        width=canvas_w,
        height=canvas_h,
        bg="black",
        highlightthickness=1,
        highlightbackground="#2a2a2a",
    )
    canvas.pack()

    image_buffer = Image.new("RGB", (canvas_w, canvas_h), "black")
    draw_context = ImageDraw.Draw(image_buffer)

    # Posiciones iniciales en milímetros (mm) y radianes
    infield_objects = [
        {"id": _BALL_ID, "x": 0.0, "y": 0.0, "theta": 0.0},
        {"id": 0, "x": -300.0, "y": 300.0, "theta": 0.0},
        {"id": 1, "x": -400.0, "y": 400.0, "theta": 0.0},
        {"id": 2, "x": -300.0, "y": -300.0, "theta": 0.0},
        {"id": 10, "x": 300.0, "y": 300.0, "theta": 0.0},
        {"id": 11, "x": 600.0, "y": 0.0, "theta": 0.0},
        {"id": 12, "x": 300.0, "y": -300.0, "theta": 0.0},
    ]

    ros_handler = RosHandler(infield_objects)
    object_handler = ObjectHandler(infield_objects, ros_handler, SIMULATION)

    executor = rclpy.executors.MultiThreadedExecutor()
    executor.add_node(ros_handler)

    ros_thread = threading.Thread(target=executor.spin, daemon=True)
    ros_thread.start()

    last_pub_time = millis()
    pub_hz = 10

    def handle_field_click(event) -> None:
        team_robots = (
            object_handler.get_yellow_robots()
            if sidebar.team == "YELLOW"
            else object_handler.get_blue_robots()
        )
        on_canvas_click(event, object_handler.get_ball(), team_robots)

    canvas.bind("<Button-1>", handle_field_click)

    def loop() -> None:
        nonlocal last_pub_time
        current_time = millis()

        if not object_handler.in_goal_callback:
            canvas.delete("all")
            image_buffer.paste("black", [0, 0, canvas_w, canvas_h])
            field.draw_field(canvas, draw_context)

        is_yellow = sidebar.team == "YELLOW"
        settings.strategy.attacking_right = sidebar.attacking_right

        team_robots = object_handler.get_yellow_robots() if is_yellow else object_handler.get_blue_robots()
        enemy_robots = object_handler.get_blue_robots() if is_yellow else object_handler.get_yellow_robots()
        ball = object_handler.get_ball()

        if strategy_stopped.get():
            object_handler.stop_robots()
        elif ball is not None:
            strategy(ball, team_robots, enemy_robots)
            if SIMULATION:
                if sidebar.move_enemies:
                    enemy_side = "LEFT" if sidebar.attacking_right else "RIGHT"
                    enemy_strategy(ball, enemy_robots, side=enemy_side)
                else:
                    for robot in enemy_robots:
                        robot.stop()

        pp = settings.path_planning
        for robot in team_robots:
            path_planning.repulsion(robot, robot, pp.team_influence_radius_mm, canvas)
        for robot in enemy_robots:
            path_planning.repulsion(robot, robot, pp.influence_radius_mm, canvas)
        if ball is not None:
            path_planning.repulsion(ball, ball, pp.ball_influence_radius_mm, canvas)

        object_handler.update(canvas, SIMULATION)
        object_handler.draw(canvas, draw_context, SIMULATION)

        if SIMULATION and (current_time - last_pub_time >= (1000 / pub_hz)):
            cv_image = get_frame_from_buffer(image_buffer, scale_factor=0.5)
            ros_handler.publish_image(cv_image)
            last_pub_time = current_time

        root.after(20, loop)

    loop()
    root.mainloop()
    rclpy.shutdown()


if __name__ == "__main__":
    main()