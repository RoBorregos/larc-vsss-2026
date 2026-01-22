//
// Created by kali on 3/24/24.
//

#ifndef OPENCV_IMAGE_PREPROCESSING_H
#define OPENCV_IMAGE_PREPROCESSING_H

#include <opencv4/opencv2/opencv.hpp>
#include <opencv4/opencv2/cudaarithm.hpp>
#include <opencv2/cudaimgproc.hpp>
#include <opencv2/cudawarping.hpp>
#include "app_settings.h"

namespace preprocessing {
	void filters(const cv::cuda::GpuMat& bgr_image_src, cv::cuda::GpuMat& bgr_image_out, cv::cuda::GpuMat& hsv_image_out, const VisionParams& params);
	void resize(cv::cuda::GpuMat& input_image, int target_width, int target_height);
	void resize(cv::Mat& input_image, int target_width, int target_height);
}

#endif //OPENCV_IMAGE_PREPROCESSING_H
