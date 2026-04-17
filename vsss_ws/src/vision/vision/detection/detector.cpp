#include "detector.h"

namespace {
	double distance(const cv::Point2d& point_1, const cv::Point2d& point_2) {
		return static_cast<double>(cv::norm(point_1 - point_2));
	}

	std::string color_to_string(int color) {
		switch (color) {
		case Color_ID::BLUE:    return "BLUE";
		case Color_ID::YELLOW: return "YELLOW";
		case Color_ID::CYAN:   return "CYAN";
		case Color_ID::GREEN:  return "GREEN";
		case Color_ID::MAGENTA: return "MAGENTA";
		case Color_ID::RED:    return "RED";
		case Color_ID::ORANGE: return "ORANGE";
		default: return "NONE";
		}
	};

	cv::Scalar get_debug_color(int color) {
		switch (color) {
		case Color_ID::YELLOW:  return {0, 255, 255};
		case Color_ID::BLUE:    return {255, 0, 0};
		case Color_ID::ORANGE:  return {0, 165, 255};
		case Color_ID::CYAN:    return {255, 255, 0};
		case Color_ID::MAGENTA: return {255, 0, 255};
		case Color_ID::GREEN:   return {0, 255, 0};
		case Color_ID::RED:     return {0, 0, 255};
		default:                return {128, 128, 128};
		}
	}

	void draw_rotated_rect(cv::Mat& img, cv::Point2f center, cv::Size2f size, double angle_rad, cv::Scalar color) {
		float angle_deg = angle_rad * 180.0f / CV_PI;
		cv::RotatedRect rRect(center, size, angle_deg);
		cv::Point2f vertices[4];
		rRect.points(vertices);
		std::vector<cv::Point> box_pts;
		for (int i = 0; i < 4; i++) {
			box_pts.push_back(vertices[i]);
		}

		cv::fillConvexPoly(img, box_pts, color);
		cv::polylines(img, box_pts, true, cv::Scalar(255, 255, 255), 1, cv::LINE_AA);
	}

	cv::Point2d find_geometric_median(const RobotPatch& robot_patch, int iterations = 5) {
		cv::Point2d median(0, 0);

		std::vector<cv::Point2d> points;
		for (const auto& p: robot_patch.children) points.push_back(p->centroid);
		points.push_back(robot_patch.parent->centroid);

		for (const auto& p : points) median += p;
		median.x /= static_cast<float>(points.size());
		median.y /= static_cast<float>(points.size());

		for (int i = 0; i < iterations; ++i) {
			double numX = 0, numY = 0, den = 0;

			for (const auto& p : points) {
				const double dist = static_cast<double>(cv::norm(p - median));
				if (dist < 0.1f) continue;

				const double weight = 1.0f / dist;
				numX += p.x * weight;
				numY += p.y * weight;
				den += weight;
			}
			if (den != 0) {
				median.x = numX / den;
				median.y = numY / den;
			}
		}
		return median;
	};

	std::vector<std::shared_ptr<Patch>> find_all_candidates(
		const std::shared_ptr<Patch>& center,
		const std::vector<std::shared_ptr<Patch>>& patches,
		int MINIMUM_DISTANCE
	) {
		std::vector<std::shared_ptr<Patch>> candidates;
		for (const auto& patch : patches) {
			if (patch->id == center->id) continue;
			if (patch->parent) continue;
			if (distance(center->centroid, patch->centroid) <= MINIMUM_DISTANCE) candidates.push_back(patch);
		}

		return candidates;
	}

	std::vector<std::shared_ptr<Patch>> find_child_1_candidates(
		const std::shared_ptr<Patch>& parent,
		const std::vector<std::shared_ptr<Patch>>& patches,
		const std::vector<RobotRelationship>& relationship,
		const int MINIMUM_DISTANCE
	) {
		const std::vector<std::shared_ptr<Patch>> candidates = find_all_candidates(parent, patches, MINIMUM_DISTANCE);
		std::vector<std::shared_ptr<Patch>> child_1_candidates;

		for (const auto& candidate : candidates) {
			const bool linked = std::any_of(relationship.begin(), relationship.end(), [&](const RobotRelationship& r) {
				return r.parent_id == parent->id && r.child_1_id == candidate->id;
			});

			if (!linked) child_1_candidates.push_back(candidate);
		}

		return child_1_candidates;
	}

