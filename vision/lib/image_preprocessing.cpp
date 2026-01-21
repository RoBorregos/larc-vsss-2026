
#include "image_preprocessing.h"

static void update_lut_if_needed(float gamma, float& last_gamma,
								 cv::cuda::GpuMat& gpu_lut,
								 cv::Ptr<cv::cuda::LookUpTable>& lut_alg) {
	if (std::abs(gamma - last_gamma) < 0.001f) return;

	cv::Mat cpu_lut(1, 256, CV_8U);
	uchar* p = cpu_lut.ptr();
	for (int i = 0; i < 256; ++i) {
		p[i] = cv::saturate_cast<uchar>(pow(i / 255.0, 1.0 / gamma) * 255.0);
	}

	gpu_lut.upload(cpu_lut);

	if (lut_alg.empty()) {
		lut_alg = cv::cuda::createLookUpTable(gpu_lut);
	} else {
		lut_alg = cv::cuda::createLookUpTable(gpu_lut);
	}

	last_gamma = gamma;
}

void preprocessing::filters(const cv::cuda::GpuMat& bgr_image_src,
                            cv::cuda::GpuMat& bgr_image_out,
                            cv::cuda::GpuMat& hsv_image_out,
                            const VisionParams &params) {

    if (params.blue_boost != 1.0f || params.green_boost != 1.0f || params.red_boost != 1.0f) {
       const cv::Scalar scales(params.blue_boost, params.green_boost, params.red_boost, 1.0);
       cv::cuda::multiply(bgr_image_src, scales, bgr_image_out);
    } else {
       bgr_image_src.copyTo(bgr_image_out);
    }


    cv::cuda::GpuMat d_lab;
    cv::cuda::cvtColor(bgr_image_out, d_lab, cv::COLOR_BGR2Lab);

    std::vector<cv::cuda::GpuMat> lab_channels;
    cv::cuda::split(d_lab, lab_channels);

	// With cuda I can do 2x2 tiles (free performance)
    static cv::Ptr<cv::cuda::CLAHE> clahe_alg = cv::cuda::createCLAHE(params.clahe_clip_limit, cv::Size(2, 2));
    clahe_alg->setClipLimit(params.clahe_clip_limit);
    clahe_alg->apply(lab_channels[0], lab_channels[0]);

    cv::cuda::merge(lab_channels, d_lab);

    cv::cuda::GpuMat d_lab_filtered;
    cv::cuda::bilateralFilter(d_lab, d_lab_filtered, -1, params.bilateral_sigma, 3);

    cv::cuda::cvtColor(d_lab_filtered, bgr_image_out, cv::COLOR_Lab2BGR);
    cv::cuda::cvtColor(bgr_image_out, hsv_image_out, cv::COLOR_BGR2HSV);

    static float last_gamma_v = -1.0f;
    static cv::cuda::GpuMat lut_v_data;
    static cv::Ptr<cv::cuda::LookUpTable> lut_v_alg;

    static float last_gamma_s = -1.0f;
    static cv::cuda::GpuMat lut_s_data;
    static cv::Ptr<cv::cuda::LookUpTable> lut_s_alg;

    bool do_gamma_v = (params.gamma_correction_v != 1.0f);
    bool do_gamma_s = (params.gamma_correction_s != 1.0f);
    bool do_saturation = (params.saturation != 1.0f);

    if (do_gamma_v) update_lut_if_needed(params.gamma_correction_v, last_gamma_v, lut_v_data, lut_v_alg);
    if (do_gamma_s) update_lut_if_needed(params.gamma_correction_s, last_gamma_s, lut_s_data, lut_s_alg);

    if (!do_gamma_v && !do_gamma_s && !do_saturation) return;

    std::vector<cv::cuda::GpuMat> hsv_channels;
    cv::cuda::split(hsv_image_out, hsv_channels);

    if (do_saturation) {
       cv::cuda::multiply(hsv_channels[1], params.saturation, hsv_channels[1]);
    }

    if (do_gamma_s) {
       lut_s_alg->transform(hsv_channels[1], hsv_channels[1]);
    }

    if (do_gamma_v) {
       lut_v_alg->transform(hsv_channels[2], hsv_channels[2]);
    }

    cv::cuda::merge(hsv_channels, hsv_image_out);
    cv::cuda::cvtColor(hsv_image_out, bgr_image_out, cv::COLOR_HSV2BGR);
}
void preprocessing::resize(cv::cuda::GpuMat& input_image, const int target_width, const int target_height) {
	if (input_image.cols == target_width && input_image.rows == target_height) {
		return;
	}

	cv::cuda::GpuMat temp;
	cv::cuda::resize(input_image, temp, cv::Size(target_width, target_height), 0, 0, cv::INTER_LINEAR);
	input_image = temp;
}