from dataclasses import dataclass

@dataclass
class StrategySettings:
    # Umbrales dinámicos y patada
    distance_threshold_min_mm: float = 100.0
    distance_threshold_max_mm: float = 500.0
    angle_threshold_min_rad: float = 0.1
    angle_threshold_max_rad: float = 0.5
    kick_angle_tolerance_rad: float = 0.2
    prediction_horizon_s: float = 0.5
    base_speed_mm_s: float = 150.0
    angle_offset_rad: float = 0.26

    # General
    attacking_right: bool = True
    ball_prediction_print_interval_s: float = 1.0

    # Atacante
    distance_to_ball_threshold_mm: float = 100.0
    behind_ball_offset_mm: float = 80.0

    # Portero / Defensa
    goalkeeper_left_padding_mm: float = 50.0
    goalkeeper_right_padding_mm: float = 150.0
    goalkeeper_y_threshold_mm: float = 20.0

    # Helper
    helper_follow_distance_mm: float = 250.0
    helper_min_distance_to_attacker_mm: float = 150.0


@dataclass
class SimulationSettings:
    movement_noise: float = 0.05
    min_speed_mm_s: float = 10.0


@dataclass
class VisionSettings:
    velocity_smoothing_alpha: float = 0.3
    pose_update_hz: float = 30.0

@dataclass
class RoleSettings:
    min_score_distance_mm: float = 50.0

    # Pesos de defensa
    defender_distance_weight: float = 0.6
    defender_hysteresis_weight: float = 0.4

    # Pesos de ataque
    attacker_distance_weight: float = 0.6
    attacker_angle_weight: float = 0.3
    attacker_hysteresis_weight: float = 0.1

@dataclass
class PathPlanningSettings:
    # Ganancias
    attractive_gain: float = 1.0
    enemy_repulsive_gain: float = 1.5
    vortex_gain: float = 1.5
    wall_gain: float = 1.0
    max_field: float = 2.0

    # Factores de ajuste
    expand_influence_radius: float = 1.2
    expand_repulsive_gain: float = 1.2
    reduce_attraction_if_blocked: float = 0.5

    # Distancias y radios (en mm — ajusta a tus valores anteriores * 1000 si estaban en metros)
    influence_radius_mm: float = 200.0
    team_influence_radius_mm: float = 150.0
    ball_influence_radius_mm: float = 100.0
    wall_margin_mm: float = 100.0
    obstacle_safety_margin_mm: float = 30.0
    blocking_width_mm: float = 50.0


# Instancias listas para importar
path_planning = PathPlanningSettings()
strategy = StrategySettings()
simulation = SimulationSettings()
roles = RoleSettings()
vision = VisionSettings()