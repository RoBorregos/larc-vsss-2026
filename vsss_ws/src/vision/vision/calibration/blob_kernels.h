#pragma once
#include <opencv2/core/cuda.hpp>
#include "blob_stats.h"

void launchBlobKernel(cv::cuda::GpuMat& hsv, cv::cuda::GpuMat& mask, BlobStats* d_stats);