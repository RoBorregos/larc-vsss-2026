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
    std::cout << "[INFO] USING OPENCV VERSION: " << CV_VERSION << std::endl;
    std::cout << "[INFO] USING " << cv::cuda::getCudaEnabledDeviceCount() << " CUDA DEVICES" << std::endl;

    const std::string path = "/home/iker/Documents/RoboticProjects/VSSS/media/image.jpeg";
    // const std::string path = "/home/iker/Documents/RoboticProjects/VSSS/media/robot_display.mp4";

    const std::string window_name = "VSSS";
    cv::namedWindow(window_name);

    AppData app_data;
    GUI gui(&app_data, window_name);
    BlobCalibrator blob_calibrator(&gui, &app_data);
    ColorCalibrator color_calibrator(&gui, &app_data);
    Detector detector(&gui, &app_data, &blob_calibrator);

    blob_calibrator.load_calibration(app_data.calibration_filename);
    color_calibrator.load_calibration(app_data.calibration_filename);

    InterfaceManager interface_manager(&gui, &blob_calibrator, &color_calibrator, &app_data, &detector);

    Robot robot_1(&gui, &app_data, &detector);
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
        delay = (fps > 0) ? static_cast<int>(1000 / fps) : 10;
        std::cout << "[INFO] Modo VIDEO detectado. FPS: " << fps << std::endl;
    } else {
        image = cv::imread(path);
        delay = 1;
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

        gui.upload_frame(image);

        // if (app_data.current_state == AppState::DETECTION) {
            // detector.update();
            // robot_1.update();
            // detector.display_debug_info();
        // }

        interface_manager.draw_interface();

        gui.display_frame();

        if (cv::waitKey(delay) == KEY_ESC) break;
    }

    return 0;
}