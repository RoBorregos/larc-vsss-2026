#include "color_segmentation.cuh"
#include <opencv2/core/cuda/common.hpp>
#include <opencv2/core/cuda/vec_traits.hpp>
#include <opencv2/core/cuda/vec_math.hpp>
#include <cuda_runtime.h>
#include <cstdio>

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

	// bool debug_pixel = (x == 236 && y == 224);
	bool debug_pixel = false;

	if (debug_pixel) {
		int h = hsv_image(y, x).x;
		int s = hsv_image(y, x).y;
		int v = hsv_image(y, x).z;

		printf("\n[DEBUG GPU] Pixel(%d, %d) HSV: %d, %d, %d\n", x, y, h, s, v);
	}

	if (mask(y, x) == Color_ID::NONE) {
		output(y, x) = Color_ID::NONE;
		if (debug_pixel) printf("  -> Pixel empty in mask, skipping...\n");
		return;
	}

	const uchar3 p = hsv_image(y, x);
	const float3 pixel = make_float3(p.x, p.y, p.z);

	float min_dist = 1e9f;
	uchar best_id = Color_ID::NONE;

	for (int i = 0; i < c_num_colors; ++i) {
		const float3 mean = c_means[i];
		float3 weights = c_weights[i];

		float diff_h = fabsf(pixel.x - mean.x);
		if (diff_h > 90.0f) diff_h = 180.0f - diff_h;

		const float dh = diff_h;
		const float ds = pixel.y - mean.y;
		const float dv = pixel.z - mean.z;

		float distance = (dh * dh) + (ds * ds) + (dv * dv);

		if (debug_pixel) {
			printf("  -> Comparando con Color[%d] (Mean: %.1f, %.1f, %.1f): Distancia Calc = %.2f\n",
				   i, mean.x, mean.y, mean.z, distance);
		}

		if (distance < min_dist) {
			min_dist = distance;
			best_id = c_ids[i];
		}
	}

	if (min_dist > max_distance) {
		output(y, x) = Color_ID::NONE;
		if (debug_pixel) printf("  -> Resultado: NONE (Dist %.2f > Max %.2f)\n", min_dist, max_distance);
	} else {
		output(y, x) = best_id;
		if (debug_pixel) printf("  -> Resultado: ID %d (Dist %.2f)\n", best_id, min_dist);
	}
}

void upload_calibrations_to_gpu(const std::vector<ColorCalibration>& data) {
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

	float max_dist = 1000.0f;

	fflush(stdout);
	knc_weighted_kernel<<<grid, block>>>(hsv, mask, internal_buffer, max_dist*max_dist);
	cudaError_t error = cudaDeviceSynchronize();

	return internal_buffer;
}