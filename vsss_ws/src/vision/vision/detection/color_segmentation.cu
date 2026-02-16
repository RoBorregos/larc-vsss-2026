#include "color_segmentation.cuh"
#include <opencv2/core/cuda/common.hpp>
#include <opencv2/core/cuda/vec_traits.hpp>
#include <opencv2/core/cuda/vec_math.hpp>
#include <cuda_runtime.h>
#include <cstdio>

#define MAX_COLORS 256

__constant__ float3 c_means[MAX_COLORS];
__constant__ uchar  c_ids[MAX_COLORS];
__constant__ int    c_num_colors;

__global__ void knc_dynamic_k_kernel(
    const cv::cuda::PtrStepSz<uchar3> hsv_image,
    const cv::cuda::PtrStepSz<uchar> mask,
    cv::cuda::PtrStepSz<uchar> output,
    float max_distance,
    int k_param
) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x >= hsv_image.cols || y >= hsv_image.rows) return;

    const float w_h = 4.5f;
    const float w_s = 1.0f;
    const float w_v = 1.5f;

    if (mask(y, x) == Color_ID::NONE) {
       output(y, x) = Color_ID::NONE;
       return;
    }

    const uchar3 p = hsv_image(y, x);
    const float3 pixel = make_float3(p.x, p.y, p.z);

    const int MAX_K = 9;

    int k = (k_param > MAX_K) ? MAX_K : (k_param < 1 ? 1 : k_param);

    float best_dist[MAX_K];
    uchar best_ids[MAX_K];

    #pragma unroll
    for (int i = 0; i < MAX_K; ++i) {
        best_dist[i] = 1e9f;
        best_ids[i] = Color_ID::NONE;
    }

    for (int i = 0; i < c_num_colors; ++i) {
       const float3 mean = c_means[i];

       float diff_h = fabsf(pixel.x - mean.x);
       if (diff_h > 90.0f) diff_h = 180.0f - diff_h;

       const float dh = diff_h;
       const float ds = pixel.y - mean.y;
       const float dv = pixel.z - mean.z;

       float distance = (w_h * dh * dh) + (w_s * ds * ds) + (w_v * dv * dv);

       if (distance < max_distance) {
          for (int j = 0; j < k; ++j) {
             if (distance < best_dist[j]) {
                for (int shift = k - 1; shift > j; --shift) {
                   best_dist[shift] = best_dist[shift-1];
                   best_ids[shift] = best_ids[shift-1];
                }
                best_dist[j] = distance;
                best_ids[j] = c_ids[i];
                break;
             }
          }
       }
    }

    uchar final_id = Color_ID::NONE;
    if (best_ids[0] != Color_ID::NONE) {
        if (k == 1) {
            final_id = best_ids[0];
        } else {
            int max_votes = 0;
            final_id = best_ids[0];

            for (int i = 0; i < k; ++i) {
                if (best_ids[i] == Color_ID::NONE) break;

                int current_id = best_ids[i];
                int current_votes = 1;

                for (int j = i + 1; j < k; ++j) {
                    if (best_ids[j] == current_id) {
                        current_votes++;
                    }
                }

                if (current_votes > max_votes) {
                    max_votes = current_votes;
                    final_id = current_id;
                }
            }
        }
    }

    output(y, x) = final_id;
}

__global__ void label_moore_neighborhood(
	const cv::cuda::PtrStepSz<uchar>& label_mask,
	cv::cuda::PtrStepSz<uchar> object_mask,
	int min_required
) {
	int x = blockIdx.x * blockDim.x + threadIdx.x;
	int y = blockIdx.y * blockDim.y + threadIdx.y;

	if (x >= label_mask.cols || y >= label_mask.rows) return;

	if (x == 0 || x == label_mask.cols - 1 || y == 0 || y == label_mask.rows - 1) {
		object_mask(y, x) = Color_ID::NONE;
		return;
	}

	uchar color = label_mask(y, x);

	if (color == Color_ID::NONE) {
		object_mask(y, x) = Color_ID::NONE;
		return;
	}

	int count = 0;
	#pragma unroll
	for (int dy = -1; dy <= 1; ++dy) {
		#pragma unroll
		for (int dx = -1; dx <= 1; ++dx) {
			if (dx == 0 && dy == 0) continue;

			if (label_mask(y + dy, x + dx) == color) ++count;
		}
	}

	if (count >= min_required) {
		object_mask(y, x) = color;
	} else {
		object_mask(y, x) = Color_ID::NONE;
	}
}

__global__ void map_group_to_color(
	const cv::cuda::PtrStepSz<int>& labels,
	const cv::cuda::PtrStepSz<uchar>& colors,
	uchar* lookup_table
) {
	int x = blockIdx.x * blockDim.x + threadIdx.x;
	int y = blockIdx.y * blockDim.y + threadIdx.y;

	if (x >= labels.cols || y >= labels.rows) return;

	int groupId = labels(y, x);
	if (groupId > 0) {
		lookup_table[groupId] = colors(y, x);
	}
}

void upload_calibrations_to_gpu(const std::vector<ColorCalibration>& data) {
	int n = data.size();
	if (n > MAX_COLORS) n = MAX_COLORS;

	std::vector<uchar>  h_ids(n);
	std::vector<float3> h_means(n);

	for(int i = 0; i < n; i++) {
		const auto& item = data[i];

		h_ids[i] = (uchar)item.id;

		h_means[i] = make_float3(
			(float)item.hsv_avg[0],
			(float)item.hsv_avg[1],
			(float)item.hsv_avg[2]
		);

	}

	gpuErrchk(cudaMemcpyToSymbol(c_ids, h_ids.data(), n * sizeof(uchar)));
	gpuErrchk(cudaMemcpyToSymbol(c_means, h_means.data(), n * sizeof(float3)));
	gpuErrchk(cudaMemcpyToSymbol(c_num_colors, &n, sizeof(int)));
}

cv::cuda::GpuMat launch_color_segmentation(const cv::cuda::GpuMat& hsv, const cv::cuda::GpuMat& mask) {

	static cv::cuda::GpuMat internal_buffer;
	static cv::cuda::GpuMat output_buffer;

	internal_buffer.create(hsv.size(), CV_8UC1);
	output_buffer.create(hsv.size(), CV_8UC1);

	dim3 block(32, 32);
	dim3 grid(
	   (hsv.cols + block.x - 1) / block.x,
	   (hsv.rows + block.y - 1) / block.y
	);

	float max_dist = 50.0f;
	constexpr int k = 5;

	fflush(stdout);
	knc_dynamic_k_kernel<<<grid, block>>>(hsv, mask, internal_buffer, max_dist*max_dist, k);
	label_moore_neighborhood<<<grid, block>>>(internal_buffer, output_buffer, 6);
	cudaError_t error = cudaDeviceSynchronize();

	return output_buffer;
}

