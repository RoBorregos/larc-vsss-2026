#include <opencv2/opencv.hpp>
#include "gui.h"
#include "blob_calibrator.h"
#include "interface_manager.h"
#include "app_settings.h"
#include "robot.h"

#define KEY_ESC 27

int main() {
    std::cout << "USING OPENCV VERSION: " << CV_VERSION << std::endl;

    const std::string path = "/home/iker/Documents/RoboticProjects/VSSS/vision/media/robot_display.mp4";
    const std::string window_name = "VSSS";
    cv::namedWindow(window_name);

    AppData app_data;
    GUI drawer(&app_data, window_name);
    BlobCalibrator blob_calibrator(&drawer, &app_data);
    ColorCalibrator color_calibrator(&drawer, &app_data);
    Detector detector(&drawer, &app_data);

    InterfaceManager interface_manager(&drawer, &blob_calibrator, &color_calibrator, &app_data, &detector);

    Robot robot_1(&drawer, &app_data, &detector);
    robot_1.initialize(1, TeamColor::BLUE, {PatchColor::CYAN, PatchColor::GREEN});

    cv::VideoCapture cap(path);
    if (!cap.isOpened()) return -1;
    double fps = cap.get(cv::CAP_PROP_FPS);
    int delay = (fps > 0) ? static_cast<int>(1000 / fps) : 30;

    cv::setMouseCallback(window_name, InterfaceManager::on_mouse, &interface_manager);
    cv::Mat image;

    while (true) {
        if (!app_data.paused) {
            cv::Mat temp;
            cap >> temp;
            if (!temp.empty()) image = temp;
            else {
                cap.set(cv::CAP_PROP_POS_FRAMES, 0);
                cap >> image;
            }
        }
        drawer.upload_frame(image);

        // if (app_data.current_state == AppState::DETECTION) {
        detector.update();
        robot_1.update();
        // }


        interface_manager.draw_interface();

        drawer.set_clahe_clip_limit(app_data.color_params.clahe_clip_limit);
        drawer.set_display_saturation(app_data.color_params.saturation);
        drawer.set_display_gamma_correction(app_data.color_params.gamma_correction);
        drawer.set_bilateral_sigma(static_cast<int>(app_data.color_params.bilateral_sigma));
        drawer.set_green_boost(app_data.color_params.green_boost);
        drawer.set_blue_boost(app_data.color_params.blue_boost);
        drawer.set_red_boost(app_data.color_params.red_boost);

        drawer.display_frame();

        if (cv::waitKey(delay) == KEY_ESC) break;
    }

    return 0;
}