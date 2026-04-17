import math

BETA = math.radians(60)  # degrees
GAMA = 0 # degrees
OMEGA_TO_RPM = 60 / (2 * math.pi)  # Conversion factor from rad/s to RPM
STATE_PACKET_SIZE = 16
INIT_PACKET_SIZE = 4
WATCHDOG_TIME = 500000000  # 500 ms in nanoseconds

DEFAULT_PORT = 8081