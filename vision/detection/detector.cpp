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
		const std::vector<std::shared_ptr<Patch>>& patches
	) {
		std::vector<std::shared_ptr<Patch>> candidates;
		for (const auto& patch : patches) {
			if (patch->id == center->id) continue;
			if (patch->parent) continue;
			if (distance(center->centroid, patch->centroid) <= 20) candidates.push_back(patch);
		}

		return candidates;
	}

	std::vector<std::shared_ptr<Patch>> find_child_1_candidates(
		const std::shared_ptr<Patch>& parent,
		const std::vector<std::shared_ptr<Patch>>& patches,
		const std::vector<RobotRelationship>& relationship
	) {
		const std::vector<std::shared_ptr<Patch>> candidates = find_all_candidates(parent, patches);
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
		const std::vector<RobotRelationship>& relationship
	) {
		const std::vector<std::shared_ptr<Patch>> candidates = find_all_candidates(parent, patches);
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

std::vector<std::shared_ptr<Patch>> Detector::get_patches(const cv::Mat& label_map) {
	std::vector<std::shared_ptr<Patch>> patches;
	int id = 0;

	for (int color = 1; color <= 7; color++) {
		cv::Mat mask;
		cv::compare(label_map, color, mask, cv::CMP_EQ);

		std::vector<std::vector<cv::Point>> contours;
		cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

		for (const auto& cnt : contours) {
			const double area = cv::contourArea(cnt);

			if (area > 10) {
				const cv::Moments m = cv::moments(cnt);
				if (m.m00 == 0) continue;
				cv::Rect bbox = cv::boundingRect(cnt);
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
	return patches;
}

std::vector<RobotPatch> Detector::get_robot_patches(std::vector<std::shared_ptr<Patch>>& patches) {
	std::vector<RobotPatch> isolated_robots = get_isolated_robot_patches(patches);
	std::vector<RobotPatch> clustered_robots;

	std::vector<std::shared_ptr<Patch>> clustered_patches;
	std::vector<std::shared_ptr<Patch>> isolated_patches;

	for (const auto& robot : isolated_robots) {
		isolated_patches.insert(isolated_patches.end(), robot.patches.begin(), robot.patches.end());
	}

	auto compare_by_id = [](const std::shared_ptr<Patch>& a, const std::shared_ptr<Patch>& b) -> bool {
		return a->id < b->id;
	};

	std::sort(patches.begin(), patches.end(), compare_by_id);
	std::sort(isolated_patches.begin(), isolated_patches.end(), compare_by_id);

	std::set_difference(
		patches.begin(), patches.end(),
		isolated_patches.begin(), isolated_patches.end(),
		std::back_inserter(clustered_patches),
		compare_by_id
	);

	clustered_robots = get_clustered_robot_patches(clustered_patches);

	std::vector<RobotPatch> all_robots;
	all_robots.reserve(clustered_robots.size() + isolated_robots.size());

	all_robots.insert(all_robots.end(), clustered_robots.begin(), clustered_robots.end());
	all_robots.insert(all_robots.end(), isolated_robots.begin(), isolated_robots.end());

	return all_robots;
}

std::optional<BallPatch> Detector::get_ball_patch(const std::vector<std::shared_ptr<Patch>>& patches) {
	if (patches.empty()) return std::nullopt;
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
		return ball;
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

			if (distance(p1->centroid, p2->centroid) <= 20) {
				is_isolated = false;
				break;
			}
		}

		if (is_isolated) {
			ball.patch = p1;
			ball.center = p1->centroid;
			return ball;
		}
	}

	return std::nullopt;
}

std::vector<RobotPatch> Detector::get_clustered_robot_patches(const std::vector<std::shared_ptr<Patch>>& clustered_patches) {
	std::vector<std::shared_ptr<Patch>> patches = clustered_patches;
	std::vector<RobotPatch> robot_candidates;
	std::vector<RobotRelationship> relationship;
	int iterations = 0;
	constexpr int max_iterations = 1000;

	while (!patches.empty()) {
		if ((iterations++) > max_iterations) return {};
		std::vector<std::shared_ptr<Patch>> parents;
		std::vector<std::shared_ptr<Patch>> children;

		std::shared_ptr<Patch> parent;
		std::shared_ptr<Patch> child_1;
		std::shared_ptr<Patch> child_2;

		std::vector<std::shared_ptr<Patch>> child_1_candidates;
		std::vector<std::shared_ptr<Patch>> child_2_candidates;

		for (const auto& p : patches) {
			if (p->parent) {
				parents.emplace_back(p);
			} else {
				children.emplace_back(p);
			}
		}

		if (parents.empty()) {
			// std::cout << "[ERROR] No parent found" << std::endl;
			return {};
		}

		if ((parents.size() * 2) != children.size()) {
			// std::cout << "[ERROR] Invalid proportion" << std::endl;
			return {};
		}

		parent = parents.front();

		child_1_candidates = find_child_1_candidates(parent, patches, relationship);
		if (child_1_candidates.empty()) {
			patches = clustered_patches;
			robot_candidates.clear();
			continue;
		}
		child_1 = child_1_candidates.front();

		child_2_candidates = find_child_2_candidates(parent, child_1, patches, relationship);
		if (child_2_candidates.empty()) {
			patches = clustered_patches;
			robot_candidates.clear();
			relationship.push_back({parent->id, child_1->id});
			continue;
		}
		child_2 = child_2_candidates.front();

		relationship.push_back({parent->id, child_1->id, child_2->id});
		RobotPatch robot;
		robot.parent = parent;
		robot.children = {child_1, child_2};
		robot.patches = {parent, child_1, child_2};
		robot_candidates.push_back(robot);

		remove_patch_by_id(patches, parent->id);
		remove_patch_by_id(patches, child_1->id);
		remove_patch_by_id(patches, child_2->id);
		int debug_dummy = 0;
	}

	return robot_candidates;
}

void Detector::get_robot_data(std::vector<RobotPatch>& robot_patches) {
    for (auto& robot_patch : robot_patches) {
       robot_patch.center = find_geometric_median(robot_patch);

       double midpoint_x = (robot_patch.children[0]->centroid.x + robot_patch.children[1]->centroid.x) / 2.0;
       double midpoint_y = (robot_patch.children[0]->centroid.y + robot_patch.children[1]->centroid.y) / 2.0;

       cv::Point2d forward_temp(midpoint_x - robot_patch.center.x, midpoint_y - robot_patch.center.y);
       cv::Point2d v0 = robot_patch.children[0]->centroid - robot_patch.center;

       double cross = forward_temp.x * v0.y - forward_temp.y * v0.x;

       cv::Point2d p_left, p_right;
       unsigned char left_color, right_color;

       if (cross > 0) {
          p_left = robot_patch.children[0]->centroid;
          p_right = robot_patch.children[1]->centroid;
          left_color  = static_cast<unsigned char>(robot_patch.children[0]->color);
          right_color = static_cast<unsigned char>(robot_patch.children[1]->color);
       } else {
          p_left = robot_patch.children[1]->centroid;
          p_right = robot_patch.children[0]->centroid;
          left_color  = static_cast<unsigned char>(robot_patch.children[1]->color);
          right_color = static_cast<unsigned char>(robot_patch.children[0]->color);
       }

       double dx_LR = p_right.x - p_left.x;
       double dy_LR = p_right.y - p_left.y;

       robot_patch.facing = std::atan2(dx_LR, -dy_LR);

       robot_patch.id = RobotIdentities::get_id(
          static_cast<unsigned char>(robot_patch.parent->color),
          left_color,
          right_color
       );
    }
}


void Detector::display_debug(cv::Mat& label_map, const std::vector<std::shared_ptr<Patch>>& patches, const std::vector<RobotPatch>& robot_patches, std::optional<BallPatch>& ball_patch) {
    cv::Mat centroid_view = cv::Mat::zeros(label_map.size(), CV_8UC1);
    cv::Mat robot_view = cv::Mat::zeros(label_map.size(), CV_8UC3);

    for (const auto& p : patches) {
        const int radius = p->parent ? 5 : 2;
        cv::circle(centroid_view, p->centroid, radius, static_cast<int>(p->color), cv::FILLED);
    }

    for (const auto& robot : robot_patches) {
        cv::circle(robot_view, robot.center, 20, cv::Scalar(255, 255, 255), 1, cv::LINE_AA);

        float angle = robot.facing;

        int px = static_cast<int>(robot.parent->centroid.x);
        int py = static_cast<int>(robot.parent->centroid.y);

        float angle_left = angle - 0.6f;
        float angle_right = angle + 0.6f;

        int c1x = static_cast<int>(robot.children.at(0)->centroid.x);
        int c1y = static_cast<int>(robot.children.at(0)->centroid.y);

        int c2x = static_cast<int>(robot.children.at(1)->centroid.x);
        int c2y = static_cast<int>(robot.children.at(1)->centroid.y);

    	draw_rotated_rect(robot_view, { (float)px, (float)py }, cv::Size2f(10, 20), angle, get_debug_color(robot.parent->color));
   		draw_rotated_rect(robot_view, { (float)c1x, (float)c1y }, cv::Size2f(10, 10), angle, get_debug_color(robot.children[0]->color));
   		draw_rotated_rect(robot_view, { (float)c2x, (float)c2y }, cv::Size2f(10, 10), angle, get_debug_color(robot.children[1]->color));

        int tip_x = static_cast<int>(robot.center.x + 20 * std::cos(angle));
        int tip_y = static_cast<int>(robot.center.y + 20 * std::sin(angle));
        cv::line(robot_view, robot.center, {tip_x, tip_y}, cv::Scalar(200, 200, 200), 2);
    	cv::putText(robot_view, "id: " + std::to_string(robot.id), robot.center + cv::Point2d(10,-10),
			cv::FONT_HERSHEY_PLAIN, 0.8, cv::Scalar(255,255,255), 1);
    }

    if (ball_patch.has_value()) {
       cv::circle(robot_view, ball_patch->center, 6, cv::Scalar(0, 165, 255), cv::FILLED);
       cv::circle(robot_view, ball_patch->center, 8, cv::Scalar(255, 255, 255), 1);
       cv::putText(robot_view, "BALL", ball_patch->center + cv::Point2d(10,-10),
                   cv::FONT_HERSHEY_PLAIN, 0.8, cv::Scalar(255,255,255), 1);
    }

    const cv::Mat colored_label_mask = visualize_labels(label_map);
    const cv::Mat colored_centroid_mask = visualize_labels(centroid_view);

    cv::imshow("Colored label mask: ", colored_label_mask);
    cv::imshow("Colored centroid mask", colored_centroid_mask);
    cv::imshow("Robot view", robot_view);
}

std::pair<std::vector<RobotPatch>, std::optional<BallPatch>> Detector::update() {
	cv::Mat label_map = get_label_map();
	std::vector<std::shared_ptr<Patch>> patches = get_patches(label_map);

	std::optional<BallPatch> ball_patch = get_ball_patch(patches);

	if (ball_patch.has_value()) {
		int target_id = ball_patch.value().patch->id;
		const auto it = std::remove_if(patches.begin(), patches.end(),
			[target_id](const auto& p) {
				return p->id == target_id;
			});

		patches.erase(it, patches.end());
	}

	std::vector<RobotPatch> robot_patches = get_robot_patches(patches);
	get_robot_data(robot_patches);

	#ifdef DEBUG_MODE
		display_debug(label_map, patches, robot_patches, ball_patch);
	#endif

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

std::vector<RobotPatch> Detector::get_isolated_robot_patches(std::vector<std::shared_ptr<Patch>>& patches) {
	std::vector<RobotPatch> isolated_robots;

	std::vector<std::shared_ptr<Patch>> parents;
	std::vector<std::shared_ptr<Patch>> children;

	for (const auto& p : patches) {
		if (p->parent) parents.emplace_back(p);
		else children.emplace_back(p);
	}

	for (const auto& parent : parents) {
		std::vector<std::shared_ptr<Patch>> candidates;
		for (const auto& child : children) {
			if (distance(parent->centroid, child->centroid) <= 20) {
				candidates.emplace_back(child);
			}
		}

		if (candidates.size() == 2) {
			bool accept_candidates = true;

			for (const auto& child : candidates) {
				for (const auto& patch : patches) {
					if (
						patch->id == parent->id
						|| patch->id == candidates[0]->id
						|| patch->id == candidates[1]->id) continue;

					if (distance(child->centroid, patch->centroid) <= 20) accept_candidates = false;
				}
			}

			if (accept_candidates) {
				RobotPatch robot_patch;
				robot_patch.parent = parent;
				robot_patch.children = candidates;
				robot_patch.patches.push_back(parent);
				robot_patch.patches.push_back(candidates[0]);
				robot_patch.patches.push_back(candidates[1]);
				isolated_robots.push_back(robot_patch);
			}
		}
	}

	return isolated_robots;
}
