import argparse
from isaaclab.app import AppLauncher

parser = argparse.ArgumentParser(description="VSSS Test")
AppLauncher.add_app_launcher_args(parser)
args_cli = parser.parse_args()

app_launcher = AppLauncher(args_cli)
simulation_app = app_launcher.app

import isaaclab.sim as sim_utils
from isaaclab.sim import SimulationContext

print("Simulator starting, setting up everything...")

sim = SimulationContext()
sim.set_camera_view([2.0, 2.0, 2.0], [0.0, 0.0, 0.0])

cfg_light = sim_utils.DistantLightCfg(intensity=3000.0, color=(1.0, 1.0, 1.0))
cfg_light.func("/World/Light", cfg_light, translation=(1, 0, 10))

cfg_ground = sim_utils.GroundPlaneCfg()
cfg_ground.func("/World/GroundPlane", cfg_ground)

cfg_ball = sim_utils.SphereCfg(
    radius=0.02135,
    rigid_props=sim_utils.RigidBodyPropertiesCfg(),
    mass_props=sim_utils.MassPropertiesCfg(mass=0.046),
    collision_props=sim_utils.CollisionPropertiesCfg(),
    visual_material=sim_utils.PreviewSurfaceCfg(diffuse_color=(1.0, 0.5, 0.0))
)

cfg_ball.func("/World/Ball", cfg_ball, translation=(0.0, 0.0, 0.5))

sim.reset()

print("Entering loop")

while simulation_app.is_running():
    sim.step(render=True)

simulation_app.close()
