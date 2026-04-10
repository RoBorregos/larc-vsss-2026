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
#include "ros_handler.h"

#define KEY_ESC 27

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

inline void noop() {};

int main(int argc, char * argv[]) {
    std::cout << "[INFO] OPENCV VERSION: " << CV_VERSION << std::endl;
    std::cout << "[INFO] CUDA DEVICES: " << cv::cuda::getCudaEnabledDeviceCount() << std::endl;

    rclcpp::init(argc, argv);

    AppData app_data;
    Drawer drawer;
    Coordinates coordinates(&app_data);
    const std::string window_name = "VSSS";
    cv::namedWindow(window_name);
    GUI gui(&app_data, &drawer, window_name);
    Tracker tracker(&coordinates, &gui, &drawer, &app_data);
    auto ros_handler = std::make_shared<RosHandler>(&app_data, &tracker);

    ros_handler->declare_parameter("use_camera", false);
    ros_handler->declare_parameter("simulator", false);
    ros_handler->declare_parameter("record_video", false);
    ros_handler->declare_parameter("debug_mode", false);
    ros_handler->declare_parameter("camera_id", "/dev/vsss_cam");
    ros_handler->declare_parameter("file_path", "/ros2_ws/vsss/vsss_ws/src/vision/media/log/raw_2026-02-24_04-18-37.avi");


    bool use_camera = ros_handler->get_parameter("use_camera").as_bool();
    app_data.simulator = ros_handler->get_parameter("simulator").as_bool();
    app_data.debug_mode = ros_handler->get_parameter("debug_mode").as_bool();
    bool record_video = ros_handler->get_parameter("record_video").as_bool();
    std::string camera_id = ros_handler->get_parameter("camera_id").as_string();
    std::string file_path = ros_handler->get_parameter("file_path").as_string();

    app_data.calibration_filename = app_data.get_calibration_path(app_data.simulator);

    RCLCPP_INFO(ros_handler->get_logger(), "MODE: %s | RECORDING: %s",
                app_data.simulator ? "SIMULATOR" : (use_camera ? "CAMERA" : "FILE"),
                record_video ? "ON" : "OFF");

    std::set<int> infield_objects = {0, 1, 2, 10, 11, 12, 20};
    cv::VideoCapture cap;
    cv::VideoWriter raw_writer;
    cv::VideoWriter processed_writer;

    BlobCalibrator blob_calibrator(&gui, &app_data);
    ColorCalibrator color_calibrator(&gui, &app_data);
    CameraCalibrator camera_calibrator(&app_data, &cap, &use_camera);
    Detector detector(&gui, &app_data, &blob_calibrator, ros_handler);

    rclcpp::executors::SingleThreadedExecutor executor;
    executor.add_node(ros_handler);

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
        cap.open((index != -1) ? index : 0, cv::CAP_V4L2);

        if (!cap.isOpened()) {
            RCLCPP_ERROR(ros_handler->get_logger(), "Could not open camera!");
            return -1;
        }

        cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));
        cap.set(cv::CAP_PROP_FRAME_WIDTH, frame_width);
        cap.set(cv::CAP_PROP_FRAME_HEIGHT, frame_height);
        cap.set(cv::CAP_PROP_FPS, fps);

        frame_width = (int)cap.get(cv::CAP_PROP_FRAME_WIDTH);
        frame_height = (int)cap.get(cv::CAP_PROP_FRAME_HEIGHT);
        fps = cap.get(cv::CAP_PROP_FPS);

    } else if (app_data.simulator) {
        noop();
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
        raw_writer.open(get_timestamped_filename("raw"), fourcc, fps, cv::Size(frame_width, frame_height), true);
        processed_writer.open(get_timestamped_filename("processed"), fourcc, fps, cv::Size(frame_width, frame_height), true);
    }

    cv::setMouseCallback(window_name, InterfaceManager::on_mouse, &interface_manager);
    cv::Mat processed_frame_cpu;

    app_data.fps = fps;

    while (rclcpp::ok()) {
        gui.fps_timer.start();

        if (use_camera) {
            cap >> image;
        } else if (app_data.simulator) {
            cv::Mat sim_frame = ros_handler->get_latest_frame();
            if (!sim_frame.empty()) {
                image = sim_frame;
                frame_width = image.cols;
                frame_height = image.rows;
            }
        } else if (cap.isOpened() && !app_data.paused) {
            cap >> image;
            if (image.empty()) { // Loop video
                cap.set(cv::CAP_PROP_POS_FRAMES, 0);
                cap >> image;
            }
        }

        if (image.empty()) {
            if (app_data.simulator) { executor.spin_some(); continue; }
            break;
        };

        if (raw_writer.isOpened()) raw_writer.write(image);

        gui.upload_frame(image);
        auto detections = detector.update();
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
        ros_handler->publish_objects();
        executor.spin_some();
        if (cv::waitKey(delay) == KEY_ESC) break;
    }

    if (cap.isOpened()) cap.release();
    if (raw_writer.isOpened()) raw_writer.release();
    if (processed_writer.isOpened()) processed_writer.release();
    cv::destroyAllWindows();
    rclcpp::shutdown();
    return 0;
}