import cv2
import time

def nothing(x):
    pass

def print_settings(cap, source="Botón"):
    """ Imprime la configuración actual en la terminal """
    print(f"\n--- 📸 CONFIGURACIÓN ACTUAL ({source}) ---")
    timestamp = time.strftime("%H:%M:%S")

    # Mapeo para que sea legible
    auto_exp = "Auto (3)" if cap.get(cv2.CAP_PROP_AUTO_EXPOSURE) == 3 else "Manual (1)"
    auto_foco = "Auto (1)" if cap.get(cv2.CAP_PROP_AUTOFOCUS) == 1 else "Manual (0)"

    print(f"[{timestamp}]")
    print(f"Brillo:        {cap.get(cv2.CAP_PROP_BRIGHTNESS)}")
    print(f"Contraste:     {cap.get(cv2.CAP_PROP_CONTRAST)}")
    print(f"Auto-Exp:      {auto_exp}")
    print(f"Exposición:    {cap.get(cv2.CAP_PROP_EXPOSURE)}")
    print(f"Auto-Foco:     {auto_foco}")
    print(f"Foco:          {cap.get(cv2.CAP_PROP_FOCUS)}")
    print("-" * 40)

def main():
    # 1. CONFIGURACIÓN E INICIO
    device_path = '/dev/vsss_cam' # Cambiar a 0 si es webcam normal
    print(f"Iniciando cámara en {device_path}...")

    cap = cv2.VideoCapture(device_path, cv2.CAP_V4L2)
    if not cap.isOpened():
        cap = cv2.VideoCapture(0) # Intento secundario
        if not cap.isOpened():
            print("❌ Error: No se detecta la cámara.")
            return

    # 2. GUARDAR VALORES POR DEFECTO (SNAPSHOT INICIAL)
    # Leemos cómo estaba la cámara antes de que nosotros tocáramos nada.
    defaults = {
        "brightness": int(cap.get(cv2.CAP_PROP_BRIGHTNESS)),
        "contrast":   int(cap.get(cv2.CAP_PROP_CONTRAST)),
        # V4L2: 1=Manual, 3=Auto. Convertimos a 0/1 para el slider.
        "auto_exp":   1 if cap.get(cv2.CAP_PROP_AUTO_EXPOSURE) == 3 else 0,
        "exposure":   int(cap.get(cv2.CAP_PROP_EXPOSURE)),
        "auto_focus": int(cap.get(cv2.CAP_PROP_AUTOFOCUS)),
        "focus":      int(cap.get(cv2.CAP_PROP_FOCUS))
    }

    print("✅ Valores por defecto guardados:", defaults)

    # 3. INTERFAZ GRÁFICA
    window_name = "Configurador VSSS (Con Reset)"
    cv2.namedWindow(window_name)

    # Sliders (Inician en la posición actual de la cámara)
    cv2.createTrackbar('Brillo', window_name, defaults["brightness"], 255, nothing)
    cv2.createTrackbar('Contraste', window_name, defaults["contrast"], 255, nothing)

    # Exposición
    cv2.createTrackbar('Auto Exp (0=Man, 1=Auto)', window_name, defaults["auto_exp"], 1, nothing)
    cv2.createTrackbar('Exposicion', window_name, defaults["exposure"], 1000, nothing)

    # Foco
    cv2.createTrackbar('Auto Focus (0=Man, 1=Auto)', window_name, defaults["auto_focus"], 1, nothing)
    cv2.createTrackbar('Foco', window_name, defaults["focus"], 255, nothing)

    # BOTONES DE ACCIÓN (Simulados)
    cv2.createTrackbar('>> IMPRIMIR INFO <<', window_name, 0, 1, nothing)
    cv2.createTrackbar('>> RESET DEFAULT <<', window_name, 0, 1, nothing)

    print("\n✅ Controles listos.")
    print("   [p] Imprimir configuración")
    print("   [r] Resetear a valores iniciales")
    print("   [q] Salir")

    while True:
        ret, frame = cap.read()
        if not ret:
            break

        # --- LÓGICA DE RESETEO ---
        # Verificamos si se presionó el botón visual o la tecla 'r'
        trigger_reset = cv2.getTrackbarPos('>> RESET DEFAULT <<', window_name)
        key = cv2.waitKey(1) & 0xFF

        if trigger_reset == 1 or key == ord('r'):
            print("\n🔄 Restaurando valores de fábrica...")

            # 1. Restaurar Cámara
            cap.set(cv2.CAP_PROP_BRIGHTNESS, defaults["brightness"])
            cap.set(cv2.CAP_PROP_CONTRAST, defaults["contrast"])

            # Exp: Convertir de vuelta al estándar V4L2 (3=Auto, 1=Manual)
            v4l2_def_exp = 3 if defaults["auto_exp"] == 1 else 1
            cap.set(cv2.CAP_PROP_AUTO_EXPOSURE, v4l2_def_exp)
            # Solo aplicamos el valor manual si el default era manual
            if v4l2_def_exp == 1:
                cap.set(cv2.CAP_PROP_EXPOSURE, defaults["exposure"])

            cap.set(cv2.CAP_PROP_AUTOFOCUS, defaults["auto_focus"])
            if defaults["auto_focus"] == 0:
                cap.set(cv2.CAP_PROP_FOCUS, defaults["focus"])

            # 2. Restaurar GUI (Mover los sliders visualmente)
            cv2.setTrackbarPos('Brillo', window_name, defaults["brightness"])
            cv2.setTrackbarPos('Contraste', window_name, defaults["contrast"])
            cv2.setTrackbarPos('Auto Exp (0=Man, 1=Auto)', window_name, defaults["auto_exp"])
            cv2.setTrackbarPos('Exposicion', window_name, defaults["exposure"])
            cv2.setTrackbarPos('Auto Focus (0=Man, 1=Auto)', window_name, defaults["auto_focus"])
            cv2.setTrackbarPos('Foco', window_name, defaults["focus"])

            # 3. Reiniciar el botón de reset a 0
            cv2.setTrackbarPos('>> RESET DEFAULT <<', window_name, 0)
            print("✅ Restauración completada.")

        # --- LÓGICA DE IMPRESIÓN ---
        trigger_print = cv2.getTrackbarPos('>> IMPRIMIR INFO <<', window_name)
        if trigger_print == 1 or key == ord('p'):
            print_settings(cap, source="Usuario")
            cv2.setTrackbarPos('>> IMPRIMIR INFO <<', window_name, 0)

        # --- LÓGICA NORMAL (Lectura de Sliders) ---
        # Solo aplicamos valores si NO estamos reseteando en este frame para evitar conflictos
        if trigger_reset == 0 and key != ord('r'):
            brillo = cv2.getTrackbarPos('Brillo', window_name)
            contraste = cv2.getTrackbarPos('Contraste', window_name)
            auto_exp_gui = cv2.getTrackbarPos('Auto Exp (0=Man, 1=Auto)', window_name)
            exp_val = cv2.getTrackbarPos('Exposicion', window_name)
            auto_focus_gui = cv2.getTrackbarPos('Auto Focus (0=Man, 1=Auto)', window_name)
            focus_val = cv2.getTrackbarPos('Foco', window_name)

            cap.set(cv2.CAP_PROP_BRIGHTNESS, brillo)
            cap.set(cv2.CAP_PROP_CONTRAST, contraste)

            # Lógica Auto-Exposición
            v4l2_auto_exp = 3 if auto_exp_gui == 1 else 1
            cap.set(cv2.CAP_PROP_AUTO_EXPOSURE, v4l2_auto_exp)
            if v4l2_auto_exp == 1:
                cap.set(cv2.CAP_PROP_EXPOSURE, exp_val)

            # Lógica Auto-Foco
            cap.set(cv2.CAP_PROP_AUTOFOCUS, auto_focus_gui)
            if auto_focus_gui == 0:
                cap.set(cv2.CAP_PROP_FOCUS, focus_val)

        # Salir
        if key == ord('q'):
            break

        cv2.imshow(window_name, frame)

    cap.release()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()