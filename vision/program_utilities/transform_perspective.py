import cv2
import numpy as np

# 1. Configuración de entrada
# Nota: Verifica que la ruta sea correcta
video_path = '/vsss_video.mp4'
cap = cv2.VideoCapture(video_path)

if not cap.isOpened():
	print("Error: No se pudo abrir el video.")
	exit()

# Factor de escalado (0.8 = 80%)
scale_factor = 0.6

# 2. Configuración de salida
fps = int(cap.get(cv2.CAP_PROP_FPS))
output_width, output_height = 640, 480

# 3. Definir los puntos de transformación
# NOTA: Si redimensionamos el frame de entrada, los puntos 'src' (pts1)
pts1 = np.array([
	[-20, 150],    # Se va a Top-Left (0,0)
	[105, 95],  # Se va a Top-Right (w,0)
	[188, 310],  # Se va a Bottom-Left (0,h) (Ojo: en formato 'Z' el 3ro es BL)
	[284, 200]   # Se va a Bottom-Right (w,h)
], dtype=np.float32)

# Puntos de Destino (dst): Se mantienen igual porque definen el tamaño de salida fijo
pts2 = np.float32([[0, 0],
				   [output_width, 0],
				   [0, output_height],
				   [output_width, output_height]])

# Calcular la Matriz de Transformación (M)
M = cv2.getPerspectiveTransform(pts1, pts2)

# 4. Configurar VideoWriter
fourcc = cv2.VideoWriter_fourcc(*'mp4v')
out = cv2.VideoWriter('../media/video_salida.mp4', fourcc, fps, (output_width, output_height))

print(f"Procesando video con escala {scale_factor}... Presiona 'q' para salir.")

while True:
	ret, frame = cap.read()

	if not ret:
		break

	# --- CAMBIO PRINCIPAL: RESIZE FRAME ---
	# Redimensionamos el frame de entrada al 80% en ambos ejes (fx, fy)
	# None indica que calculamos el tamaño basado en los factores fx y fy
	frame = cv2.resize(frame, None, fx=scale_factor, fy=scale_factor)
	# --------------------------------------

	# 5. Aplicar la transformación de perspectiva
	# La imagen 'frame' ahora es más pequeña, por eso ajustamos pts1 arriba
	warped_frame = cv2.warpPerspective(frame, M, (output_width, output_height))

	# 6. Guardar frame
	out.write(warped_frame)

	# Mostrar en pantalla
	cv2.imshow('Original Redimensionado', frame)
	cv2.imshow('Transformado', warped_frame)

	if cv2.waitKey(int(1000 / fps)) & 0xFF == ord('q'):
		break

cap.release()
out.release()
cv2.destroyAllWindows()
print("Proceso terminado.")