import json
import os

def process_file():
    filename = input("Enter calibration's file: ")

    if not os.path.exists(filename):
        print(f"Error: {filename} not found.")
        return

    try:
        with open(filename, 'r', encoding='utf-8') as f:
            data = json.load(f)
    except json.JSONDecodeError:
        print("Error: Invalid JSON.")
        return

    colores_objetivo = [
        "blue", "cyan", "green", "magenta",
        "orange", "red", "yellow"
    ]

    modified = False


    for color in colores_objetivo:
        if color in data and isinstance(data[color], list) and len(data[color]) > 0:
            confirmation = input(f"¿Clear color '{color}'? ({len(data[color])} occurrences) [y/n]: ").lower()

            if confirmation == 'y':
                data[color] = []
                modified = True
                print(f"✓ '{color}' cleared.")
            else:
                print(f"- '{color}' maintained.")
        elif color in data:
            print(f"i '{color}' no action is available.")

    if modified:
        try:
            with open(filename, 'w', encoding='utf-8') as f:
                json.dump(data, f, indent=4)
            print(f"\nFile '{filename}' updated.")
        except Exception as e:
            print(f"Error when saving the file: {e}")
    else:
        print("\nNo changes were made.")

if __name__ == "__main__":
    process_file()
