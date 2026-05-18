import argparse
import math
import time

from isaaclab.app import AppLauncher

# ============================================================== #
# CLI
# ============================================================== #
parser = argparse.ArgumentParser(
    description="Massive simulation for VSSS.",
    formatter_class=argparse.ArgumentDefaultsHelpFormatter
)

sim_group = parser.add_argument_group('Configuración de Simulación')
sim_group.add_argument("--num_fields", type=int, default=2, help="Amount of fields to create.")
sim_group.add_argument("--grid_spacing", type=float, default=3.0, help="Distance in meters in between each field center.")

AppLauncher.add_app_launcher_args(parser)
args = parser.parse_args()

app_launcher = AppLauncher(args)
simulation_app = app_launcher.app

# ----- Imports post-launcher -----
from isaacsim.core.api import World
from pxr import UsdGeom, Gf, UsdPhysics, PhysxSchema
import omni.usd

from logic.field import Field
from logic.constants import config
from logic.team import Team
from logic.robot_fleet import RobotFleet
from logic.ball_fleet import BallFleet

# ============================================================== #
# WORLD AND STAGE
# ============================================================== #
world = World(stage_units_in_meters=1.0, backend="torch", device="cuda:0")
stage = omni.usd.get_context().get_stage()
world.scene.add_default_ground_plane()

# --- PhysX GPU buffers ---
target_capacity = max(16384, args.num_fields * 1536)  # ~1500 pairs per field

# Search (or create) the PhysicsScene in stage
physics_scene_path = "/physicsScene"
physics_scene = UsdPhysics.Scene.Get(stage, physics_scene_path)
if not physics_scene:
    physics_scene = UsdPhysics.Scene.Define(stage, physics_scene_path)

physx_scene_api = PhysxSchema.PhysxSceneAPI.Apply(physics_scene.GetPrim())
physx_scene_api.CreateGpuFoundLostAggregatePairsCapacityAttr(target_capacity)
physx_scene_api.CreateGpuFoundLostPairsCapacityAttr(target_capacity)
physx_scene_api.CreateGpuTotalAggregatePairsCapacityAttr(target_capacity)
physx_scene_api.CreateGpuCollisionStackSizeAttr(1024 * 1024 * 64)

# --- Buffers for narrow-phase collisions  ---
patch_capacity = max(163840, args.num_fields * 2000)
contact_capacity = max(524288, args.num_fields * 8192)

print(f"[INFO] Patch capacity set to {patch_capacity}")
print(f"[INFO] Contact capacity set to {contact_capacity}")

physx_scene_api.CreateGpuMaxRigidPatchCountAttr(patch_capacity)
physx_scene_api.CreateGpuMaxRigidContactCountAttr(contact_capacity)
# ---------------------------------------------------

NUM_FIELDS = args.num_fields
GRID_SPACING = args.grid_spacing
HEADLESS = args.headless
USD_PATH = config["robot"]["usd_path"]

cols = int(math.ceil(math.sqrt(NUM_FIELDS)))

# ============================================================== #
# ENVS + GLOBAL PATHS + TEAMS (views)
# ============================================================== #
teams_per_env: list[tuple[Team, Team]] = []

yellow_prim_paths, yellow_spawn_positions = [], []
blue_prim_paths, blue_spawn_positions = [], []
ball_prim_paths, ball_spawn_positions = [], []

start_time_envs = time.time()

for i in range(NUM_FIELDS):
    row, col = divmod(i, cols)
    x = (col - (cols - 1) / 2.0) * GRID_SPACING
    y = (row - (cols - 1) / 2.0) * GRID_SPACING

    env_path = f"/World/envs/env_{i}"
    env_xform = UsdGeom.Xform.Define(stage, env_path)
    UsdGeom.XformCommonAPI(env_xform).SetTranslate(Gf.Vec3d(x, y, 0.0))

    Field(parent_path=env_path, name="VSSS_Pitch")

    yellow_prim_paths += Team.build_prim_paths(env_path, "yellow")
    yellow_spawn_positions += Team.build_spawn_positions("yellow")
    blue_prim_paths += Team.build_prim_paths(env_path, "blue")
    blue_spawn_positions += Team.build_spawn_positions("blue")

    ball_prim_paths.append(f"{env_path}/ball")
    ball_spawn_positions.append((0.0, 0.0, 0.05))

    ty = Team(team_color="yellow", attacking="right", env_path=env_path, env_idx=i)
    tb = Team(team_color="blue", attacking="left", env_path=env_path, env_idx=i)
    teams_per_env.append((ty, tb))

    # --- Progress and ETA ---
    current = i + 1
    elapsed_time = time.time() - start_time_envs
    percentage = (current / NUM_FIELDS) * 100.0
    time_per_env = elapsed_time / current
    eta_seconds = time_per_env * (NUM_FIELDS - current)
    eta_mins, eta_secs = divmod(int(eta_seconds), 60)

    print(
        f"\r[INFO] Setting up fields: {current}/{NUM_FIELDS} [{percentage:.1f}%] - ETA: {eta_mins:02d}:{eta_secs:02d} - {env_path}" + " " * 15,
        end='', flush=True)

