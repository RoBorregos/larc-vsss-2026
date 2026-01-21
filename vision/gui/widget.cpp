#include "widget.h"
#include "gui.h"

Widget::Widget(GUI *gui, int rowspace, std::string label) {
	this->gui = gui;
	this->rowspace = rowspace;
	this->label = std::move(label);
	this->is_hovered = false;
}

bool Widget::handle_input(int mouse_x, int mouse_y, int event) {
	cv::Rect roi = gui->rowspace_roi(rowspace);
	is_hovered = roi.contains<int>({mouse_x, mouse_y});
	return false;
}

void Widget::set_label(const std::string &new_label) {
	this->label = new_label;
}

void Widget::set_rowspace(int new_rowspace) {
	this->rowspace = new_rowspace;
}

