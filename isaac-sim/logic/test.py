import argparse
from isaaclab.app import AppLauncher

# Lanzar la app primero
parser = argparse.ArgumentParser(description="Prueba VSSS")
AppLauncher.add_app_launcher_args(parser)
args_cli = parser.parse_args()

app_launcher = AppLauncher(args_cli)
simulation_app = app_launcher.app

# Importaciones de IsaacLab usando el prefijo correcto de tu versión
import isaaclab.sim as sim_utils
from isaaclab.sim import SimulationContext

print("Simulador iniciado correctamente. Configurando entorno VSSS...")

# Usar SimulationContext para las físicas y el entorno
sim = SimulationContext()
sim.set_camera_view([2.0, 2.0, 2.0], [0.0, 0.0, 0.0]) # Posicionar la cámara

# 1. Añadir luz
cfg_light = sim_utils.DistantLightCfg(intensity=3000.0, color=(1.0, 1.0, 1.0))
cfg_light.func("/World/Light", cfg_light, translation=(1, 0, 10))

# 2. Añadir piso de colisión
cfg_ground = sim_utils.GroundPlaneCfg()
cfg_ground.func("/World/GroundPlane", cfg_ground)

# 3. Crear la pelota de VSSS con física completa
cfg_ball = sim_utils.SphereCfg(
    radius=0.02135,
    rigid_props=sim_utils.RigidBodyPropertiesCfg(), # Lo hace un cuerpo rígido
    mass_props=sim_utils.MassPropertiesCfg(mass=0.046), # 46 gramos
    collision_props=sim_utils.CollisionPropertiesCfg(), # Habilita colisiones
    visual_material=sim_utils.PreviewSurfaceCfg(diffuse_color=(1.0, 0.5, 0.0)) # Color naranja
)
# Instanciarla a medio metro de altura
cfg_ball.func("/World/Ball", cfg_ball, translation=(0.0, 0.0, 0.5))

# Reiniciar el contexto para aplicar las físicas antes de correr
sim.reset()

print("¡Listo! Entrando al bucle de simulación...")

# Bucle principal
while simulation_app.is_running():

    sim.step(render=True)

# Cerrar limpio
simulation_app.close()