# Final resume
total_time_envs = time.time() - start_time_envs
tot_mins_envs, tot_secs_envs = divmod(int(total_time_envs), 60)
print(f"\n[INFO] {NUM_FIELDS} fields were set up in {tot_mins_envs:02d}:{tot_secs_envs:02d}.")

# ============================================================== #
# GLOBAL FLEETS
# ============================================================== #
fleet_yellow = RobotFleet(
    usd_path=USD_PATH,
    prim_paths=yellow_prim_paths,
    prim_paths_expr="/World/envs/env_.*/team/yellow/robot_.*",
    spawn_positions=yellow_spawn_positions,
    team_color="yellow",
    name="fleet_yellow",
    headless=HEADLESS
)

fleet_blue = RobotFleet(
    usd_path=USD_PATH,
    prim_paths=blue_prim_paths,
    prim_paths_expr="/World/envs/env_.*/team/blue/robot_.*",
    spawn_positions=blue_spawn_positions,
    team_color="blue",
    name="fleet_blue",
    headless=HEADLESS
)

ball_fleet = BallFleet(
    prim_paths=ball_prim_paths,
    prim_paths_expr="/World/envs/env_.*/ball",
    spawn_positions=ball_spawn_positions,
    name="ball_fleet",
)

fleet_yellow.spawn()
fleet_blue.spawn()
ball_fleet.spawn()

for ty, tb in teams_per_env:
    ty.attach_fleet(fleet_yellow)
    tb.attach_fleet(fleet_blue)

# ============================================================== #
# INIT
# ============================================================== #
world.reset()

for fleet in (fleet_yellow, fleet_blue):
    fleet.initialize()
    fleet.configure_drive_modes(wheel_damping=config["motors"]["wheel_damping"])
    fleet.configure_motors()
    fleet.paint()

ball_fleet.initialize()

# ============================================================== #
# LOOP — Square movement
# ============================================================== #
total_robots = NUM_FIELDS * Team.ROBOTS_PER_TEAM * 2
print(f"\n[INFO] Simulation started with {NUM_FIELDS} fields ({total_robots} robots + {NUM_FIELDS} balls).")
print(f"[INFO] PhysX capacity adjusted to: {target_capacity}\n")

STEPS_PER_SIDE = 90
BLUE_DIRS = [0.0, 90.0, 180.0, 270.0]
YELLOW_DIRS = [180.0, 270.0, 0.0, 90.0]
SPEED = 0.6
DURATION_PER_SIDE_SIM = 3.0

SIM_DT = world.get_physics_dt() if hasattr(world, 'get_physics_dt') else (1.0 / 60.0)

prev_time = time.time()
sps_avg = 0.0
alpha = 0.1
frame_count = 0

sim_time_accumulator = 0.0
current_side = 0

# --- NUEVA VARIABLE DE CONTROL ---
robots_detenidos = False  # Nos ayuda a saber si ya debemos parar

while simulation_app.is_running():
    curr_time = time.time()
    dt_real = curr_time - prev_time
    prev_time = curr_time
    if dt_real > 0:
        sps_avg = (alpha * (1.0 / dt_real)) + (1.0 - alpha) * sps_avg

    if frame_count % 10 == 0:
        pass

    # Si ya se activó la detención, forzamos velocidad 0 y saltamos la lógica de movimiento
    if robots_detenidos:
        fleet_yellow.move_omnidirectional(speed=0.0, direction_deg=0.0)
        fleet_blue.move_omnidirectional(speed=0.0, direction_deg=0.0)
        fleet_yellow.commit_motor_commands()
        fleet_blue.commit_motor_commands()
        world.step(render=not HEADLESS)
        frame_count += 1
        continue  # Saltamos el resto del bucle para que no sigan avanzando

    # --- LÓGICA DE TIEMPO DE SIMULACIÓN ---
    sim_time_accumulator += SIM_DT

    if sim_time_accumulator >= DURATION_PER_SIDE_SIM:
        # ¡ATENCIÓN AQUÍ!
        # Si estábamos en el lado 0 (adelante) y el tiempo ya se cumplió:
        if current_side == 0:
            # >>> AQUÍ SE ACTIVA EL IF QUE PEDISTE <<<
            print("¡Se han completado los 3 segundos hacia adelante! Deteniendo robots...")

            # Puedes poner aquí la lógica que quieras. Por ejemplo:
            robots_detenidos = True

            # Si prefieres romper el bucle por completo y cerrar la app, usarías:
            # break

        lados_a_avanzar = int(sim_time_accumulator // DURATION_PER_SIDE_SIM)
        current_side = (current_side + lados_a_avanzar) % 4
        sim_time_accumulator %= DURATION_PER_SIDE_SIM

    # Movimiento normal (solo se ejecuta si 'robots_detenidos' es False)
    fleet_yellow.move_omnidirectional(speed=SPEED, direction_deg=YELLOW_DIRS[current_side])
    fleet_blue.move_omnidirectional(speed=SPEED, direction_deg=BLUE_DIRS[current_side])

    fleet_yellow.commit_motor_commands(debug=True)
    fleet_blue.commit_motor_commands()

    world.step(render=not HEADLESS)
    frame_count += 1

print("\n[INFO] Cleaning and stopping...")

del fleet_yellow
del fleet_blue
del ball_fleet
del teams_per_env

world.clear_instance()
del world

import gc
gc.collect()

simulation_app.close()
print("[INFO] Exit.")