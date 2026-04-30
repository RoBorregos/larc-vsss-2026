import argparse
from isaaclab.app import AppLauncher

parser = argparse.ArgumentParser(description="VSSS Test")
AppLauncher.add_app_launcher_args(parser)
args_cli = parser.parse_args()

app_launcher = AppLauncher(args_cli)
simulation_app = app_launcher.app

import isaaclab.sim as sim_utils
from isaaclab.sim import SimulationContext
from robot import Robot

print("Simulator starting, setting up everything...")

sim = SimulationContext()
sim.set_camera_view([2.0, 2.0, 2.0], [0.0, 0.0, 0.0])

cfg_light = sim_utils.DistantLightCfg(intensity=3000.0, color=(1.0, 1.0, 1.0))
cfg_light.func("/World/Light", cfg_light, translation=(1, 0, 10))

cfg_ground = sim_utils.GroundPlaneCfg()
cfg_ground.func("/World/GroundPlane", cfg_ground)

robot_blue = Robot("/World/Robot_blue", "Blue1", color=(0.0, 0.0, 1.0))

sim.reset()

while simulation_app.is_running():
    robot_blue.apply_forward_thrust(5.0)
    sim.step(render=True)

simulation_app.close()
