#ifndef COLOR_SEGMENTATION_CUH
#define COLOR_SEGMENTATION_CUH

#include <opencv2/core/cuda/common.hpp>
#include <opencv2/core/cuda.hpp>
#include <vector>
#include "app_settings.h"

namespace Color_ID {
	constexpr unsigned char NONE    = 0;
	constexpr unsigned char BLUE    = 1;
	constexpr unsigned char YELLOW  = 2;
	constexpr unsigned char CYAN    = 3;
	constexpr unsigned char GREEN   = 4;
	constexpr unsigned char MAGENTA = 5;
	constexpr unsigned char RED     = 6;
	constexpr unsigned char ORANGE  = 7;
}

void upload_color_calibration(const std::vector<ColorCalibration>& data);

cv::cuda::GpuMat launch_color_segmentation(const cv::cuda::GpuMat& hsv,
							   const cv::cuda::GpuMat& mask
							   );

#endif