# --- Physics & Simulation Timing ---
DT = 0.016              # Delta Time (approx. 60 FPS)
FRICTION = 0.880        # Global velocity decay factor
PUSH_FORCE = 60.0       # Force applied during robot movement
RESTITUTION = 0.5         # Bounciness (0 = inelastic collision)
KICK_FORCE = 2          # Force applied to the ball when the ball is kicked

# --- Entity Dimensions ---
ROBOT_RADIUS = 3.75
BALL_RADIUS = 2.135

# --- Display & Field Geometry ---
DISPLAY_CANVAS_WIDTH = 240
DISPLAY_CANVAS_HEIGHT = 135
DISPLAY_FIELD_WIDTH = 150
DISPLAY_FIELD_HEIGHT = 130
DISPLAY_MID_X = DISPLAY_FIELD_WIDTH / 2
DISPLAY_SCALE = 7       # Conversion factor from world units to pixels
TEXT_OFFSET = 15

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

# --- Navigation & Control Thresholds ---
ANGLE_THRESHOLD = 15    # Tolerance in degrees for alignment
ANGLE_OFFSET = 40       # Applied offset for orbital movement
BASE_SPEED = 0.10       # Movement speed for non-path-planning movement

# --- Cardinal Direction Mappings (Degrees) ---
LEFT = 180
RIGHT = 0
DOWN = 90
UP = 270

# --- Goal Post Geometry & Detection Areas ---
GOAL = {
    "x": DISPLAY_FIELD_WIDTH / 100 / 2,
    "y": 0,
    "LEFT_PADDING": 0.07,
    "RIGHT_PADDING": 0.15,
    "MIDPOINT_OFFSET": 0.10
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

# --- Robot Movement & Precision ---
MOVEMENT_NOISE = 1      # Deviation based on a Gaussian distribution
MIN_SPEED = 0.01        # Minimum velocity threshold; below this, speed resets to 0

# --- Environmental & Sensor Simulation ---
ILLUMINATION_INTENSITY = 30
NOISE_RATE = 0.20

# --- Measurements ---
WEIGHT_CONVERSION = 0.01