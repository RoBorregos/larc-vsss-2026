#include "camera_calibrator.h"

CameraCalibrator::CameraCalibrator(AppData* app_data, cv::VideoCapture* cap, bool* use_camera_ptr) {
    this->app_data = app_data;
    this->cap = cap;
    this->use_camera_ptr = use_camera_ptr;
}

void CameraCalibrator::reset_parameters() {
    app_data->camera_params = CameraParams();
    upload_to_camera();
}

void CameraCalibrator::upload_to_camera() {
    cap->set(cv::CAP_PROP_BRIGHTNESS, app_data->camera_params.brightness);
    cap->set(cv::CAP_PROP_CONTRAST, app_data->camera_params.contrast);
    cap->set(cv::CAP_PROP_AUTO_EXPOSURE, app_data->camera_params.auto_exposure);
    cap->set(cv::CAP_PROP_EXPOSURE, app_data->camera_params.exposure);
    cap->set(cv::CAP_PROP_AUTOFOCUS, app_data->camera_params.auto_focus);
    cap->set(cv::CAP_PROP_FOCUS, app_data->camera_params.focus);
}

void CameraCalibrator::save_calibration(const std::string& filename) {
    if (app_data == nullptr) {
        std::cerr << "[PANIC] app_data is null in save_calibration! Check initialization." << std::endl;
        std::abort();
    }

    json root;

    std::ifstream infile(filename);

    if (infile.is_open() && infile.peek() != std::ifstream::traits_type::eof()) {
        try {
            infile >> root;
        } catch (json::parse_error& e) {
            std::cerr << "[ERROR] Corrupt JSON: " << filename << ": " << e.what() << std::endl;

            infile.close();

            std::string backupName = filename + ".corrupt";

            std::cerr << "[WARN] Moving corrupt file to '" << backupName
                      << "' and generating new file." << std::endl;

            try {
                std::filesystem::rename(filename, backupName);
            } catch (std::filesystem::filesystem_error& fs_e) {
                std::cerr << "[ERROR] Backupt could not be saved: " << fs_e.what() << std::endl;
                std::cerr << "[FATAL] Cannot safe-keep corrupt config data. Fix permissions or disk space." << std::endl;
                exit(EX_CANTCREAT);
            }

            root = json::object();
        }
    }

    infile.close();
    root["camera_calibration"] = {
        { "brightness",         app_data->camera_params.brightness    },
        { "contrast",           app_data->camera_params.contrast      },
        { "autoexposure",       app_data->camera_params.auto_exposure },
        { "exposure",           app_data->camera_params.exposure      },
        { "autofocus",          app_data->camera_params.auto_focus    },
        { "focus",              app_data->camera_params.focus         }
    };

    std::ofstream outfile(filename);

    if (outfile.is_open()) {
        outfile << root.dump(4);
        outfile.close();
        std::cout << "[INFO] CameraCalibrator saved in: " << filename << std::endl;
    } else {
        std::cerr << "[ERROR] Failed to write in: " << filename << std::endl;
        std::cerr << "[SYSTEM] " << std::strerror(errno) << std::endl;
        std::cerr << "[FATAL] Cannot create output file. Check directory permissions or disk space." << std::endl;

        exit(EX_CANTCREAT);
    }

    upload_to_camera();
}

void CameraCalibrator::load_calibration(const std::string& filename) {
    std::cout << "[DEBUG] Opening file: " << filename << std::endl;

    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "[WARN] Could not open file (Doesn't exists or not enough permissions)." << std::endl;
        return;
    }

    if (file.peek() == std::ifstream::traits_type::eof()) {
        std::cerr << "[ERROR] File exists but ists empty: " << filename << std::endl;
        std::cerr << "[FATAL] Configuration file '" << filename << "' is empty/corrupt. Delete it to regenerate defaults or fix manually." << std::endl;
        exit(EX_CONFIG);
    }

    json root;

    try {
        file >> root;
    } catch (json::parse_error& e) {
        std::cerr << "[ERROR] Corrupt JSON: " << e.what() << std::endl;
        std::cerr << "[FATAL] Configuration file '" << filename
                  << "' contains invalid syntax. Verify structure or delete the file to reset." << std::endl;
        exit(EX_CONFIG);
    }

    if (root.contains("camera_calibration")) {
        auto j_camera = root["camera_calibration"];

        app_data->camera_params.brightness = j_camera.value("brightness", app_data->camera_params.brightness);
        app_data->camera_params.contrast = j_camera.value("contrast", app_data->camera_params.contrast);
        app_data->camera_params.auto_exposure = j_camera.value("autoexposure", app_data->camera_params.auto_exposure);
        app_data->camera_params.exposure = j_camera.value("exposure", app_data->camera_params.exposure);
        app_data->camera_params.auto_focus = j_camera.value("autofocus", app_data->camera_params.auto_focus);
        app_data->camera_params.focus = j_camera.value("focus", app_data->camera_params.focus);

        std::cout << "[INFO] Camera parameters loaded from file." << std::endl;
    } else {
        std::cerr << "[WARN] Camera parameters are not found in file." << std::endl;
        std::cout << "[INFO] Using camera default parameters." << std::endl;
    }

    upload_to_camera();
}
