import json
import os
import matplotlib
# Forzamos el backend TkAgg para evitar errores en Linux/Pop!_OS
matplotlib.use('TkAgg')
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import numpy as np

# ================= CONFIGURACIÓN =================
# Ajusta esta ruta a donde tengas tu archivo json
JSON_PATH = "/home/iker/Documents/RoboticProjects/VSSS/vision/vsss_calibration.json"

# Mapeo de colores del JSON a colores de Matplotlib
COLOR_MAP = {
	"blue":    "navy",
	"cyan":    "cyan",
	"yellow":  "gold",
	"green":   "lime",
	"magenta": "magenta",
	"red":     "red",
	"orange":  "orange"
}
# =================================================

def load_data_from_json(filepath):
	if not os.path.exists(filepath):
		print(f"[ERROR] No se encontró el archivo: {filepath}")
		return {}

	with open(filepath, 'r') as f:
		try:
			return json.load(f)
		except json.JSONDecodeError as e:
			print(f"[ERROR] El JSON está corrupto: {e}")
			return {}

def main():
	data = load_data_from_json(JSON_PATH)
	if not data:
		return

	fig = plt.figure(figsize=(12, 9))
	ax = fig.add_subplot(111, projection='3d')

	print(f"[INFO] Visualizando datos de: {JSON_PATH}")

	# Iterar por cada color en el JSON (blue, yellow, etc.)
	for color_key, items_list in data.items():

		display_color = COLOR_MAP.get(color_key, "gray")

		for item in items_list:
			# Solo mostrar calibraciones válidas
			if not item.get("valid", False):
				continue

			# Obtener el promedio HSV
			avg = item.get("avg_hsv", [0, 0, 0])
			h, s, v = avg[0], avg[1], avg[2]

			# --- ANÁLISIS DE CONFLICTOS AUTOMÁTICO ---
			marker = 'o'
			size = 80
			edge = 'black'
			label_text = color_key

			# Graficar el punto
			ax.scatter(h, s, v, c=display_color, marker=marker, s=size, edgecolors=edge, alpha=0.8, label=label_text)

			# Línea al suelo para ver la altura (Value)
			ax.plot([h, h], [s, s], [0, v], color=display_color, linestyle='--', linewidth=1, alpha=0.4)

	# Configuración de Ejes
	ax.set_xlabel('HUE (Matiz) [0-180]')
	ax.set_ylabel('SATURATION [0-255]')
	ax.set_zlabel('VALUE (Brillo) [0-255]')

	ax.set_xlim(0, 180)
	ax.set_ylim(0, 255)
	ax.set_zlim(0, 255)

	plt.title(f'Visualización de Calibración VSSS\nArchivo: {os.path.basename(JSON_PATH)}')

	# Truco para limpiar la leyenda (evitar etiquetas repetidas)
	handles, labels = plt.gca().get_legend_handles_labels()
	by_label = dict(zip(labels, handles))
	plt.legend(by_label.values(), by_label.keys(), loc='upper left')

	print("[INFO] Gráfica generada. Cierra la ventana para salir.")
	plt.show()

if __name__ == "__main__":
	main()