#ifndef OPENCV_CAMERA_CALIBRATOR_H
#define OPENCV_CAMERA_CALIBRATOR_H

#include <fstream>
#include <opencv2/opencv.hpp>
#include <cstring>
#include <cerrno>
#include <sysexits.h>
#include "json.hpp"
#include "app_settings.h"

using json = nlohmann::json;

class CameraCalibrator {
private:
    AppData* app_data;
    cv::VideoCapture* cap;

    bool properties_modified = false;
    bool* use_camera_ptr;

public:
    CameraCalibrator(AppData* app_data, cv::VideoCapture* cap, bool* use_camera_ptr);
    void upload_to_camera();

    void reset_parameters();

    void save_calibration(const std::string& filename);
    void load_calibration(const std::string& filename);
};


#endif //OPENCV_CAMERA_CALIBRATOR_H