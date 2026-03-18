# --- Physics & Simulation Timing ---
DT = 0.016              # Delta Time (approx. 60 FPS)
FRICTION = 0.880        # Global velocity decay factor
PUSH_FORCE = 60.0       # Force applied during robot movement
RESTITUTION = 0.5         # Bounciness (0 = inelastic collision)
KICK_FORCE = 0.3          # Force applied to the ball when the ball is kicked

# --- Entity Dimensions ---
FIELD_WIDTH = 1.5
FIELD_HEIGHT = 1.3

ROBOT_RADIUS = 3.75
BALL_RADIUS = 2.135

# --- Navigation & Control Thresholds ---
ANGLE_THRESHOLD_MIN = 15    # Tolerance in degrees for alignment when the distance is to the maximum
DISTANCE_THRESHOLD_MAX = 0.4 # Distance where the angle threshold is set to min

ANGLE_THRESHOLD_MAX = 30
DISTANCE_THRESHOLD_MIN = 0.05 # Distance where the angle threshold is set to max

ANGLE_OFFSET = 40       # Applied offset for orbital movement
BASE_SPEED = 0.10       # Movement speed for non-path-planning movement

HELPER_HOLD_OFFSET_X=0.1
HELPER_STOP_THRESHOLD=0.015
GOALKEEPER_Y_THRESHOLD = 0.02
HELPER_Y_THRESHOLD = 0.02
HELPER_FOLLOW_DISTANCE=0.20
HELPER_FOLLOW_SPEED = BASE_SPEED * 0.8
HELPER_MINIMUM_DISTANCE_TO_ATTACKER = 0.05

# --- Display & Field Geometry ---
DISPLAY_SCALE = 7       # Conversion factor from world units to pixels
DISPLAY_CANVAS_WIDTH = 240
DISPLAY_CANVAS_HEIGHT = 135
DISPLAY_FIELD_WIDTH = 150
DISPLAY_FIELD_HEIGHT = 130
DISPLAY_CORNER_LENGTH = 7
DISPLAY_MID_X = DISPLAY_FIELD_WIDTH / 2
DISPLAY_MID_Y = DISPLAY_FIELD_HEIGHT / 2
DISPLAY_GOAL_DEPTH = 10 * DISPLAY_SCALE
DISPLAY_GOAL_WIDTH = 40 * DISPLAY_SCALE
TEXT_OFFSET = 15
DISPLAY_BALL_PREDICTION_INTERVAL = 0.5

# --- Goal Dimensions ---
ATTACKING_RIGHT = True # Set to False if attacking left
GOAL_Y = 0
GOAL_X = DISPLAY_FIELD_WIDTH / 2 if ATTACKING_RIGHT else -DISPLAY_FIELD_WIDTH / 2

# --- UI Padding & Offset Calculations ---
LEFT_PADDING = DISPLAY_SCALE * (DISPLAY_CANVAS_WIDTH - DISPLAY_FIELD_WIDTH) / 2
UPPER_PADDING = DISPLAY_SCALE * (DISPLAY_CANVAS_HEIGHT - DISPLAY_FIELD_HEIGHT) / 2

# --- Visual Markers & UI Elements ---
VISUAL_CIRCLE_DIAMETER = 2.5
VISUAL_CIRCLE_RADIUS = VISUAL_CIRCLE_DIAMETER / 2
VISUAL_CROSS_SIZE = 3.5

# --- Gameplay & Positioning Logic ---
DEFENDER_OFFSET_X = 25
DEFENDER_OFFSET_Y = 0
DEFENDER_MAX_FORWARD = DISPLAY_MID_X + 5

# --- Cardinal Direction Mappings (Degrees) ---
LEFT = 180
RIGHT = 0
DOWN = 90
UP = 270

# --- Goal Post Geometry & Detection Areas ---
DISTANCE_TO_BALL_THRESHOLD = 0.3
ZONE_GOAL = {
    "x": DISPLAY_FIELD_WIDTH / 100 / 2,
    "y": 0,
    "LEFT_PADDING": 0.07,
    "RIGHT_PADDING": 0.15,
    "MIDPOINT_OFFSET": 0.10
}

GOAL = {
    "LEFT_X": -1.5 / 2,
    "LEFT_Y": 0,
    "RIGHT_X": 1.5 / 2,
    "RIGHT_Y": 0
}

