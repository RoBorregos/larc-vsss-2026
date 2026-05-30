from pxr import Gf

config = {
    "robot": {
        "name": "omnirobot",
        "device": "cuda:0",
        "spawn_orientation_euler_deg": (90.0, 0.0, 0.0),
        "joint_prefixes": {
            "wheel": "wheel_",
            "roller": "roller_"
        },
        "usd_path": "/workspace/project/assets/omnirobot/omnirobot.usd"
    },
    "colors": {
        "yellow": Gf.Vec3f(1.0, 1.0, 0.0),
        "blue": Gf.Vec3f(0.0, 0.0, 1.0)
    },
    "motors": {
        "max_velocity": 57.6,
        "max_acceleration_step": 3.0,
        "friction_coast_step": 0.5,
        "wheel_damping": 15.0,
        "max_effort": 0.0343
    },
    "control": {
        "kp": 0.5,
        "ki": 0.01,
        "kd": 0.05
    },
    "sensors": {
        "default_latency_steps": 0,
        "default_encoder_noise_std": 0.0
    },
    "spawn_positions": {
        "yellow": {
            "robot_0": (-0.6, -0.3, 0.05),
            "robot_1": (-0.6, 0.0, 0.05),
            "robot_2": (-0.6, 0.3, 0.05)
        },
        "blue": {
            "robot_0": (0.6, -0.3, 0.05),
            "robot_1": (0.6, 0.0, 0.05),
            "robot_2": (0.6, 0.3, 0.05)
        }
    },
    "debug": {
        "print_interval_sec": .1
    }
}