	std::vector<std::shared_ptr<Patch>> find_child_2_candidates(
		const std::shared_ptr<Patch>& parent,
		const std::shared_ptr<Patch>& child_1,
		const std::vector<std::shared_ptr<Patch>>& patches,
		const std::vector<RobotRelationship>& relationship,
		const int MINIMUM_DISTANCE
	) {
		const std::vector<std::shared_ptr<Patch>> candidates = find_all_candidates(parent, patches, MINIMUM_DISTANCE);
		std::vector<std::shared_ptr<Patch>> child_2_candidates;

		for (const auto& candidate : candidates) {
			if (candidate->id == child_1->id) {
				continue;
			}

			bool already_tried = std::any_of(relationship.begin(), relationship.end(), [&](const RobotRelationship& r) {
				return r.parent_id == parent->id &&
					   r.child_1_id == child_1->id &&
					   (r.child_2_id.has_value() && r.child_2_id.value() == candidate->id);
			});

			if (!already_tried) child_2_candidates.push_back(candidate);
		}

		return child_2_candidates;
	}

	void remove_patch_by_id(std::vector<std::shared_ptr<Patch>>& vec, int id) {
		vec.erase(std::remove_if(vec.begin(), vec.end(),
			[id](const auto& p) { return p->id == id; }), vec.end());
	}
}

Detector::Detector(GUI *gui, AppData *app_data, BlobCalibrator* blob_calibrator, std::shared_ptr<RosHandler> ros_handler):
	gui(gui), app_data(app_data), blob_calibrator(blob_calibrator), ros_handler(std::move(ros_handler)) {

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

cv::cuda::GpuMat Detector::get_blob_mask(const cv::cuda::GpuMat &hsv_image, bool debug_mode) {
	const auto& p = app_data->mask_params;

	const cv::Scalar lower(p.h_min, p.s_min, p.v_min);
	const cv::Scalar upper(p.h_max, p.s_max, p.v_max);

	cv::cuda::GpuMat mask;
	cv::cuda::inRange(hsv_image, lower, upper, mask);

	if (debug_mode) {
		cv::Mat mask_host;
		cudaDeviceSynchronize();
		mask.download(mask_host);
		cv::imshow("Mask S+V: ", mask_host);
	}

	return mask;
}

cv::Mat Detector::get_label_map() {
	bool debug_mode = ros_handler->get_parameter("debug_mode").as_bool();

	const cv::cuda::GpuMat hsv_image = gui->get_image(cv::COLOR_BGR2HSV);

	const cv::cuda::GpuMat blob_mask = get_blob_mask(hsv_image, debug_mode);
	const cv::cuda::GpuMat label_map = launch_color_segmentation(hsv_image, blob_mask);

	cv::Mat label_map_host;
	cudaDeviceSynchronize();
	label_map.download(label_map_host);

	cv::imshow("Label map", visualize_labels(label_map_host));
	return label_map_host;
}

std::pair<std::vector<RobotPatch>, std::optional<BallPatch>> Detector::update() {
	robot_patches.clear();
	patches.clear();
	ball_patch = std::nullopt;

	bool debug_mode = ros_handler->get_parameter("debug_mode").as_bool();

	cv::Mat label_map = get_label_map();
	get_patches(label_map);
	get_ball_patch();

	cv::cuda::GpuMat frame_gpu = gui->get_image();
	cv::Mat frame_cpu;

	frame_gpu.download(frame_cpu);

	detect_aruco_robots(frame_cpu);
	if (debug_mode) {
		display_aruco_debug(frame_cpu);
	}

	return { robot_patches, ball_patch };
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
			case Color_ID::BLUE:    out_ptr[x] = cv::Vec3b(255, 0, 0);   break;
			case Color_ID::YELLOW:  out_ptr[x] = cv::Vec3b(0, 255, 255); break;
			case Color_ID::CYAN:    out_ptr[x] = cv::Vec3b(255, 255, 0); break;
			case Color_ID::GREEN:   out_ptr[x] = cv::Vec3b(0, 255, 0);   break;
			case Color_ID::MAGENTA: out_ptr[x] = cv::Vec3b(255, 0, 255); break;
			case Color_ID::RED:     out_ptr[x] = cv::Vec3b(0, 0, 255);   break;
			case Color_ID::ORANGE:  out_ptr[x] = cv::Vec3b(0, 165, 255); break;
			default:
				out_ptr[x] = cv::Vec3b(255, 255, 255);
				break;
			}
		}
	}

	return colored_img;
}

