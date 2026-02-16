#pragma once

#include <string>
#include <functional>

class GUI;

class Widget {
protected:
	GUI* gui;
	int rowspace;
	std::string label;
	bool is_hovered;

	std::function<void()> on_change_callback;
public:
	Widget(GUI* gui, int rowspace, std::string label, std::function<void()> callback = nullptr);
	virtual ~Widget() = default;

	virtual void draw() = 0;
	virtual bool handle_input(int mouse_x, int mouse_y, int event);
	void set_label(const std::string& new_label);
	void set_rowspace(int new_rowspace);
	void attach_callback(std::function<void()> callback);
};
