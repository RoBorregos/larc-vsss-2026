import cv2
import numpy as np

video_path = input("Input video path: ")
output_path = input("Output video path: ")
cap = cv2.VideoCapture(video_path)

if not cap.isOpened():
	print("Error: Video could not be opened.")
	exit()

scale_factor = 0.6

fps = int(cap.get(cv2.CAP_PROP_FPS))
output_width, output_height = 640, 480

pts1 = np.array([
	[-20, 150],    # Top-Left (0,0)
	[105, 95],  # Top-Right (w,0)
	[188, 310],  # Bottom-Left (0,h)
	[284, 200]   # Bottom-Right (w,h)
], dtype=np.float32)

pts2 = np.float32([[0, 0],
				   [output_width, 0],
				   [0, output_height],
				   [output_width, output_height]])

M = cv2.getPerspectiveTransform(pts1, pts2)

fourcc = cv2.VideoWriter_fourcc(*'mp4v')
out = cv2.VideoWriter(output_path, fourcc, fps, (output_width, output_height))

print(f"Processing video (scale factor: {scale_factor}) ... Press 'q' to exit.")

while True:
	ret, frame = cap.read()

	if not ret:
		break

	frame = cv2.resize(frame, None, fx=scale_factor, fy=scale_factor)

	warped_frame = cv2.warpPerspective(frame, M, (output_width, output_height))

	out.write(warped_frame)

	cv2.imshow('Resized frame', frame)
	cv2.imshow('Warped frame', warped_frame)

	if cv2.waitKey(int(1000 / fps)) & 0xFF == ord('q'):
		break

cap.release()
out.release()
cv2.destroyAllWindows()