void Detector::detect_aruco_robots(const cv::Mat& frame) {
	std::vector<int> markerIds;
	std::vector<std::vector<cv::Point2f>> markerCorners;

	cv::aruco::Dictionary dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_ARUCO_ORIGINAL);
	cv::aruco::DetectorParameters detectorParams = cv::aruco::DetectorParameters();

	detectorParams.adaptiveThreshWinSizeMin = 3;
	detectorParams.adaptiveThreshWinSizeMax = 23;
	detectorParams.adaptiveThreshWinSizeStep = 5;

	detectorParams.minMarkerPerimeterRate = 0.015;

	detectorParams.errorCorrectionRate = 0.6;
	detectorParams.polygonalApproxAccuracyRate = 0.05;

	detectorParams.perspectiveRemoveIgnoredMarginPerCell = 0.25;
	detectorParams.perspectiveRemovePixelPerCell = 10;

	detectorParams.cornerRefinementMethod = cv::aruco::CORNER_REFINE_APRILTAG;
	detectorParams.cornerRefinementWinSize = 3;
	detectorParams.cornerRefinementMaxIterations = 30;
	detectorParams.cornerRefinementMinAccuracy = 0.01;

	detectorParams.maxErroneousBitsInBorderRate = 0.5;
	detectorParams.minOtsuStdDev = 3.0;

	constexpr float resize_factor = 1.5;

	cv::Mat enlarged;
	cv::resize(frame, enlarged, cv::Size(), resize_factor, resize_factor, cv::INTER_CUBIC);

	cv::Mat gray;
	cv::cvtColor(enlarged, gray, cv::COLOR_BGR2GRAY);

	cv::Mat sharp;
	cv::GaussianBlur(gray, sharp, cv::Size(0, 0), 3);
	cv::addWeighted(gray, 3.5, sharp, -1.5, 0, gray);

	cv::imshow("grey+enlarged+sharp", gray);

	cv::aruco::ArucoDetector detector(dictionary, detectorParams);

	detector.detectMarkers(gray, markerCorners, markerIds);

	for (size_t i = 0; i < markerIds.size(); i++) {
		RobotPatch robot;
		robot.id = RobotIdentities::get_robot_id_by_aruco(markerIds[i]);

		cv::Point2f center(0, 0);
		for (const auto& corner : markerCorners[i]) center += (corner / resize_factor);
		robot.center = center * 0.25f;

		cv::Point2f front_vec = markerCorners[i][1] - markerCorners[i][0];
		robot.facing = std::atan2(front_vec.y, front_vec.x) - (M_PI / 2);

		robot_patches.push_back(robot);
	}
}

