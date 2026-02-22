import json
import os
import matplotlib
matplotlib.use('TkAgg')
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import numpy as np
import mplcursors

JSON_PATH = input("Enter calibration's file: ")

COLOR_MAP = {
	"blue":    "navy",
	"cyan":    "cyan",
	"yellow":  "gold",
	"green":   "lime",
	"magenta": "magenta",
	"red":     "red",
	"orange":  "orange"
}

def load_data_from_json(filepath):
	if not os.path.exists(filepath):
		print(f"[ERROR] {filepath} was not found")
		return {}
	with open(filepath, 'r') as f:
		try:
			return json.load(f)
		except json.JSONDecodeError as e:
			print(f"[ERROR] Corrupt JSON: {e}")
			return {}

def main():
	data = load_data_from_json(JSON_PATH)
	if not data:
		return

	fig = plt.figure(figsize=(12, 9))
	ax = fig.add_subplot(111, projection='3d')

	print(f"[INFO] Visualizing data from: {JSON_PATH}")

	scatter_artists = []

	for color_key, items_list in data.items():
		if color_key == "color_calibration" or color_key == "roi_points" or color_key == "mask_params" or color_key == "camera_calibration":
			continue

		display_color = COLOR_MAP.get(color_key, "gray")

		h_vals, s_vals, v_vals = [], [], []
		custom_data_list = []

		for i, item in enumerate(items_list):
			if not item.get("valid", False):
				continue

			avg = item.get("avg_hsv", [0, 0, 0])
			h, s, v = avg[0], avg[1], avg[2]

			h_vals.append(h)
			s_vals.append(s)
			v_vals.append(v)

			ax.plot([h, h], [s, s], [0, v], color=display_color, linestyle='--', linewidth=1, alpha=0.3)

			# Preparar textos
			h_range = item.get("min_hsv", ["-","-","-"])
			max_hsv = item.get("max_hsv", ["-","-","-"])

			info_text = (
				f"COLOR: {color_key.upper()}\n"
				f"HSV Avg: ({int(h)}, {int(s)}, {int(v)})\n"
				f"Rango H: {h_range[0]} - {max_hsv[0]}"
			)

			custom_data_list.append({
				"label": info_text,
				"raw_json": item
			})

		if h_vals:
			sc = ax.scatter(
				h_vals, s_vals, v_vals,
				c=display_color, marker='o', s=80, edgecolors='black', alpha=0.9,
				label=color_key
			)
			sc.custom_data = custom_data_list
			scatter_artists.append(sc)

	ax.set_xlabel('HUE (Matiz)')
	ax.set_ylabel('SATURATION')
	ax.set_zlabel('VALUE (Brillo)')
	ax.set_xlim(0, 180); ax.set_ylim(0, 255); ax.set_zlim(0, 255)

	plt.title(f'VSSS Calibration display \nfile: {os.path.basename(JSON_PATH)}')
	plt.legend(loc='upper left')

	cursor = mplcursors.cursor(scatter_artists, hover=True)

	@cursor.connect("add")
	def on_add(sel):
		try:
			sel.annotation.arrow_patch.set_visible(False)
			sel.annotation.arrow_patch.set_connectionstyle("arc3,rad=0")
		except Exception:
			pass

		try:
			if hasattr(sel, "index"):
				index = sel.index
			elif hasattr(sel.target, "index"):
				index = sel.target.index
			else:
				sel.annotation.set_text("Error: No index")
				return

			if hasattr(index, '__iter__') and len(index) > 0:
				index = index[0]

			index = int(index)

			artist = sel.artist
			if hasattr(artist, "custom_data") and index < len(artist.custom_data):
				data_item = artist.custom_data[index]
				sel.annotation.set_text(data_item["label"])

				sel.annotation.get_bbox_patch().set(fc="white", alpha=0.95, edgecolor="black", boxstyle="round,pad=0.5")

			else:
				sel.annotation.set_text("Sin datos")

		except Exception as e:
			print(f"[WARNING] Tooltip error: {e}")
			sel.annotation.set_visible(False)

	print("[INFO] Hover mouse for information.")
	plt.show()

if __name__ == "__main__":
	main()
