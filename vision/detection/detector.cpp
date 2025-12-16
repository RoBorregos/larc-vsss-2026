#include "detector.h"

Detector::Detector(GUI *gui, AppData *app_data): gui(gui), app_data(app_data) {

}

void Detector::find_objects() {
	cv::Mat white_mask;
	cv::inRange(hsv_mat, cv::Scalar(0, 0, 180), cv::Scalar(180, 60, 255), white_mask);
	cv::Mat kernel_white = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
	cv::dilate(white_mask, white_mask, kernel_white);
	hsv_mat.setTo(cv::Scalar(0, 0, 0), white_mask);

	cv::GaussianBlur(hsv_mat, hsv_mat, cv::Size(5, 5), 1.5);

	cv::Mat object_mask;
	cv::inRange(hsv_mat,
				cv::Scalar(0, 100, 50),
				cv::Scalar(180, 255, 255),
				object_mask);

	cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(7, 7));
	cv::morphologyEx(object_mask, object_mask, cv::MORPH_OPEN, kernel);
	cv::morphologyEx(object_mask, object_mask, cv::MORPH_CLOSE, kernel);

	cv::imshow("Mascara Limpia S+V", object_mask);

	std::vector<std::vector<cv::Point>> temp_objects;
	cv::findContours(object_mask, temp_objects, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

	std::vector<std::vector<cv::Point>> objects;
	for (const auto& cnt : temp_objects) {
		double area = cv::contourArea(cnt);
		if (area > 200 && area < 10000) {

			std::vector<cv::Point> approx;
			double epsilon = 0.03 * cv::arcLength(cnt, true);
			cv::approxPolyDP(cnt, approx, epsilon, true);

			objects.push_back(approx);
		}
	}

	this->found_objects = objects;
}

