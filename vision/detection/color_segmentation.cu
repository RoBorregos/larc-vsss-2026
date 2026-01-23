#include "color_segmentation.cuh"
#include <opencv2/core/cuda/common.hpp>
#include <opencv2/core/cuda/vec_traits.hpp>
#include <opencv2/core/cuda/vec_math.hpp>
#include <cuda_runtime.h>

#define MAX_COLORS 16

__constant__ float3 c_means[MAX_COLORS];
__constant__ float3 c_weights[MAX_COLORS];
__constant__ uchar  c_ids[MAX_COLORS];
__constant__ int    c_num_colors;

__global__ void knc_weighted_kernel(
	const cv::cuda::PtrStepSz<uchar3> hsv_image,
	const cv::cuda::PtrStepSz<uchar> mask,
	cv::cuda::PtrStepSz<uchar> output,
	float max_distance
) {
	int x = blockIdx.x * blockDim.x + threadIdx.x;
	int y = blockIdx.y * blockDim.y + threadIdx.y;

	if (x >= hsv_image.cols || y >= hsv_image.rows) return;

	if (mask(y, x) == Color_ID::NONE) {
		output(y, x) == Color_ID::NONE;
		return;
	}

	const uchar3 p = hsv_image(y, x);
	const float3 pixel = make_float3(p.x, p.y, p.z);

	float min_dist = 1e9f;
	uchar best_id = Color_ID::NONE;

	for (int i = 0; i < c_num_colors; ++i) {
		const float3 mean = c_means[i];
		float3 weights = c_weights[i];

		const float dh = pixel.x - mean.x;
		const float ds = pixel.y - mean.y;
		const float dv = pixel.z - mean.z;

		float distance = (dh * dh * weights.x) + (ds * ds * weights.y) + (dv * dv * weights.z);

		if (distance < min_dist) {
			min_dist = distance;
			best_id = c_ids[i];
		}
	}

	if (min_dist > max_distance) {
		best_id = Color_ID::NONE;
	} else {
		output(y, x) = best_id;
	}
}

void upload_color_calibration(const std::vector<ColorCalibration>& data) {
	int n = data.size();
	if (n > MAX_COLORS) n = MAX_COLORS;

	std::vector<uchar>  h_ids(n);
	std::vector<float3> h_means(n);
	std::vector<float3> h_weights(n);

	for(int i = 0; i < n; i++) {
		const auto& item = data[i];

		h_ids[i] = (uchar)item.id;

		h_means[i] = make_float3(
			(float)item.hsv_avg[0],
			(float)item.hsv_avg[1],
			(float)item.hsv_avg[2]
		);

		float sH = (float)item.hsv_stddev[0];
		float sS = (float)item.hsv_stddev[1];
		float sV = (float)item.hsv_stddev[2];

		h_weights[i].x = 1.0f / (sH * sH + 1e-6f);
		h_weights[i].y = 1.0f / (sS * sS + 1e-6f);
		h_weights[i].z = 1.0f / (sV * sV + 1e-6f);
	}

	cudaMemcpyToSymbol(c_ids, h_ids.data(), n * sizeof(uchar));
	cudaMemcpyToSymbol(c_means, h_means.data(), n * sizeof(float3));
	cudaMemcpyToSymbol(c_weights, h_weights.data(), n * sizeof(float3));
	cudaMemcpyToSymbol(c_num_colors, &n, sizeof(int));
}

cv::cuda::GpuMat launch_color_segmentation(const cv::cuda::GpuMat& hsv, const cv::cuda::GpuMat& mask) {

	static cv::cuda::GpuMat internal_buffer;

	internal_buffer.create(hsv.size(), CV_8UC1);

	dim3 block(32, 32);
	dim3 grid(
	   (hsv.cols + block.x - 1) / block.x,
	   (hsv.rows + block.y - 1) / block.y
	);

	float max_dist_sq = 60.0f * 60.0f;

	knc_weighted_kernel<<<grid, block>>>(hsv, mask, internal_buffer, max_dist_sq);

	return internal_buffer;
}