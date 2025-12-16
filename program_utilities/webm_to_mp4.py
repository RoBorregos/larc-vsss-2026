import os
# --- CAMBIO AQUÍ: Nueva forma de importar en MoviePy 2.0+ ---
try:
	from moviepy import VideoFileClip
except ImportError:
	# Fallback por si acaso tienes una estructura diferente instalada
	from moviepy.video.io.VideoFileClip import VideoFileClip
# ------------------------------------------------------------

def convertir_webm_a_mp4(archivo_entrada, archivo_salida):
	if not os.path.exists(archivo_entrada):
		print(f"❌ Error: El archivo '{archivo_entrada}' no se encuentra.")
		return

	try:
		print(f"🔄 Iniciando conversión: {archivo_entrada} -> {archivo_salida}...")

		# Cargar clip
		clip = VideoFileClip(archivo_entrada)

		# Escribir archivo (MoviePy 2.0 a veces prefiere codec='libx264' explícito)
		clip.write_videofile(archivo_salida, codec='libx264', audio_codec='aac')

		clip.close()
		print(f"✅ ¡Éxito! Video guardado como: {archivo_salida}")

	except Exception as e:
		print(f"⚠️ Ocurrió un error: {e}")

if __name__ == "__main__":
	# --- CONFIGURACIÓN ---
	nombre_entrada = "/home/iker/Documents/RoboticProjects/VSSS/Vision/media/robot_display.webm"  # Cambia esto por el
	# nombre
	# de tu archivo
	nombre_salida = "/home/iker/Documents/RoboticProjects/VSSS/Vision/media/robot_display.mp4"
	# ---------------------

	convertir_webm_a_mp4(nombre_entrada, nombre_salida)