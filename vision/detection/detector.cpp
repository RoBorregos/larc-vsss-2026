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

cv::Mat Detector::get_label_map() {
	const cv::cuda::GpuMat hsv_image = gui->get_image(cv::COLOR_BGR2HSV);

	const cv::cuda::GpuMat blob_mask = get_blob_mask(hsv_image);
	const cv::cuda::GpuMat label_map = launch_color_segmentation(hsv_image, blob_mask);

	cv::Mat label_map_host;
	label_map.download(label_map_host);
	return label_map_host;
}

std::vector<Patch> Detector::get_patches(const cv::Mat& label_map) {
	std::vector<Patch> patches;
	int id = 0;

	for (int color = 1; color <= 7; color++) {
		cv::Mat mask;
		cv::compare(label_map, color, mask, cv::CMP_EQ);

		std::vector<std::vector<cv::Point>> contours;
		cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

		for (const auto& cnt : contours) {
			const double area = cv::contourArea(cnt);

			if (area > 25) {
				const cv::Moments m = cv::moments(cnt);
				if (m.m00 == 0) continue;

				const cv::Point2d center(m.m10 / m.m00, m.m01 / m.m00);
				Patch p;
				p.id = id++;
				p.color = color;
				p.centroid = center;
				p.parent = (color == 1 || color == 2);
				patches.emplace_back(p);
			}
		}
	}

	return patches;
}

std::vector<RobotPatch> Detector::get_robot_patches(const std::vector<Patch>& patches) {
	std::vector<RobotPatch> robots;

	for (const auto& parent : patches) {
		if (!parent.parent) continue;

		std::vector<Patch> robot_patches;
		for (const auto& child : patches) {
			if (child.parent) continue;

			if (distance(parent.centroid, child.centroid) <= 30) {
				robot_patches.emplace_back(child);
			}
		}

		if (robot_patches.size() == 2) {
			RobotPatch robot_patch;
			robot_patch.parent_patch = parent;
			robot_patch.child_patches = robot_patches;

			std::vector<Patch> complete_robot_patches;
			complete_robot_patches.insert(
				complete_robot_patches.end(),
				robot_patches.begin(),
				robot_patches.end()
				);

			robot_patch.patches = complete_robot_patches;

			bool accept_robot = true;

			for (const auto& child : robot_patch.child_patches) {
				for (const auto& patch : patches) {

					if (patch.id == parent.id || patch.id == robot_patches[0].id || patch.id == robot_patches[1].id) continue;

					if (distance(child.centroid, patch.centroid) <= 30) accept_robot = false;
				}
			}

			if (accept_robot) robots.emplace_back(robot_patch);
		}
	}

	std::vector<Patch> unmatched_patches; // THIS IS INCOMPLETE
	for (const auto& patch : patches) {
		bool unmatched = true;
		for (const auto& robot : robots) {
			if (std::any_of(robot.patches.begin(), robot.patches.end(),
				[&patch](const auto& p) { return p.id == patch.id; })) unmatched = false;
		}

		if (unmatched) unmatched_patches.push_back(patch);
	}

	std::cout << "\n=== REPORTE DE PATCHES HUERFANOS (Total: " << unmatched_patches.size() << ") ===" << std::endl;

	if (unmatched_patches.empty()) {
		std::cout << "   >> Todo perfecto: Todos los patches fueron asignados a robots." << std::endl;
	} else {
		for (const auto& p : unmatched_patches) {
			std::cout << "   -> [ID:" << p.id << "] "
					  << "Tipo: " << (p.parent ? "PADRE " : "HIJO  ")
					  << "| Color: " << p.color
					  << "| Pos: (" << p.centroid.x << ", " << p.centroid.y << ")"
					  << std::endl;
		}
	}
	std::cout << "========================================================\n" << std::endl;

	return robots;
}

void Detector::get_robot_data(std::vector<RobotPatch>& robot_patches) {
	for (auto& robot_patch : robot_patches) {
		robot_patch.center = find_geometric_median(robot_patch);


		double midpoint_x = 0;
		double midpoint_y = 0;
		for (const auto& p : robot_patch.child_patches) {
			midpoint_x += p.centroid.x;
			midpoint_y += p.centroid.y;
		}
		midpoint_x /= 2;
		midpoint_y /= 2;

		double dx = midpoint_x - robot_patch.center.x;
		double dy = midpoint_y - robot_patch.center.y;

		robot_patch.facing = std::atan2(dy, dx);
	}
}

void Detector::display_debug(cv::Mat& label_map, const std::vector<Patch>& patches, const std::vector<RobotPatch>& robot_patches) {
	cv::Mat centroid_view = cv::Mat::zeros({label_map.cols, label_map.rows}, CV_8UC1);
	cv::Mat robot_view = cv::Mat::zeros({label_map.cols, label_map.rows}, CV_8UC3);

	for (const auto& [id, color, centroid, parent] : patches) {
		const int radius = parent ? 5 : 2;
		cv::circle(centroid_view, centroid, radius, color, cv::FILLED);
	}

	for (const auto& robot_patch : robot_patches) {
		cv::circle(robot_view, robot_patch.center, 20, cv::Scalar(255, 255, 255), 2);
		cv::circle(robot_view, robot_patch.center, 4, cv::Scalar(0, 0, 255), cv::FILLED);

		int lx = static_cast<int>(robot_patch.center.x + 30 * std::cos(robot_patch.facing));
		int ly = static_cast<int>(robot_patch.center.y + 30 * std::sin(robot_patch.facing));

		cv::line(robot_view, robot_patch.center, {lx, ly}, cv::Scalar(255, 0, 0), 2);
	}

	const cv::Mat colored_label_mask = visualize_labels(label_map);
	const cv::Mat colored_centroid_mask = visualize_labels(centroid_view);
	cv::imshow("Colored label mask: ", colored_label_mask);
	cv::imshow("Colored centroid mask", colored_centroid_mask);
	cv::imshow("Robot view", robot_view);
}

void Detector::update() {
	cv::Mat label_map = get_label_map();
	const std::vector<Patch> patches = get_patches(label_map);

	std::vector<RobotPatch> robot_patches = get_robot_patches(patches);
	get_robot_data(robot_patches);

	#ifdef DEBUG_MODE
		display_debug(label_map, patches, robot_patches);
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

double Detector::distance(const cv::Point2d& point_1, const cv::Point2d& point_2) {
	return static_cast<double>(cv::norm(point_1 - point_2));
}

cv::Point2d Detector::find_geometric_median(const RobotPatch& robot_patch, const int iterations) {
	cv::Point2d median(0, 0);

	std::vector<cv::Point2d> points;
	for (const auto& p: robot_patch.child_patches) points.push_back(p.centroid);
	points.push_back(robot_patch.parent_patch.centroid);

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
}