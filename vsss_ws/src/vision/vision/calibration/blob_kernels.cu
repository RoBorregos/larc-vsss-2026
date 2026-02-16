#include <cuda_runtime.h>
#include <device_launch_parameters.h>

#include <opencv2/core/cuda.hpp>
#include <opencv2/core/cuda/common.hpp>

#include <iostream>

struct BlobStats {
	unsigned int count;
	unsigned int count_low_red;  // < 30
	unsigned int count_high_red; // > 150

	unsigned long long sum_s, sq_sum_s;
	unsigned int min_s, max_s;

	unsigned long long sum_v, sq_sum_v;
	unsigned int min_v, max_v;

	unsigned long long sum_h, sq_sum_h;
	unsigned int min_h, max_h;

	unsigned long long sum_h_wrap, sq_sum_h_wrap;
	unsigned int min_h_wrap, max_h_wrap;
};

__global__ void blobStatisticsKernel(
	cv::cuda::PtrStepSz<uchar3> hsv,
	cv::cuda::PtrStepSz<uchar> mask,
	BlobStats* stats) {
	int x = blockIdx.x * blockDim.x + threadIdx.x;
	int y = blockIdx.y * blockDim.y + threadIdx.y;

	if (x >= hsv.cols || y >= hsv.rows) return;
	if (mask(y, x) == 0) return;

	uchar3 pixel = hsv(y, x);
	int h = pixel.x;
	int s = pixel.y;
	int v = pixel.z;

	atomicAdd(&stats->count, 1);

	atomicAdd(&stats->sum_s, (unsigned long long)s);
	atomicAdd(&stats->sq_sum_s, (unsigned long long)(s * s));
	atomicMin(&stats->min_s, s);
	atomicMax(&stats->max_s, s);

	atomicAdd(&stats->sum_v, (unsigned long long)v);
	atomicAdd(&stats->sq_sum_v, (unsigned long long)(v * v));
	atomicMin(&stats->min_v, v);
	atomicMax(&stats->max_v, v);

	atomicAdd(&stats->sum_h, (unsigned long long)h);
	atomicAdd(&stats->sq_sum_h, (unsigned long long)(h * h));
	atomicMin(&stats->min_h, h);
	atomicMax(&stats->max_h, h);

	if (h < 30) atomicAdd(&stats->count_low_red, 1);
	else if (h > 150) atomicAdd(&stats->count_high_red, 1);

	int h_w = (h < 90) ? h + 180 : h;

	atomicAdd(&stats->sum_h_wrap, (unsigned long long)h_w);
	atomicAdd(&stats->sq_sum_h_wrap, (unsigned long long)(h_w * h_w));
	atomicMin(&stats->min_h_wrap, h_w);
	atomicMax(&stats->max_h_wrap, h_w);
}

__global__ void initStats(BlobStats* s) {
	if (threadIdx.x == 0 && blockIdx.x == 0) {
		s->count = 0; s->count_low_red = 0; s->count_high_red = 0;
		s->sum_s = 0; s->sq_sum_s = 0; s->min_s = 255; s->max_s = 0;
		s->sum_v = 0; s->sq_sum_v = 0; s->min_v = 255; s->max_v = 0;
		s->sum_h = 0; s->sq_sum_h = 0; s->min_h = 255; s->max_h = 0;
		s->sum_h_wrap = 0; s->sq_sum_h_wrap = 0; s->min_h_wrap = 400; s->max_h_wrap = 0;
	}
}

void launchBlobKernel(cv::cuda::GpuMat& hsv, cv::cuda::GpuMat& mask, BlobStats* d_stats) {

	dim3 block(32, 32);
	dim3 grid((hsv.cols + block.x - 1) / block.x,
			  (hsv.rows + block.y - 1) / block.y);

	initStats<<<1, 1>>>(d_stats);
	cudaDeviceSynchronize();

	blobStatisticsKernel<<<grid, block>>>(hsv, mask, d_stats);

	cudaError_t error = cudaGetLastError();
	if (error != cudaSuccess) {
		printf("CUDA error: %s\n", cudaGetErrorString(error));
	}
}