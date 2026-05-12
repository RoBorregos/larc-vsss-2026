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

print("=== Wheel joint mapping ===")
for slot, dof_idx in enumerate(robot.wheel_indexes.tolist()):
    joint_name = robot.dof_names[dof_idx]
    print(f"  throttle slot [{slot}] -> DOF index {dof_idx} -> joint name '{joint_name}'")
print(f"Total wheel joints found: {len(robot.wheel_indexes)}")

THROTTLE = 1
wheel_idx = 1

print(f"Driving {robot.dof_names[robot.wheel_indexes[wheel_idx]]} at throttle {THROTTLE}")

while simulation_app.is_running():
    now = time.time()

    throttle = torch.zeros(4, device=robot.device)
    throttle[wheel_idx] = THROTTLE
    robot.apply_motor_command(throttle)

    world.step(render=True)

simulation_app.close()