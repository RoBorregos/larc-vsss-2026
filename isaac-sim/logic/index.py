from isaaclab.app import AppLauncher

app_launcher = AppLauncher(headless=False)
simulation_app = app_launcher.app

from isaacsim.core.api import World
from isaacsim.core.utils.stage import add_reference_to_stage
from isaacsim.core.prims import Articulation
from logic.robot import Robot
import time
import torch

world = World(stage_units_in_meters=1.0)
world.scene.add_default_ground_plane()

USD_PATH = "/workspace/project/onshape-to-robot/omnirobot/omnirobot.usd"
robot = Robot(
    usd_path=USD_PATH,
    prim_path="/World/Omnirobot",
    spawn_orientation_euler_deg=(-90.0, 0.0, 0.0)
)

robot.spawn()

world.reset()
robot.initialize()
robot.configure_drive_modes()
robot.configure_motors()

start_time = time.time()

print("¡Iniciando secuencia en bucle: Frontal -> Izquierda -> Trasera -> Derecha -> Detener!")

while simulation_app.is_running():
    now = time.time()
    elapsed = now - start_time
    cycle_time = elapsed % 10.0

    # 1. RESETEAR SEÑALES (por defecto todas entran en Coast mode / 0.0)
    robot.set_throttle_front(0.0)
    robot.set_throttle_left(0.0)
    robot.set_throttle_back(0.0)
    robot.set_throttle_right(0.0)

    # 2. ACTIVAR LA LLANTA CORRESPONDIENTE SEGÚN LA FASE DEL CICLO
    if cycle_time < 2.0:
        # De 0 a 2 segundos: Llanta Frontal
        robot.move_omnidirectional(1.0, 0)

    elif cycle_time < 4.0:
        # De 2 a 4 segundos: Llanta Izquierda
        robot.move_omnidirectional(1.0, 90)

    elif cycle_time < 6.0:
        # De 4 a 6 segundos: Llanta Trasera
        robot.move_omnidirectional(1.0, 180)

    elif cycle_time < 8.0:
        # De 6 a 8 segundos: Llanta Derecha
        robot.move_omnidirectional(1.0, 270)

    else:
        # De 8 a 10 segundos: Detener (todas quedan en 0.0 por el reset de arriba)
        pass

    # 3. ENVIAR LOS COMANDOS ACUMULADOS
    robot.commit_motor_commands(debug=True)

    # 4. AVANZAR EL SIMULADOR
    world.step(render=True)

simulation_app.close()
