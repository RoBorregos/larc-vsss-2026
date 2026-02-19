#include <filesystem>
#include <opencv2/opencv.hpp>
#include <opencv2/core/cuda.hpp>
#include <algorithm>
#include <cuda_runtime.h>
#include <string>
#include <chrono>
#include <ctime>
#include "gui.h"
#include "blob_calibrator.h"
#include "interface_manager.h"
#include "app_settings.h"
#include "coordinates.h"
#include "tracker.h"
#include "publisher.h"

#define KEY_ESC 27

bool use_camera = true;
const auto camera_id = "/dev/vsss_cam";
// const std::string file_path = "/media/ikercsv/Files/Projects/larc-vsss-2026/media/image.jpeg";
const std::string file_path = "/media/ikercsv/Files/Projects/larc-vsss-2026/media/log/raw_2026-01-28_18-43-53.avi";
const bool record_video = false;

bool is_video_file(const std::string& path) {
    std::string p = path;
    std::transform(p.begin(), p.end(), p.begin(), ::tolower);
    return (p.find(".mp4") != std::string::npos ||
            p.find(".avi") != std::string::npos ||
            p.find(".mov") != std::string::npos ||
            p.find(".mkv") != std::string::npos);
}

int get_camera_index(const std::string& path) {
    try {
        std::filesystem::path p = std::filesystem::canonical(path);
        std::string filename = p.filename().string();
        if (filename.rfind("video", 0) == 0) {
            return std::stoi(filename.substr(5));
        }
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << std::endl;
    }
    return -1;
}

std::string get_timestamped_filename(std::string prefix) {
    const char* env_path = std::getenv("VSSS_SAVE_PATH");
    std::string base_path = (env_path != nullptr) ? std::string(env_path) : "./media/log/";

    try {
        if (!std::filesystem::exists(base_path)) {
            std::filesystem::create_directories(base_path);
        }
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Filesystem: " << e.what() << std::endl;
    }

    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm* now_tm = std::localtime(&now_time);

    char buffer[100];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d_%H-%M-%S", now_tm);

    return base_path + prefix + "_" + std::string(buffer) + ".avi";
}

