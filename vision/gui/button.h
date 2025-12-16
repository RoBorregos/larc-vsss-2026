#ifndef BUTTON_H
#define BUTTON_H

#include <widget.h>
#include <functional>
#include <string>

class Button final : public Widget {
private:
	std::function<void()> callback;
	bool is_pressed = false;

public:
	Button(GUI* gui, int rowspace, std::string label, std::function<void()> callback);

	void draw() override;
	bool handle_input(int mouse_x, int mouse_y, int event) override;
};

#endif //BUTTON_H
