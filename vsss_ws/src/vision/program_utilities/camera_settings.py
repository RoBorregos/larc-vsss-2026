import cv2
import time

def nothing(x):
    pass

def print_settings(cap, source="Botón"):
    print(f"\n--- 📸 SETTINGS ({source}) ---")
    timestamp = time.strftime("%H:%M:%S")

    auto_exp = "Auto (3)" if cap.get(cv2.CAP_PROP_AUTO_EXPOSURE) == 3 else "Manual (1)"
    auto_foco = "Auto (1)" if cap.get(cv2.CAP_PROP_AUTOFOCUS) == 1 else "Manual (0)"

    print(f"[{timestamp}]")
    print(f"Brightness:        {cap.get(cv2.CAP_PROP_BRIGHTNESS)}")
    print(f"Contrast:     {cap.get(cv2.CAP_PROP_CONTRAST)}")
    print(f"Auto-Exp:      {auto_exp}")
    print(f"Exposure:    {cap.get(cv2.CAP_PROP_EXPOSURE)}")
    print(f"Auto-Focus:     {auto_foco}")
    print(f"Focus:          {cap.get(cv2.CAP_PROP_FOCUS)}")
    print("-" * 40)

def main():
    device_path = '/dev/vsss_cam'
    print(f"Starting device {device_path}...")

    cap = cv2.VideoCapture(device_path, cv2.CAP_V4L2)
    if not cap.isOpened():
        cap = cv2.VideoCapture(0)
        if not cap.isOpened():
            print("❌ Error: Camera not found.")
            return

    defaults = {
        "brightness": int(cap.get(cv2.CAP_PROP_BRIGHTNESS)),
        "contrast":   int(cap.get(cv2.CAP_PROP_CONTRAST)),
        # V4L2: 1=Manual, 3=Auto. Convertimos a 0/1 para el slider.
        "auto_exp":   1 if cap.get(cv2.CAP_PROP_AUTO_EXPOSURE) == 3 else 0,
        "exposure":   int(cap.get(cv2.CAP_PROP_EXPOSURE)),
        "auto_focus": int(cap.get(cv2.CAP_PROP_AUTOFOCUS)),
        "focus":      int(cap.get(cv2.CAP_PROP_FOCUS))
    }

    print("✅ Default values saved:", defaults)

    # 3. INTERFAZ GRÁFICA
    window_name = "VSSS Settings"
    cv2.namedWindow(window_name)

    # Sliders (Inician en la posición actual de la cámara)
    cv2.createTrackbar('Brightness', window_name, defaults["brightness"], 255, nothing)
    cv2.createTrackbar('Contrast', window_name, defaults["contrast"], 255, nothing)

    # Exposición
    cv2.createTrackbar('Auto Exp (0=Man, 1=Auto)', window_name, defaults["auto_exp"], 1, nothing)
    cv2.createTrackbar('Exposure', window_name, defaults["exposure"], 1000, nothing)

    # Foco
    cv2.createTrackbar('Auto Focus (0=Man, 1=Auto)', window_name, defaults["auto_focus"], 1, nothing)
    cv2.createTrackbar('Focus', window_name, defaults["focus"], 255, nothing)

    # BOTONES DE ACCIÓN (Simulados)
    cv2.createTrackbar('>> PRINT INFO <<', window_name, 0, 1, nothing)
    cv2.createTrackbar('>> RESET DEFAULT <<', window_name, 0, 1, nothing)

    print("   [p] Print settings")
    print("   [r] Reset to default")
    print("   [q] Exit")

    while True:
        ret, frame = cap.read()
        if not ret:
            break

        trigger_reset = cv2.getTrackbarPos('>> RESET DEFAULT <<', window_name)
        key = cv2.waitKey(1) & 0xFF

        if trigger_reset == 1 or key == ord('r'):
            print("\n🔄 Reset...")

            cap.set(cv2.CAP_PROP_BRIGHTNESS, defaults["brightness"])
            cap.set(cv2.CAP_PROP_CONTRAST, defaults["contrast"])

            v4l2_def_exp = 3 if defaults["auto_exp"] == 1 else 1
            cap.set(cv2.CAP_PROP_AUTO_EXPOSURE, v4l2_def_exp)
            if v4l2_def_exp == 1:
                cap.set(cv2.CAP_PROP_EXPOSURE, defaults["exposure"])

            cap.set(cv2.CAP_PROP_AUTOFOCUS, defaults["auto_focus"])
            if defaults["auto_focus"] == 0:
                cap.set(cv2.CAP_PROP_FOCUS, defaults["focus"])

            cv2.setTrackbarPos('Brightness', window_name, defaults["brightness"])
            cv2.setTrackbarPos('Contrast', window_name, defaults["contrast"])
            cv2.setTrackbarPos('Auto Exp (0=Man, 1=Auto)', window_name, defaults["auto_exp"])
            cv2.setTrackbarPos('Exposure', window_name, defaults["exposure"])
            cv2.setTrackbarPos('Auto Focus (0=Man, 1=Auto)', window_name, defaults["auto_focus"])
            cv2.setTrackbarPos('Focus', window_name, defaults["focus"])

            cv2.setTrackbarPos('>> RESET DEFAULT <<', window_name, 0)
            print("✅ Reset to default.")

        trigger_print = cv2.getTrackbarPos('>> PRINT INFO <<', window_name)
        if trigger_print == 1 or key == ord('p'):
            print_settings(cap, source="User")
            cv2.setTrackbarPos('>> PRINT INFO <<', window_name, 0)

        if trigger_reset == 0 and key != ord('r'):
            brightness = cv2.getTrackbarPos('Brightness', window_name)
            contrast = cv2.getTrackbarPos('Contrast', window_name)
            auto_exp_gui = cv2.getTrackbarPos('Auto Exp (0=Man, 1=Auto)', window_name)
            exp_val = cv2.getTrackbarPos('Exposure', window_name)
            auto_focus_gui = cv2.getTrackbarPos('Auto Focus (0=Man, 1=Auto)', window_name)
            focus_val = cv2.getTrackbarPos('Focus', window_name)

            cap.set(cv2.CAP_PROP_BRIGHTNESS, brightness)
            cap.set(cv2.CAP_PROP_CONTRAST, contrast)

            v4l2_auto_exp = 3 if auto_exp_gui == 1 else 1
            cap.set(cv2.CAP_PROP_AUTO_EXPOSURE, v4l2_auto_exp)
            if v4l2_auto_exp == 1:
                cap.set(cv2.CAP_PROP_EXPOSURE, exp_val)

            cap.set(cv2.CAP_PROP_AUTOFOCUS, auto_focus_gui)
            if auto_focus_gui == 0:
                cap.set(cv2.CAP_PROP_FOCUS, focus_val)

        if key == ord('q'):
            break

        cv2.imshow(window_name, frame)

    cap.release()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