void Detector::categorize_objects() {
	cv::Mat full_image = gui->get_image(cv::COLOR_BGR2HSV);
	std::vector<std::vector<DetectedPatch>> detected_objects;

	for (unsigned long obj_i = 0; obj_i < found_objects.size(); obj_i++) {
		cv::Rect rect = cv::boundingRect(found_objects[obj_i]);

		rect = rect & cv::Rect(0, 0, full_image.cols, full_image.rows);

		if (rect.area() == 0) continue;

		cv::Mat robot_roi = full_image(rect);

		std::vector<DetectedPatch> patches;
		cv::GaussianBlur(robot_roi, robot_roi, cv::Size(3, 3), 0);
		cv::Mat global_mask = cv::Mat::zeros(robot_roi.rows + 2, robot_roi.cols + 2, CV_8UC1);
		cv::Scalar tolerance(7, 40, 40);


		for (int y = 0; y < robot_roi.rows; y++) {
			for (int x = 0; x < robot_roi.cols; x++) {
				if (global_mask.at<uchar>(y + 1, x + 1) != 0) continue;

				cv::Vec3b seed_pixel = robot_roi.at<cv::Vec3b>(y, x);
				if (seed_pixel[2] < 40) continue; // Ignore dark pixels (low V)

				cv::Mat patch_mask = cv::Mat::zeros(robot_roi.rows + 2, robot_roi.cols + 2, CV_8UC1);
				cv::Rect rect;

				int area = cv::floodFill(robot_roi, patch_mask, cv::Point(x, y), cv::Scalar(),
									 &rect, tolerance, tolerance,
									 cv::FLOODFILL_MASK_ONLY | cv::FLOODFILL_FIXED_RANGE | (255 << 8));

				if (area > 10 && area < 600) {
					DetectedPatch patch;
					patch.frame_id = 0; // todo: assign frame id using gui (missing implementation)
					patch.pixel_count = area;

					cv::Scalar sum(0, 0, 0);
					cv::Scalar sq_sum(0, 0, 0);

					// Initialization values
					patch.min_hsv = cv::Scalar(180, 255, 255);
					patch.max_hsv = cv::Scalar(0, 0, 0);

					for (int j = rect.y; j < rect.y + rect.height; j++) {
						for (int i = rect.x; i < rect.x + rect.width; i++) {
							if (patch_mask.at<uchar>(j + 1, i + 1) == 0) continue;

							cv::Vec3b pixel = robot_roi.at<cv::Vec3b>(j, i);

							if (pixel[0] < patch.min_hsv[0]) patch.min_hsv[0] = pixel[0];
							if (pixel[1] < patch.min_hsv[1]) patch.min_hsv[1] = pixel[1];
							if (pixel[2] < patch.min_hsv[2]) patch.min_hsv[2] = pixel[2];

							if (pixel[0] > patch.max_hsv[0]) patch.max_hsv[0] = pixel[0];
							if (pixel[1] > patch.max_hsv[1]) patch.max_hsv[1] = pixel[1];
							if (pixel[2] > patch.max_hsv[2]) patch.max_hsv[2] = pixel[2];

							for (int k = 0; k < 3; k++) {
								double val = pixel[k];
								sum[k] += val;
								sq_sum[k] += val * val;
							}
						}
					}

					for (int k = 0; k < 3; k++) {
						patch.avg_hsv[k] = sum[k] / area;
						double variance = (sq_sum[k] / area) - (patch.avg_hsv[k] * patch.avg_hsv[k]);
						patch.std_dev_hsv[k] = std::sqrt(std::max(0.0, variance));
					}

					cv::Mat roi_mask_crop = patch_mask(cv::Range(rect.y + 1, rect.y + rect.height + 1),
												   cv::Range(rect.x + 1, rect.x + rect.width + 1));
					std::vector<std::vector<cv::Point>> contours;
					cv::findContours(roi_mask_crop, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

					if (!contours.empty()) {
						for(auto& pt : contours[0]) {
							pt.x += rect.x;
							pt.y += rect.y;
						}
						patch.contour = contours[0];
					}

					patches.push_back(patch);

					cv::bitwise_or(global_mask, patch_mask, global_mask);
				}
			}
		}

		detected_objects.push_back(patches);
	}

	this->categorized_objects = detected_objects;
}

void Detector::update() {
	hsv_mat = gui->get_image(cv::COLOR_BGR2HSV);

	find_objects();
	for (const auto& contour : found_objects) {
		gui->closed_polyline(contour);
	}

	categorize_objects();
}

std::optional<State> Detector::ball() {
	return std::nullopt;
};

std::optional<State> Detector::robot(TeamColor team, const std::vector<PatchColor> &color_pattern) {
	return std::nullopt;
};

void Detector::display_debug_info() {
    if (found_objects.empty() || found_objects.size() != categorized_objects.size()) {
        return;
    }

    cv::Mat full_image = gui->get_image();

    std::cout << "\n--- DEBUG INFO (Frame Display) ---" << std::endl;

    for (size_t i = 0; i < found_objects.size(); ++i) {

        cv::Rect rect = cv::boundingRect(found_objects[i]);

        rect = rect & cv::Rect(0, 0, full_image.cols, full_image.rows);

        if (rect.area() == 0) continue;

        cv::Mat robot_roi = full_image(rect).clone();
        cv::Mat zoomed_view;
        double zoom = 5.0;

        cv::resize(robot_roi, zoomed_view, cv::Size(), zoom, zoom, cv::INTER_NEAREST);

        const std::vector<DetectedPatch>& patches = categorized_objects[i];

        std::cout << "ROBOT " << i << " (" << patches.size() << " patches detected):" << std::endl;

        for (size_t j = 0; j < patches.size(); ++j) {
            const DetectedPatch& p = patches[j];

            std::vector<cv::Point> scaled_contour = p.contour;
            for (auto& pt : scaled_contour) {
                pt.x = (int)(pt.x * zoom);
                pt.y = (int)(pt.y * zoom);
            }

        	if (!scaled_contour.empty()) {
                std::vector<std::vector<cv::Point>> to_draw = {scaled_contour};
                cv::drawContours(zoomed_view, to_draw, -1, cv::Scalar(0, 255, 0), 1);

                cv::putText(zoomed_view, std::to_string(j), scaled_contour[0],
                            cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);
            }

            std::cout << "  > Patch " << j
                      << " | Area: " << p.pixel_count
                      << " | Avg HSV: [" << (int)p.avg_hsv[0] << ", "
                      << (int)p.avg_hsv[1] << ", " << (int)p.avg_hsv[2] << "]"
                      << " | StdDev H: " << p.std_dev_hsv[0] << std::endl;
        }

        std::string win_name = "Debug Robot " + std::to_string(i);
        cv::imshow(win_name, zoomed_view);
    }
}