#include "button.h"
#include "gui.h"

Button::Button(GUI *gui, const int rowspace, std::string label, std::function<void()> callback): Widget(gui, rowspace,
std::move(label)) {
	this->callback = std::move(callback);
}

void Button::draw() {
	if (!gui) return;

	gui->free_rowspace(rowspace);
	const cv::Rect roi = gui->rowspace_roi(rowspace);

	cv::Mat& frame = gui->get_frame();
	const cv::Scalar bg_color = is_hovered ? cv::Scalar(100, 100, 100) : cv::Scalar(50, 50, 50);

	cv::rectangle(frame, roi, bg_color, cv::FILLED);
	gui->text(label, rowspace, Drawer::Layer::INTERFACE, 0.7, false);
}

bool Button::handle_input(int x, int y, int event) {
	Widget::handle_input(x, y, event);

	if (is_hovered && event == cv::EVENT_LBUTTONDOWN) {
		if (callback) {
			callback();
		}
		return true;
	}

	return false;
}