#include "toggle.h"

Toggle::Toggle(GUI* gui, int rowspace, std::string label, bool* value_ptr, std::function<void()> callback): Widget(gui, rowspace, std::move(label), std::move(callback)) {

    this->value_ptr = value_ptr;
}

void Toggle::draw() {
    const cv::Rect roi = gui->rowspace_roi(rowspace);
    gui->free_rowspace(rowspace);

    cv::Mat& frame = gui->get_frame();

    cv::rectangle(frame, roi, cv::Scalar(30, 30, 30), cv::FILLED);

    const int left_padding = 10;
    const int toggle_width = roi.width / 4;
    border_radius = roi.height / 4;

    const int start_x = roi.x + left_padding + border_radius;
    const int end_x = start_x + toggle_width;
    const int start_y = roi.y;
    const int center_y = roi.y + roi.height / 2;
    const int knob_radius = border_radius - 5;

    knob_x = start_x + toggle_width * (static_cast<int>(*value_ptr));
    knob_y = center_y;
    cv::Scalar knob_color = *(value_ptr) ? cv::Scalar(89, 199, 52) : cv::Scalar(48, 59, 255);

    cv::Rect toggle_body(start_x, start_y, toggle_width, roi.height);
    cv::rectangle(frame, toggle_body, cv::Scalar(20, 20, 20), cv::FILLED);

    gui->text(label, rowspace, Drawer::Layer::INTERFACE, 0.4, false);

    cv::circle(frame, cv::Point(end_x, center_y), border_radius * 2, cv::Scalar(20, 20, 20), cv::FILLED);
    cv::circle(frame, cv::Point(start_x, center_y), border_radius * 2, cv::Scalar(20, 20, 20), cv::FILLED);
    cv::circle(frame, cv::Point(knob_x, center_y), knob_radius * 2, knob_color, cv::FILLED);
}

bool Toggle::handle_input(int mouse_x, int mouse_y, int event) {
    const int dx = mouse_x - knob_x;
    const int dy = mouse_y - knob_y;
    const bool knob_pressed = (dx * dx) + (dy * dy) <= (border_radius * border_radius);

    if (event == cv::EVENT_LBUTTONDOWN && knob_pressed) {
        *value_ptr = !(*value_ptr);
        if (on_change_callback) on_change_callback();
        return true;
    }

    Widget::handle_input(mouse_x, mouse_y, event);
    return knob_pressed;
}
