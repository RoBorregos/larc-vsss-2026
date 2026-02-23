#pragma once

#include <functional>
#include <widget.h>
#include <string>

class Slider final : public Widget {
public:
	float* value_ptr;
	float min_v, max_v;
	bool is_dragging = false;

	Slider(GUI* gui, int rowspace, std::string label, float* value_ptr, float min_v, float max_v, std::function<void()> callback = nullptr);

	void draw() override;
	bool handle_input(int mouse_x, int mouse_y, int event) override;
};