int main(int argc, char * argv[]) {
    std::cout << "[INFO] OPENCV VERSION: " << CV_VERSION << std::endl;
    std::cout << "[INFO] CUDA DEVICES: " << cv::cuda::getCudaEnabledDeviceCount() << std::endl;
    std::cout << "[INFO] OPENCV BUILD:" << cv::getBuildInformation() << std::endl;

    rclcpp::init(argc, argv);

    const std::string window_name = "VSSS";
    cv::namedWindow(window_name);

    std::set<int> infield_objects = {1, 3, 4, 12, 17, 18, 20};

    cv::VideoCapture cap;
    cv::VideoWriter raw_writer;
    cv::VideoWriter processed_writer;

    AppData app_data;
    GUI gui(&app_data, window_name);
    Coordinates coordinates(&app_data);
    Tracker tracker(&coordinates, &gui);
    BlobCalibrator blob_calibrator(&gui, &app_data);
    ColorCalibrator color_calibrator(&gui, &app_data);
    CameraCalibrator camera_calibrator(&app_data, &cap, &use_camera);
    Detector detector(&gui, &app_data, &blob_calibrator);
    auto publisher = std::make_shared<Publisher>(&app_data, &tracker);

    std::vector<ColorCalibration> calibrations = blob_calibrator.load_calibration(app_data.calibration_filename);
    color_calibrator.load_calibration(app_data.calibration_filename);
    camera_calibrator.load_calibration(app_data.calibration_filename);

    detector.upload_calibrations(calibrations);
    coordinates.update_matrix();
    tracker.upload_infield_objects(infield_objects);

    InterfaceManager interface_manager(&gui, &blob_calibrator, &color_calibrator, &camera_calibrator, &app_data, &detector, &coordinates);

    cv::Mat image;
    int delay = 1;
    int frame_width = 1920;
    int frame_height = 1080;
    double fps = 60;

    if (use_camera) {
        int index = get_camera_index(camera_id);
        if (index != -1) {
            cap.open(index, cv::CAP_V4L2);
        } else {
            cap.open(0, cv::CAP_V4L2);
        }

        if (!cap.isOpened()) {
            std::cerr << "[ERROR] Could not open camera" << std::endl;
            return -1;
        }

        cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));
        cap.set(cv::CAP_PROP_FRAME_WIDTH, frame_width);
        cap.set(cv::CAP_PROP_FRAME_HEIGHT, frame_height);
        cap.set(cv::CAP_PROP_FPS, fps);

        frame_width = (int)cap.get(cv::CAP_PROP_FRAME_WIDTH);
        frame_height = (int)cap.get(cv::CAP_PROP_FRAME_HEIGHT);
        fps = cap.get(cv::CAP_PROP_FPS);

        std::cout << "fps from camera: " << fps << std::endl;

    } else {
        if (is_video_file(file_path)) {
            cap.open(file_path);
            if (!cap.isOpened()) return -1;
            frame_width = (int)cap.get(cv::CAP_PROP_FRAME_WIDTH);
            frame_height = (int)cap.get(cv::CAP_PROP_FRAME_HEIGHT);
            fps = cap.get(cv::CAP_PROP_FPS);
            delay = (fps > 0) ? static_cast<int>(1000 / fps) : 30;
        } else {
            image = cv::imread(file_path);
            if (image.empty()) return -1;
            frame_width = image.cols;
            frame_height = image.rows;
            fps = 30;
        }
    }

    if (record_video && frame_width > 0 && frame_height > 0) {
        int fourcc = cv::VideoWriter::fourcc('M', 'J', 'P', 'G');

        std::string raw_filename = get_timestamped_filename("raw");
        std::string proc_filename = get_timestamped_filename("processed");

        raw_writer.open(raw_filename, fourcc, fps, cv::Size(frame_width, frame_height), true);
        processed_writer.open(proc_filename, fourcc, fps, cv::Size(frame_width, frame_height), true);
    }

    cv::setMouseCallback(window_name, InterfaceManager::on_mouse, &interface_manager);

    cv::Mat processed_frame_cpu;

    while (rclcpp::ok()) {
	    gui.fps_timer.start();

        if (use_camera) {
            cv::Mat temp;
            cap >> temp;
            if (temp.empty()) break;
            image = temp;
        } else if (cap.isOpened() && !app_data.paused) {
            cv::Mat temp;
            cap >> temp;
            if (!temp.empty()) image = temp;
            else { cap.set(cv::CAP_PROP_POS_FRAMES, 0); cap >> image; }
        }

        if (image.empty()) break;

        if (raw_writer.isOpened()) {
            if (image.size() == cv::Size(frame_width, frame_height)) {
                raw_writer.write(image);
            }
        }

        gui.upload_frame(image);

        std::pair<std::vector<RobotPatch>, std::optional<BallPatch>> detections = detector.update();
        tracker.update(detections.first, detections.second);
        tracker.display_debug_image(image.cols, image.rows);

        interface_manager.draw_interface();

        if (processed_writer.isOpened()) {
            cv::cuda::GpuMat gpu_result = gui.get_image();
            cudaDeviceSynchronize();
            gpu_result.download(processed_frame_cpu);

            if (processed_frame_cpu.size() != cv::Size(frame_width, frame_height)) {
                cv::resize(processed_frame_cpu, processed_frame_cpu, cv::Size(frame_width, frame_height));
            }

            processed_writer.write(processed_frame_cpu);
        }

        gui.display_frame();
        publisher->publish_objects();
        rclcpp::spin_some(publisher);
        if (cv::waitKey(delay) == KEY_ESC) break;
    }

    std::cout << "[INFO] Closing resources..." << std::endl;
    if (cap.isOpened()) cap.release();
    if (raw_writer.isOpened()) raw_writer.release();
    if (processed_writer.isOpened()) processed_writer.release();
    cv::destroyAllWindows();
    rclcpp::shutdown();

    return 0;
}