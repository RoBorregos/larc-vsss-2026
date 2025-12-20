#include <opencv2/opencv.hpp>
#include <algorithm>
#include <string>
#include "gui.h"
#include "blob_calibrator.h"
#include "interface_manager.h"
#include "app_settings.h"
#include "robot.h"

#define KEY_ESC 27

bool is_video_file(const std::string& path) {
    std::string p = path;
    std::transform(p.begin(), p.end(), p.begin(), ::tolower);

    if (p.find(".mp4") != std::string::npos ||
        p.find(".avi") != std::string::npos ||
        p.find(".mov") != std::string::npos ||
        p.find(".mkv") != std::string::npos) {
        return true;
    }
    return false;
}

int main() {
    std::cout << "USING OPENCV VERSION: " << CV_VERSION << std::endl;

    const std::string path = "/home/iker/Documents/RoboticProjects/VSSS/media/image.jpeg";
    // const std::string path = "/home/iker/Documents/RoboticProjects/VSSS/media/robot_display.mp4";

    const std::string window_name = "VSSS";
    cv::namedWindow(window_name);

    AppData app_data;
    GUI drawer(&app_data, window_name);
    BlobCalibrator blob_calibrator(&drawer, &app_data);
    ColorCalibrator color_calibrator(&drawer, &app_data);
    Detector detector(&drawer, &app_data, &blob_calibrator);

    blob_calibrator.load_calibration(app_data.calibration_filename);
    color_calibrator.load_calibration(app_data.calibration_filename);

    InterfaceManager interface_manager(&drawer, &blob_calibrator, &color_calibrator, &app_data, &detector);

    Robot robot_1(&drawer, &app_data, &detector);
    robot_1.initialize(1, TeamColor::BLUE, {PatchColor::CYAN, PatchColor::GREEN});

    bool is_video = is_video_file(path);
    cv::VideoCapture cap;
    cv::Mat image;
    int delay = 30;

    if (is_video) {
        cap.open(path);
        if (!cap.isOpened()) {
            std::cerr << "[ERROR] No se pudo abrir el video: " << path << std::endl;
            return -1;
        }
        double fps = cap.get(cv::CAP_PROP_FPS);
        delay = (fps > 0) ? static_cast<int>(1000 / fps) : 30;
        std::cout << "[INFO] Modo VIDEO detectado. FPS: " << fps << std::endl;
    } else {
        image = cv::imread(path);
        if (image.empty()) {
            std::cerr << "[ERROR] No se pudo abrir la imagen: " << path << std::endl;
            return -1;
        }
        std::cout << "[INFO] Modo IMAGEN detectado." << std::endl;
    }

    cv::setMouseCallback(window_name, InterfaceManager::on_mouse, &interface_manager);

    while (true) {
        if (is_video && !app_data.paused) {
            cv::Mat temp;
            cap >> temp;
            if (!temp.empty()) {
                image = temp;
            } else {
                cap.set(cv::CAP_PROP_POS_FRAMES, 0);
                cap >> image;
            }
        }

        if (image.empty()) break;

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