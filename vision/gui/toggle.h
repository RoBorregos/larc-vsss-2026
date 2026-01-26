#ifndef OPENCV_TOGGLE_H
#define OPENCV_TOGGLE_H

#include <functional>
#include <widget.h>
#include <string>
#include <gui.h>

class Toggle final : public Widget {
public:
    bool* value_ptr;

    Toggle(GUI* gui, int rowspace, std::string label, bool* value_ptr, std::function<void()> callback = nullptr);

    void draw() override;
    bool handle_input(int mouse_x, int mouse_y, int event) override;
private:
    int knob_x{};
    int knob_y{};
    int border_radius{};
};


#endif //OPENCV_TOGGLE_H