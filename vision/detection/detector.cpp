#include "detector.h"

Detector::Detector(GUI *gui, AppData *app_data, BlobCalibrator* blob_calibrator): gui(gui), app_data(app_data), blob_calibrator(blob_calibrator) {

}

void Detector::on_debug_mouse(const int event, const int x, const int y, int flags, void* userdata) {
	if (event != cv::EVENT_LBUTTONDOWN) return;

	const auto* data = static_cast<DebugWindowData*>(userdata);

	const int local_x = static_cast<int>(x / data->zoom_factor);
	const int local_y = static_cast<int>(y / data->zoom_factor);

	const int global_x = local_x + data->roi_offset.x;
	const int global_y = local_y + data->roi_offset.y;

	std::cout << "[Debug Click] Local: " << x << "," << y
			  << " -> Global: " << global_x << "," << global_y << std::endl;

	data->calibrator->handle_click(global_x, global_y);
}

cv::cuda::GpuMat Detector::get_blob_mask(const cv::cuda::GpuMat &hsv_image) {
	const auto& p = app_data->mask_params;

	const cv::Scalar lower(p.h_min, p.s_min, p.v_min);
	const cv::Scalar upper(p.h_max, p.s_max, p.v_max);

	cv::cuda::GpuMat mask;
	cv::cuda::inRange(hsv_image, lower, upper, mask);

	#ifdef DEBUG_MODE
		cv::Mat mask_host;
		mask.download(mask_host);
		cv::imshow("Mask S+V: ", mask_host);
	#endif
	return mask;
}

void Detector::update() {
	const cv::cuda::GpuMat hsv_image = gui->get_image(cv::COLOR_BGR2HSV);

	const cv::cuda::GpuMat blob_mask = get_blob_mask(hsv_image);
	cv::cuda::GpuMat label_map = launch_color_segmentation(hsv_image, blob_mask);

	#ifdef DEBUG_MODE
		cv::Mat mask_host;
		label_map.download(mask_host);

		cv::Mat colored_mask = visualize_labels(mask_host);
		cv::imshow("Label Map: ", colored_mask);
	#endif
}

void Detector::upload_calibrations(const std::vector<ColorCalibration>& color_calibrations) {
	upload_calibrations_to_gpu(color_calibrations);
}

cv::Mat Detector::visualize_labels(const cv::Mat& label_map) {
	cv::Mat colored_img = cv::Mat::zeros(label_map.size(), CV_8UC3);

	for (int y = 0; y < label_map.rows; ++y) {
		const uchar* row_ptr = label_map.ptr<uchar>(y);
		cv::Vec3b* out_ptr = colored_img.ptr<cv::Vec3b>(y);

		for (int x = 0; x < label_map.cols; ++x) {
			int id = row_ptr[x];
			if (id == 0) continue;

			switch (id) {
			case Color_ID::BLUE:    out_ptr[x] = cv::Vec3b(255, 0, 0);   break; // Azul
			case Color_ID::YELLOW:  out_ptr[x] = cv::Vec3b(0, 255, 255); break; // Amarillo
			case Color_ID::CYAN:    out_ptr[x] = cv::Vec3b(255, 255, 0); break; // Cyan
			case Color_ID::GREEN:   out_ptr[x] = cv::Vec3b(0, 255, 0);   break; // Verde
			case Color_ID::MAGENTA: out_ptr[x] = cv::Vec3b(255, 0, 255); break; // Magenta
			case Color_ID::RED:     out_ptr[x] = cv::Vec3b(0, 0, 255);   break; // Rojo
			case Color_ID::ORANGE:  out_ptr[x] = cv::Vec3b(0, 165, 255); break; // Naranja
			default:
				out_ptr[x] = cv::Vec3b(255, 255, 255);
				break;
			}
		}
	}

	return colored_img;
}