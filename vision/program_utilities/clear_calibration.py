import json
import os

def procesar_archivo_colores():
    filename = "/media/ikercsv/Files/Projects/larc-vsss-2026/vision/vsss_calibration.json"

    if not os.path.exists(filename):
        print(f"Error: No se encuentra el archivo '{filename}'.")
        return

    try:
        with open(filename, 'r', encoding='utf-8') as f:
            data = json.load(f)
    except json.JSONDecodeError:
        print("Error: El archivo no es un JSON válido.")
        return

    # Definimos explícitamente qué llaves son colores
    colores_objetivo = [
        "blue", "cyan", "green", "magenta",
        "orange", "red", "yellow"
    ]

    modificado = False

    print("\n--- Revisión de Capas de Color ---")

    for color in colores_objetivo:
        # Verificamos si el color existe en el JSON y si tiene datos
        if color in data and isinstance(data[color], list) and len(data[color]) > 0:
            confirmar = input(f"¿Vaciar datos de '{color}'? ({len(data[color])} registros) [s/n]: ").lower()

            if confirmar == 's':
                data[color] = []
                modificado = True
                print(f"✓ '{color}' limpiado.")
            else:
                print(f"- '{color}' mantenido.")
        elif color in data:
            print(f"i '{color}' ya está vacío o no es una lista.")

    if modificado:
        try:
            with open(filename, 'w', encoding='utf-8') as f:
                json.dump(data, f, indent=4)
            print(f"\nArchivo '{filename}' actualizado correctamente.")
        except Exception as e:
            print(f"Error al guardar el archivo: {e}")
    else:
        print("\nNo se realizaron cambios.")

if __name__ == "__main__":
    procesar_archivo_colores()