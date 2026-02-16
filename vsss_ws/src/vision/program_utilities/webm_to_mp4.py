import os

try:
	from moviepy import VideoFileClip
except ImportError:
	from moviepy.video.io.VideoFileClip import VideoFileClip

def convert_webm_a_mp4(archivo_entrada, archivo_salida):
	if not os.path.exists(archivo_entrada):
		print(f"❌ Error: File '{archivo_entrada}' not found.")
		return

	try:
		print(f"🔄 Starting conversion: {archivo_entrada} -> {archivo_salida}...")

		clip = VideoFileClip(archivo_entrada)

		clip.write_videofile(archivo_salida, codec='libx264', audio_codec='aac')

		clip.close()
		print(f"✅ Video saved: {archivo_salida}")

	except Exception as e:
		print(f"⚠️ Error: {e}")

if __name__ == "__main__":
	nombre_entrada = input("Input file: .webm: ")
	nombre_salida = input("Output file: .mp4: ")

	convert_webm_a_mp4(nombre_entrada, nombre_salida)
