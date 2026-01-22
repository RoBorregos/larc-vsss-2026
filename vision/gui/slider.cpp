#include "slider.h"

#include <gui.h>

Slider::Slider(GUI *gui, const int rowspace, std::string label, float *value_ptr, float min_v, float max_v): Widget(gui,
rowspace, std::move(label)) {

	this->value_ptr = value_ptr;
	this->min_v = min_v;
	this->max_v = max_v;
}

void Slider::draw() {
	const cv::Rect roi = gui->rowspace_roi(rowspace);
	gui->free_rowspace(rowspace);

	cv::Mat& frame = gui->get_frame();

	cv::rectangle(frame, roi, cv::Scalar(30, 30, 30), cv::FILLED);

	const float normalized = (*value_ptr - min_v) / (max_v - min_v);
	const int bar_width = static_cast<int>(roi.width * 0.8);
	int start_x = roi.x + (roi.width - bar_width) / 2;
	int knob_x = start_x + static_cast<int>(normalized * (bar_width * 1.0));
	int center_y = roi.y + roi.height / 2 + 10;

	gui->text(label, rowspace, Drawer::Layer::INTERFACE, 0.4, false, -10);

	const cv::Scalar knob_color = (is_hovered || is_dragging) ? cv::Scalar(0, 255, 255) : cv::Scalar(0, 200, 200);

	gui->line({start_x, center_y}, {start_x + bar_width, center_y}, Drawer::Layer::INTERFACE, 4, cv::Scalar(255, 0, 0));
	gui->solid_circle({knob_x, center_y}, 8, Drawer::Layer::INTERFACE, knob_color);
}

bool Slider::handle_input(int mouse_x, int mouse_y, const int event) {
	cv::Rect roi = gui->rowspace_roi(rowspace);

	if (event == cv::EVENT_LBUTTONDOWN && roi.contains<int>({mouse_x, mouse_y})) {
		is_dragging = true;
	} else if (event == cv::EVENT_LBUTTONUP) {
		is_dragging = false;
	}

	if (!is_hovered) {
		is_dragging = false;
	}

	if (is_dragging) {
		int bar_width = static_cast<int> (roi.width * 0.8);
		int start_x = roi.x + (roi.width - bar_width) / 2;

		float ratio = static_cast<float>(mouse_x - start_x) / static_cast<float>(bar_width);
		ratio = std::clamp(ratio, 0.0f, 1.0f);
		*value_ptr = min_v + ratio * (max_v - min_v);

		return true;
	}

	Widget::handle_input(mouse_x, mouse_y, event);
	return is_dragging;
}
