import rclpy
import tkinter as tk
import math
import threading  # Necesario para el hilo de ROS
from . import constants
from strategy.field import field
from PIL import Image, ImageDraw
from .logic.strategy import strategy
from .logic.object_handler import ObjectHandler
from .ros_handler import RosHandler
from .logic.utils import *

simulation = True
team = "YELLOW"


def on_canvas_click(event, ball):
    # Asegúrate de que world_to_pixel o la lógica de conversión sea correcta
    x, y = ball.world_to_pixel(event.x, event.y)
    print(f"Clic detectado en Canvas: x={event.x}, y={event.y}")
    print(f"Coordenadas reales: x={x}, y={y}")
    ball.real_x = x
    ball.real_y = y
    ball.real_vx = 0
    ball.real_vy = 0


def main():
    rclpy.init()

    root = tk.Tk()
    root.title("VSSS Simulator - Threaded ROS Mode")

    canvas_w = constants.CANVAS_WIDTH * constants.SCALE
    canvas_h = constants.CANVAS_HEIGHT * constants.SCALE

    canvas = tk.Canvas(root, width=canvas_w, height=canvas_h, bg="black")
    canvas.pack()

    image_buffer = Image.new("RGB", (int(canvas_w), int(canvas_h)), "black")
    draw_context = ImageDraw.Draw(image_buffer)

    infield_objects = [
        {"id": 20, "x": 0, "y": 0, "theta": 0},
        {"id": 1, "x": -0.3, "y": 0.3, "theta": math.pi / 2},
        {"id": 3, "x": -0.4, "y": 0.4, "theta": math.pi / 2},
        {"id": 4, "x": -0.3, "y": -0.3, "theta": math.pi / 2},
        {"id": 12, "x": 0.3, "y": 0.3, "theta": -math.pi / 2},
        {"id": 17, "x": 0.4, "y": 0, "theta": -math.pi / 2},
        {"id": 18, "x": 0.3, "y": -0.3, "theta": -math.pi / 2},
    ]

    ros_handler = RosHandler(infield_objects)
    object_handler = ObjectHandler(infield_objects, ros_handler, simulation)

    executor = rclpy.executors.MultiThreadedExecutor()
    executor.add_node(ros_handler)

    ros_thread = threading.Thread(target=executor.spin, daemon=True)
    ros_thread.start()

    last_pub_time = millis()
    pub_hz = 10

    root.bind("<Button-1>", lambda event: on_canvas_click(event, object_handler.get_ball()))

    def loop():
        nonlocal last_pub_time
        current_time = millis()

        field.draw_field(
            canvas,
            draw_context,
            constants.FIELD_WIDTH * constants.SCALE,
            constants.FIELD_HEIGHT * constants.SCALE
        )

        team_robots = object_handler.get_yellow_robots() if team == "YELLOW" else object_handler.get_blue_robots()
        enemy_robots = object_handler.get_blue_robots() if team == "YELLOW" else object_handler.get_yellow_robots()

        strategy(object_handler.get_ball(), team_robots, enemy_robots)

        object_handler.update()
        object_handler.draw(canvas, draw_context)

        if current_time - last_pub_time >= (1000 / pub_hz):
            cv_image = get_frame_from_buffer(image_buffer, scale_factor=0.5)
            ros_handler.publish_image(cv_image)
            last_pub_time = current_time

        root.after(10, loop)

    loop()
    root.mainloop()

    # Al cerrar la ventana
    rclpy.shutdown()


if __name__ == "__main__":
    main()