MIDFIELD = {
    "x1": 0,
    "y1": 0,  
    "LEFT_PADDING1": 0.65,                 
    "RIGHT_PADDING1": 0.20,                
    "MIDPOINT_OFFSET1": 0.65         
}
# --- Color Definitions & Identification ---
class Color_ID:
    YELLOW = "#dedb38"
    BLUE = "#3b2ad1"
    RED = "#c91223"
    GREEN = "#4bb557"
    CYAN = "#1abeff"
    MAGENTA = "#ca0fd1"
    ORANGE = "#d1860f"
    NONE = None

# --- Robot Identity & Team Color Database ---
# Maps entity ID to a list of identifying colors
BALL_ID = 20
ROBOT_DATABASE = {
    0:  [Color_ID.YELLOW, Color_ID.RED,     Color_ID.GREEN],
    1:  [Color_ID.YELLOW, Color_ID.RED,     Color_ID.CYAN],
    2:  [Color_ID.YELLOW, Color_ID.GREEN,   Color_ID.RED],
    3:  [Color_ID.YELLOW, Color_ID.GREEN,   Color_ID.CYAN],
    4:  [Color_ID.YELLOW, Color_ID.GREEN,   Color_ID.MAGENTA],
    5:  [Color_ID.YELLOW, Color_ID.CYAN,    Color_ID.RED],
    6:  [Color_ID.YELLOW, Color_ID.CYAN,    Color_ID.GREEN],
    7:  [Color_ID.YELLOW, Color_ID.CYAN,    Color_ID.MAGENTA],
    8:  [Color_ID.YELLOW, Color_ID.MAGENTA, Color_ID.GREEN],
    9:  [Color_ID.YELLOW, Color_ID.MAGENTA, Color_ID.CYAN],

    10: [Color_ID.BLUE, Color_ID.RED,     Color_ID.GREEN],
    11: [Color_ID.BLUE, Color_ID.RED,     Color_ID.CYAN],
    12: [Color_ID.BLUE, Color_ID.GREEN,   Color_ID.RED],
    13: [Color_ID.BLUE, Color_ID.GREEN,   Color_ID.CYAN],
    14: [Color_ID.BLUE, Color_ID.GREEN,   Color_ID.MAGENTA],
    15: [Color_ID.BLUE, Color_ID.CYAN,    Color_ID.RED],
    16: [Color_ID.BLUE, Color_ID.CYAN,    Color_ID.GREEN],
    17: [Color_ID.BLUE, Color_ID.CYAN,    Color_ID.MAGENTA],
    18: [Color_ID.BLUE, Color_ID.MAGENTA, Color_ID.GREEN],
    19: [Color_ID.BLUE, Color_ID.MAGENTA, Color_ID.CYAN],

    20: [Color_ID.ORANGE, Color_ID.NONE, Color_ID.NONE]
}

# --- PATH PLANNING ---
ATTRACTIVE_GAIN = 3.0 # Strength of attraction toward the target point
WALL_GAIN = 0.1 # Strength of repulsion from field walls
EXPAND_INFLUENCE_RADIUS = 1.5
EXPAND_REPULSIVE_GAIN = 0.15
WALL_MARGIN = 0.06 # Distance from wall where wall repulsion starts acting
BEHIND_BALL_OFFSET = 0.1 # Distance behind the ball where the attacker aims to position
MAX_FIELD = 4.0 # Maximum magnitude allowed for the total vector field
BLOCKING_WIDTH = 0.18 # Width of the corridor considered for blocking detection
VORTEX_GAIN = 3.0 # Strength of the tangential component in the rolling vector when blocking is detected
ENEMY_REPULSIVE_GAIN = 1.2 # Strength of repulsion from enemy robots
TEAM_REPULSION_GAIN = 0.45 # Strength of repulsion from team robots
TEAM_INFLUENCE_RADIUS = 0.14 # Distance where team robots start influencing the field
INFLUENCE_RADIUS = 0.17 # Distance where enemy robots start influencing the field
BALL_INFLUENCE_RADIUS = 0.12 # Distance where the ball start influencing the field
ROBOT_VORTEX_SIDE = {} # Variable to store the assigned vortex side for each robot when blocking is detected

# --- Robot Movement & Precision ---
MOVEMENT_NOISE = 0.2      # Deviation based on a Gaussian distribution
MIN_SPEED = 0.01        # Minimum velocity threshold; below this, speed resets to 0
ROBOT_KP_ANGULAR_MOVEMENT = 1 # Angular movement speed factor

# --- Environmental & Sensor Simulation ---
ILLUMINATION_INTENSITY = 30
NOISE_RATE = 0.20
REDUCE_ATTRACTION_IF_BLOCKED = 0.4

# --- Measurements ---
WEIGHT_CONVERSION = 0.01
METER_TO_CM = 100