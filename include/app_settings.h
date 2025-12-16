#ifndef APP_SETTINGS_H
#define APP_SETTINGS_H

#include <string>

enum class AppState {
	MAIN_MENU,
	CALIBRATION_MENU,
	CALIBRATING,
	DETECTION
};

struct VisionParams {
	float saturation = 1.0f;
	float gamma_correction = 1.0f;
	float clahe_clip_limit = 1.0f;
	float bilateral_sigma = 0.0f;
	float green_boost = 1.0f;
	float blue_boost = 1.0f;
	float red_boost = 1.0f;
};

struct AppData {
	AppState current_state = AppState::MAIN_MENU;
	VisionParams params;
	std::string current_color = "None";
};

#endif // APP_SETTINGS_H