import cv2
import os
import sys
from datetime import datetime

def nothing(x):
	pass

save_path = input("Enter test recording media directory: ");
timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
file_name = f"vsss_capture_{timestamp}.mp4"
full_path = os.path.join(save_path, file_name)

try:
	if not os.path.exists(save_path):
		os.makedirs(save_path)
		print(f"Directory created successfully: {save_path}")
except OSError as e:
	print(f"FATAL ERROR: Could not create directory {save_path}. {e}", file=sys.stderr)
	sys.exit(1)

cap = cv2.VideoCapture(2, cv2.CAP_V4L2)

frame_width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
frame_height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
fps = 30

fourcc = cv2.VideoWriter_fourcc(*'mp4v')
out = cv2.VideoWriter(full_path, fourcc, fps, (frame_width, frame_height))

cv2.namedWindow('VSSS Recorder')
cv2.createTrackbar('Brightness', 'VSSS Recorder', 128, 255, nothing)

print(f"Recording MP4: {full_path}")

while cap.isOpened():
	ret, frame = cap.read()
	if not ret:
		break

	b = cv2.getTrackbarPos('Brightness', 'VSSS Recorder')
	cap.set(cv2.CAP_PROP_BRIGHTNESS, b)

	out.write(frame)

	cv2.putText(frame, f"REC: {file_name}", (20, 30),
				cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 1)
	cv2.imshow('VSSS Recorder', frame)

	if cv2.waitKey(1) == ord('q'):
		break

cap.release()
out.release()
cv2.destroyAllWindows()
