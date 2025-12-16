#ifndef INTERFACE_MANAGER_H
#define INTERFACE_MANAGER_H

#include <vector>
#include <map>
#include <memory>
#include "gui.h"
#include "widget.h"
#include "button.h"
#include "slider.h"
#include "calibrator.h"
#include "app_settings.h"

class InterfaceManager {
private:
	GUI* drawer;
	Calibrator* calibrator;
	AppData* app_data;

	std::vector<std::unique_ptr<Widget>> main_menu_widgets;
	std::vector<std::unique_ptr<Widget>> calibration_menu_widgets;
	std::vector<std::unique_ptr<Widget>> calibration_tool_widgets;

	MatchCalibration match_calibration;
	std::map<std::string, Button*> color_buttons;
	std::optional<CalibrationResult> last_calculated_result;

	void init_widgets();
	void save_current_calibration();
public:
	InterfaceManager(GUI* drawer, Calibrator* calibrator, AppData* app_data);
	~InterfaceManager() = default;

	void draw_interface();
	static void on_mouse(int event, int x, int y, int flags, void* userdata);
	void handle_input(int event, int x, int y);

	[[nodiscard]] MatchCalibration get_calibration() const { return match_calibration; }
};

#endif // INTERFACE_MANAGER_H