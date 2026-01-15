#include "detector.h"


Detector::Detector(GUI *gui, AppData *app_data, BlobCalibrator* blob_calibrator): gui(gui), app_data(app_data), blob_calibrator(blob_calibrator) {

}

void Detector::on_debug_mouse(int event, int x, int y, int flags, void* userdata) {
	if (event != cv::EVENT_LBUTTONDOWN) return;

	auto* data = static_cast<DebugWindowData*>(userdata);

	int local_x = static_cast<int>(x / data->zoom_factor);
	int local_y = static_cast<int>(y / data->zoom_factor);

	int global_x = local_x + data->roi_offset.x;
	int global_y = local_y + data->roi_offset.y;

	std::cout << "[Debug Click] Local: " << x << "," << y
			  << " -> Global: " << global_x << "," << global_y << std::endl;

	data->calibrator->handle_click(global_x, global_y);
}

void Detector::find_objects() {
	cv::Mat white_mask;
    cv::inRange(hsv_mat,
                cv::Scalar(0, 0, 200),      // Blancos brillantes
                cv::Scalar(180, 50, 255),
                white_mask);

	static const cv::Mat kernel_noise = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
	cv::dilate(white_mask, white_mask, kernel_noise);

	cv::Mat object_mask;
	cv::inRange(hsv_mat,
				cv::Scalar(app_data->mask_params.h_min, app_data->mask_params.s_min, app_data->mask_params.v_min),
				cv::Scalar(app_data->mask_params.h_max, app_data->mask_params.s_max, app_data->mask_params.v_max),
				object_mask);

	cv::bitwise_and(object_mask, ~white_mask, object_mask);

    static const cv::Mat kernel_clean = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));

    cv::morphologyEx(object_mask, object_mask, cv::MORPH_OPEN, kernel_clean);
    cv::morphologyEx(object_mask, object_mask, cv::MORPH_CLOSE, kernel_clean);

    cv::imshow("Mascara Final", object_mask);
    std::vector<std::vector<cv::Point>> temp_contours;
    cv::findContours(object_mask, temp_contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    std::vector<std::vector<cv::Point>> final_objects;
    final_objects.reserve(temp_contours.size());

    for (const auto& cnt : temp_contours) {
        double area = cv::contourArea(cnt);

        if (area > 200 && area < 10000) {

            cv::Rect box = cv::boundingRect(cnt);
            double aspect_ratio = (double)box.width / box.height;

            if (aspect_ratio > 0.5 && aspect_ratio < 2.0) {
                std::vector<cv::Point> approx;
                double epsilon = 0.03 * cv::arcLength(cnt, true);
                cv::approxPolyDP(cnt, approx, epsilon, true);

                final_objects.push_back(std::move(approx));
            }
        }
    }

    this->found_objects = std::move(final_objects);
}

