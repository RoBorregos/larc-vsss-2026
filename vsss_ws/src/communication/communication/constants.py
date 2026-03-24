import math

BETA = math.radians(30)  # degrees
GAMA = 0 # degrees
OMEGA_TO_RPM = 60 / (2 * math.pi)  # Conversion factor from rad/s to RPM
COS_30 = math.cos(BETA)
SIN_30 = math.sin(BETA)
STATE_PACKET_SIZE = 16
INIT_PACKET_SIZE = 4