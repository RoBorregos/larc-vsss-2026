#ifndef WIDGET_H
#define WIDGET_H

#include <string>

class GUI;

class Widget {
protected:
	GUI* gui;
	int rowspace;
	std::string label;
	bool is_hovered;
public:
	Widget(GUI* gui, int rowspace, std::string label);
	virtual ~Widget() = default;

	virtual void draw() = 0;
	virtual bool handle_input(int mouse_x, int mouse_y, int event);
	void set_label(const std::string& new_label);
	void set_rowspace(int new_rowspace);
};

#endif //WIDGET_H