void Detector::display_aruco_debug(const cv::Mat& original_frame) {
    cv::Mat debug_frame = original_frame.clone();

    for (const auto& robot : robot_patches) {
        cv::circle(debug_frame, robot.center, 4, cv::Scalar(255, 0, 255), cv::FILLED);
        cv::circle(debug_frame, robot.center, 12, cv::Scalar(0, 255, 0), 2);

        int arrow_length = 25;
        cv::Point2f direction_point(
            robot.center.x + arrow_length * std::cos(robot.facing),
            robot.center.y + arrow_length * std::sin(robot.facing)
        );

        cv::arrowedLine(debug_frame, robot.center, direction_point, cv::Scalar(0, 0, 255), 2, cv::LINE_AA, 0, 0.3);

        std::string label = "ID: " + std::to_string(robot.id);
        int baseline = 0;
        cv::Size textSize = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseline);
        cv::Point textPos(robot.center.x - textSize.width / 2, robot.center.y - 20);

        cv::rectangle(debug_frame, textPos + cv::Point(0, baseline), textPos + cv::Point(textSize.width, -textSize.height),
                      cv::Scalar(0, 0, 0), cv::FILLED);
        cv::putText(debug_frame, label, textPos, cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1, cv::LINE_AA);
    }

    std::string count_text = "Robots detectados: " + std::to_string(robot_patches.size());
    cv::putText(debug_frame, count_text, cv::Point(20, 30), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);

    cv::imshow("ArUco Detector Debug", debug_frame);
}

void Detector::get_patches(const cv::Mat& label_map) {
	int id = 0;

	for (int color = 1; color <= 7; color++) {
		cv::Mat mask;
		cv::compare(label_map, color, mask, cv::CMP_EQ);

		std::vector<std::vector<cv::Point>> contours;
		cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

		for (const auto& cnt : contours) {
			if (cv::contourArea(cnt) > PATCH_MIN_AREA) {
				const cv::Moments m = cv::moments(cnt);
				if (m.m00 == 0) continue;
				const cv::Rect bbox = cv::boundingRect(cnt);
				const cv::Point2d center(m.m10 / m.m00, m.m01 / m.m00);
				auto p = std::make_shared<Patch>();

				p->id = id++;
				p->color = color;
				p->centroid = center;
				p->parent = (color == 1 || color == 2);
				p->bounding_box = bbox;
				p->valid = true;

				patches.emplace_back(p);
			}
		}
	}

	std::vector<std::shared_ptr<Patch>> blue_patches;
	std::vector<std::shared_ptr<Patch>> cyan_patches;

	for (auto& p : patches) {
		if (p->color == Color_ID::BLUE) blue_patches.push_back(p);
		else if (p->color == Color_ID::CYAN) cyan_patches.push_back(p);
	}

	std::vector<int> patches_to_remove;

	for (auto& blue : blue_patches) {
		for (auto& cyan : cyan_patches) {
			cv::Rect intersection = blue->bounding_box & cyan->bounding_box;

			if (intersection.area() > 0) {
				float coverage = (float)intersection.area() / (float)cyan->bounding_box.area();
				if (coverage > 0.7f) {
					cyan->valid = false;

					blue->centroid = (blue->centroid + cyan->centroid) * 0.5;
				}
			}
		}
	}

	patches.erase(
		std::remove_if(patches.begin(), patches.end(),
			[](const std::shared_ptr<Patch>& p) { return !p->valid; }),
		patches.end()
	);
}

void Detector::get_ball_patch() {
	if (patches.empty()) return;
	BallPatch ball;

	int orange_occurrences = 0;
	std::shared_ptr<Patch> last_orange = nullptr;

	for (const auto& p : patches) {
		if (p->color == Color_ID::ORANGE) {
			last_orange = p;
			orange_occurrences++;
		}
	}

	if (orange_occurrences == 1 && last_orange) {
		ball.patch = last_orange;
		ball.center = last_orange->centroid;
		ball_patch = ball;
		return;
	}

	// Program Panic!!!!!!!!!!!
	// This is an attempt that directly modifies the color of the patches (see documentation)
	// If no orange is found (means that is an edge case where orange and red are indistinguishable...)
	for (const auto& p : patches) {
		if (p->color == Color_ID::ORANGE) p->color = Color_ID::RED;
	}

	for (const auto& p1 : patches) {
		if (p1->color != Color_ID::RED) continue;

		bool is_isolated = true;

		for (const auto& p2 : patches) {
			if (p1->id == p2->id) continue;

			if (distance(p1->centroid, p2->centroid) <= MINIMUM_DISTANCE) {
				is_isolated = false;
				break;
			}
		}

		if (is_isolated) {
			ball.patch = p1;
			ball.center = p1->centroid;
			ball_patch = ball;
			return;
		}
	}
}