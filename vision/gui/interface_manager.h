#ifndef INTERFACE_MANAGER_H
#define INTERFACE_MANAGER_H

#include <vector>
#include <map>
#include <memory>
#include <iostream>
#include <utility>
#include "gui.h"
#include "widget.h"
#include "button.h"
#include "slider.h"
#include "blob_calibrator.h"
#include "app_settings.h"
#include "color_calibrator.h"
#include "detector.h"

class InterfaceManager {
private:
	GUI* drawer;
	BlobCalibrator* blob_calibrator;
	ColorCalibrator* color_calibrator;
	AppData* app_data;
	Detector* detector;

	Button* pause_button = nullptr;
	std::vector<std::unique_ptr<Widget>> main_menu_widgets;
	std::vector<std::unique_ptr<Widget>> blob_calibration_menu_widgets;
	std::vector<std::unique_ptr<Widget>> blob_calibration_tool_widgets;
	std::vector<std::unique_ptr<Widget>> color_calibration_tool_widgets;
	std::vector<std::unique_ptr<Widget>> mask_calibration_tool_widgets;
	std::vector<std::unique_ptr<Widget>> roi_calibration_tool_widgets;

	std::map<std::string, Button*> color_buttons;
	std::optional<CalibrationResult> last_calculated_result;

	void init_widgets();
	void save_current_blob_calibration();
public:
	InterfaceManager(GUI* drawer, BlobCalibrator* blob_calibrator, ColorCalibrator* color_calibrator, AppData* app_data, Detector* detector);
	~InterfaceManager() = default;

	void draw_interface();
	static void on_mouse(int event, int x, int y, int flags, void* userdata);
	void handle_input(int event, int x, int y);
};

#endif // INTERFACE_MANAGER_H