#ifndef COLOR_SEGMENTATION_CUH
#define COLOR_SEGMENTATION_CUH

#include <opencv2/core/cuda/common.hpp>
#include <opencv2/core/cuda.hpp>
#include <vector>
#include "app_settings.h"

void upload_calibrations_to_gpu(const std::vector<ColorCalibration>& data);

cv::cuda::GpuMat launch_color_segmentation(const cv::cuda::GpuMat& hsv,
							   const cv::cuda::GpuMat& mask
							   );

#endif