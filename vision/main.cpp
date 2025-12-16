#include <opencv2/opencv.hpp>
#include "gui.h"
#include "calibrator.h"
#include "interface_manager.h"
#include "app_settings.h"

#define KEY_ESC 27

int main() {
    const std::string path = "/home/iker/Documents/RoboticProjects/VSSS/Vision/media/robot_display.mp4";
    const std::string window_name = "VSSS";
    cv::namedWindow(window_name);

    GUI drawer(window_name);
    Calibrator calibrator(&drawer);
    AppData app_data;

    InterfaceManager interface_manager(&drawer, &calibrator, &app_data);

    cv::VideoCapture cap(path);
    if (!cap.isOpened()) return -1;

    cv::setMouseCallback(window_name, InterfaceManager::on_mouse, &interface_manager);

    cv::Mat image;

    while (true) {
        if (app_data.current_state == AppState::MAIN_MENU ||
            app_data.current_state == AppState::DETECTION) {
            cv::Mat temp;
            cap >> temp;
            if (!temp.empty()) image = temp;
            else {
                cap.set(cv::CAP_PROP_POS_FRAMES, 0);
                cap >> image;
            }
        }

        drawer.upload_frame(image);

        interface_manager.draw_interface();

        drawer.set_clahe_clip_limit(app_data.params.clahe_clip_limit);
        drawer.set_display_saturation(app_data.params.saturation);
        drawer.set_display_gamma_correction(app_data.params.gamma_correction);
        drawer.set_bilateral_sigma(static_cast<int>(app_data.params.bilateral_sigma));
        drawer.set_green_boost(app_data.params.green_boost);
        drawer.set_blue_boost(app_data.params.blue_boost);
        drawer.set_red_boost(app_data.params.red_boost);

        drawer.display_frame();

        if (cv::waitKey(1) == KEY_ESC) break;
    }

    return 0;
}