void Detector::categorize_objects() {
    cv::Mat full_image = gui->get_image(cv::COLOR_BGR2HSV);
    std::vector<std::vector<DetectedPatch>> detected_objects;

    for (unsigned long obj_i = 0; obj_i < found_objects.size(); obj_i++) {
       cv::Rect rect = cv::boundingRect(found_objects[obj_i]);
       rect = rect & cv::Rect(0, 0, full_image.cols, full_image.rows);

       if (rect.area() == 0) continue;

       cv::Mat robot_roi = full_image(rect);

       cv::GaussianBlur(robot_roi, robot_roi, cv::Size(3, 3), 0);

       cv::Mat global_mask = cv::Mat::zeros(robot_roi.rows + 2, robot_roi.cols + 2, CV_8UC1);

       cv::Scalar tolerance(10, 40, 40);
       std::vector<DetectedPatch> patches;

       for (int y = 0; y < robot_roi.rows; y++) {
          for (int x = 0; x < robot_roi.cols; x++) {

             if (global_mask.at<uchar>(y + 1, x + 1) != 0) continue;

             cv::Vec3b seed_pixel = robot_roi.at<cv::Vec3b>(y, x);
             if (seed_pixel[2] < 40) continue;

             cv::Rect bounding_box;

             int area = cv::floodFill(robot_roi, global_mask, cv::Point(x, y), cv::Scalar(),
                             &bounding_box, tolerance, tolerance,
                             cv::FLOODFILL_MASK_ONLY | cv::FLOODFILL_FIXED_RANGE | (255 << 8));

             if (area > 10 && area < 600) {
                DetectedPatch patch;
                patch.pixel_count = area;

                cv::Scalar sum(0, 0, 0);
                cv::Scalar sq_sum(0, 0, 0);

                patch.min_hsv = cv::Scalar(180, 255, 255);
                patch.max_hsv = cv::Scalar(0, 0, 0);

                for (int j = bounding_box.y; j < bounding_box.y + bounding_box.height; j++) {
                   for (int i = bounding_box.x; i < bounding_box.x + bounding_box.width; i++) {
                   	if (global_mask.at<uchar>(j + 1, i + 1) != 255) continue;

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

                cv::Mat current_patch_mask = (global_mask(cv::Rect(bounding_box.x + 1, bounding_box.y + 1, bounding_box.width, bounding_box.height)) == 255);

                std::vector<std::vector<cv::Point>> contours;
                cv::findContours(current_patch_mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

                if (!contours.empty()) {
                   for(auto& pt : contours[0]) {
                      pt.x += bounding_box.x;
                      pt.y += bounding_box.y;
                   }
                   patch.contour = contours[0];
                }

                patches.push_back(patch);
             }

             cv::Mat mask_roi = global_mask(cv::Rect(bounding_box.x + 1, bounding_box.y + 1, bounding_box.width, bounding_box.height));
             mask_roi.setTo(1, mask_roi == 255);
          }
       }

       detected_objects.push_back(patches);
    }
    this->categorized_objects = detected_objects;
}

double Detector::angle_cosine(const cv::Point pt1, const cv::Point pt2, const cv::Point pt0) {
	const double dx1 = pt1.x - pt0.x;
	const double dy1 = pt1.y - pt0.y;
	const double dx2 = pt2.x - pt0.x;
	const double dy2 = pt2.y - pt0.y;

	const double dot_product = dx1 * dx2 + dy1 * dy2;
	const double magnitude1 = std::sqrt(dx1 * dx1 + dy1 * dy1);
	const double magnitude2 = std::sqrt(dx2 * dx2 + dy2 * dy2);

	if (magnitude1 == 0 || magnitude2 == 0) return 1.0;

	return dot_product / (magnitude1 * magnitude2);
}

bool Detector::valid_square(const std::vector<cv::Point>& contour, std::vector<cv::Point>& dst) {
	double perimeter = cv::arcLength(contour, true);
	std::vector<cv::Point> approx;

	cv::approxPolyDP(contour, approx, 0.06 * perimeter, true);

	if (approx.size() == 4 && cv::isContourConvex(approx)) {
		double max_cosine = 0;

		for (int i = 0; i < 4; i++) {
			cv::Point pt0 = approx[i];
			cv::Point pt1 = approx[(i + 1) % 4];
			cv::Point pt2 = approx[(i + 2) % 4];

			cv::Point p_prev = approx[(i + 3) % 4];
			cv::Point p_current = approx[i];
			cv::Point p_next = approx[(i + 1) % 4];

			double consine = std::abs(angle_cosine(p_next, p_prev, p_current));
			max_cosine = std::max(max_cosine, consine);
		}

		if (max_cosine < 0.4) {
			dst = approx;
			return true;
		}
	}

	return false;
}

void Detector::filter_patches() {
	for (const auto& object : categorized_objects) {
		square_patches.emplace_back();

		for (const auto& patch : object) {

			if (cv::contourArea(patch.contour) < 20) continue;

			std::vector<cv::Point> approx;

			if (valid_square(patch.contour, approx)) {
				DetectedPatch square_patch = patch;
				square_patch.contour = approx;

				square_patches.back().push_back(square_patch);
			}
		}
	}
}

void Detector::update() {
	hsv_mat = gui->get_image(cv::COLOR_BGR2HSV);
	preprocessing::apply_preprogrammed_filters(hsv_mat, app_data->color_params);

	found_objects.clear();
	categorized_objects.clear();
	square_patches.clear();


	find_objects();
	for (const auto& contour : found_objects) {
		gui->closed_polyline(contour);
	}

	// show_debug_rois(hsv_mat, found_objects);
}

std::optional<State> Detector::ball() {
	return std::nullopt;
};

std::optional<State> Detector::robot(TeamColor team, const std::vector<PatchColor> &color_pattern) {
	return std::nullopt;
};

void Detector::destroy_debug_windows() {
	for (const auto& name : window_names) {
		cv::destroyWindow(name);
	}
	window_names.clear();
}


void Detector::display_debug_info() {
    if (found_objects.empty() || found_objects.size() != categorized_objects.size()) {
    	destroy_debug_windows();
        return;
    }

    cv::Mat full_image = gui->get_image();
	std::set<std::string> current_window_names;

	debug_window_contexts.clear();
	debug_window_contexts.reserve(found_objects.size());

	std::vector<cv::Point> global_points = blob_calibrator->get_points();

    for (size_t i = 0; i < found_objects.size(); ++i) {
        cv::Rect rect = cv::boundingRect(found_objects[i]);
        rect = rect & cv::Rect(0, 0, full_image.cols, full_image.rows);

        if (rect.area() == 0) continue;

        cv::Mat robot_roi = full_image(rect).clone();
        cv::Mat zoomed_view;
        double zoom = 5.0;

        cv::resize(robot_roi, zoomed_view, cv::Size(), zoom, zoom, cv::INTER_NEAREST);

    	if (!global_points.empty()) {
    		std::vector<cv::Point> local_points;

    		for (const auto& gp : global_points) {
    			int lx = static_cast<int>((gp.x - rect.x) * zoom);
    			int ly = static_cast<int>((gp.y - rect.y) * zoom);
    			local_points.emplace_back(lx, ly);
    		}

    		if (local_points.size() > 1) {
    			bool is_closed = (local_points.size() >= 4);
    			std::vector<std::vector<cv::Point>> contours_to_draw = { local_points };

    			cv::polylines(zoomed_view, contours_to_draw, is_closed, cv::Scalar(255, 255, 255), 2);
    		}

    		for (const auto& lp : local_points) {
    			cv::circle(zoomed_view, lp, 3, cv::Scalar(0, 0, 255), -1); // Punto rojo
    		}
    	}

        std::string win_name = "Debug Robot " + std::to_string(i);
    	current_window_names.insert(win_name);

    	cv::cvtColor(zoomed_view, zoomed_view, cv::COLOR_BGR2HSV);
    	preprocessing::apply_preprogrammed_filters(zoomed_view, app_data->color_params);
    	cv::cvtColor(zoomed_view, zoomed_view, cv::COLOR_HSV2BGR);
        // cv::imshow(win_name, zoomed_view);

    	DebugWindowData context;
    	context.calibrator = this->blob_calibrator;
    	context.roi_offset = rect;
    	context.zoom_factor = zoom;
    	debug_window_contexts.push_back(context);
    	// cv::setMouseCallback(win_name, Detector::on_debug_mouse, &debug_window_contexts.back());
    }

	auto it = window_names.begin();
	while (it != window_names.end()) {
		if (current_window_names.find(*it) == current_window_names.end()) {
			cv::destroyWindow(*it);
			it = window_names.erase(it);
		} else {
			++it;
		}
	}

	window_names.insert(current_window_names.begin(), current_window_names.end());
}

void Detector::show_debug_rois(const cv::Mat& src_img, const std::vector<std::vector<cv::Point>>& contours) {
	for (size_t i = 0; i < contours.size(); ++i) {
		cv::Rect rect = cv::boundingRect(contours[i]);
		rect = rect & cv::Rect(0, 0, src_img.cols, src_img.rows);

		if (rect.area() > 0) {
			cv::Mat roi = src_img(rect);
			cv::Mat display_roi;

			cv::cvtColor(roi, display_roi, cv::COLOR_HSV2BGR);

			std::string win_name = "Debug_Robot_" + std::to_string(i);
			cv::imshow(win_name, display_roi);
		}
	}

	cv::waitKey(1);
}