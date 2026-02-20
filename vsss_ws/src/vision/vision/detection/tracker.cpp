#include "tracker.h"

#include "field_sizes.h"

void Tracker::Entity::init(const cv::Point2f start_pos, const double start_facing) {
	kf.init(4, 2, 0);

	// x' = x + vx, y' = y + vy
	kf.transitionMatrix = (cv::Mat_<float>(4, 4) <<
	1, 0, 1, 0,
	0, 1, 0, 1,
	0, 0, 1, 0,
	0, 0, 0, 1);

	kf.measurementMatrix = cv::Mat::eye(2, 4, CV_32F);

	cv::setIdentity(kf.processNoiseCov, cv::Scalar::all(1e-4));
	cv::setIdentity(kf.measurementNoiseCov, cv::Scalar::all(1e-2));
	cv::setIdentity(kf.errorCovPost, cv::Scalar::all(1));

	kf.statePost.at<float>(0) = start_pos.x;
	kf.statePost.at<float>(1) = start_pos.y;
	kf.statePost.at<float>(2) = 0;
	kf.statePost.at<float>(3) = 0;

	measurement = cv::Mat::zeros(2, 1, CV_32F);

	total_frames = 1;
	seen_frames = 1;
	accuracy = 1.0f;
	pos = start_pos;
	facing = start_facing;
	initialized = true;
}

ObjectType Tracker::Entity::team() const {
	if (!initialized) return ObjectType::None;

	if (id >= 0 && id <= 9) return ObjectType::Yellow;
	if (id >= 10 && id <= 19) return ObjectType::Blue;
	if (id == 20) return ObjectType::Ball;

	return ObjectType::None;
}

void Tracker::Entity::update(const std::optional<cv::Point2f> observed_pos, const std::optional<double> observed_facing) {
	if (!initialized && observed_pos.has_value()) {
		init(observed_pos.value(), observed_facing.value_or(0.0));
	}

	if (!initialized) return;

	total_frames++;
	cv::Mat prediction = kf.predict();

	if (observed_pos.has_value()) {
		measurement.at<float>(0) = observed_pos->x;
		measurement.at<float>(1) = observed_pos->y;
		kf.correct(measurement);

		pos.x = kf.statePost.at<float>(0);
		pos.y = kf.statePost.at<float>(1);

		blind_frames = 0;
		visible = true;
		seen_frames++;
	} else {
		pos.x = prediction.at<float>(0);
		pos.y = prediction.at<float>(1);
		blind_frames++;
		visible = false;
	}

	if (total_frames > 0) {
		accuracy = static_cast<float>(seen_frames) / static_cast<float>(total_frames);
	}

	vel.x = kf.statePost.at<float>(2);
	vel.y = kf.statePost.at<float>(3);

	if (observed_facing.has_value()) {
		const double raw_angle = observed_facing.value();

		const double x_curr = std::cos(facing);
		const double y_curr = std::sin(facing);

		const double x_new = std::cos(raw_angle);
		const double y_new = std::sin(raw_angle);

		const double x_final = (1.0 - FACING_ALPHA) * x_curr + (FACING_ALPHA * x_new);
		const double y_final = (1.0 - FACING_ALPHA) * y_curr + (FACING_ALPHA * y_new);

		facing = std::atan2(y_final, x_final);
	}
}

Tracker::Tracker(Coordinates* coordinates, GUI* gui) {
	this->coordinates = coordinates;
	this->gui = gui;
	ball.id = 20;
}

void Tracker::upload_infield_objects(const std::set<int>& infield_objects) {
	this->infield_objects = infield_objects;
}

void Tracker::update(const std::vector<RobotPatch>& detected_robots, const std::optional<BallPatch>& detected_ball) {
	std::optional<cv::Point2f> ball_pos = std::nullopt;

	if (detected_ball.has_value()) {
		ball_pos = coordinates->pixel_to_meter(detected_ball->center);
	}

	ball.update(ball_pos, std::nullopt);


	std::map<int, bool> seen_this_frame;
	for (const auto& pair : robots) {
		seen_this_frame[pair.first] = false;
	}

	for (const auto& r_patch : detected_robots) {
		if (r_patch.id == -1) continue;
		if (infield_objects.find(r_patch.id) == infield_objects.end()) continue;

		if (robots.find(r_patch.id) == robots.end()) {
			robots[r_patch.id].id = r_patch.id;
		}

		cv::Point2f pos = coordinates->pixel_to_meter(r_patch.center);

		double facing = -r_patch.facing;
		robots[r_patch.id].update(pos, facing);

		seen_this_frame[r_patch.id]	= true;
	}

	for (auto& [id, entity] : robots) {

		if (!seen_this_frame[id]) {
			entity.update(std::nullopt, std::nullopt);
		}
	}

	for (auto it = robots.begin(); it != robots.end(); ) {
		Entity& entity = it->second;
		int id = it->first;

		if (!seen_this_frame[id]) {

			if (entity.blind_frames > MAX_BLIND_FRAMES) {
				it = robots.erase(it);
				continue;
			}

			entity.update(std::nullopt, std::nullopt);
		}

		++it;
	}
}


void Tracker::display_debug_image(int width, int height) {
	cv::Mat map = cv::Mat::zeros(height, width, CV_8UC3);

	cv::Scalar grid_color(50, 50, 50);
	{
		cv::line(map,
	       coordinates->meter_to_minimap({0.0, FieldSizes::HALF_WIDTH}, width, height),
	       coordinates->meter_to_minimap({0.0, -FieldSizes::HALF_WIDTH}, width, height),
	       grid_color, FieldSizes::LINE_THICKNESS * 100);
	       
	    cv::circle(map,
	       coordinates->meter_to_minimap({0.0, 0.0}, width, height),
	       coordinates->meter_to_pixel_scalar(FieldSizes::CENTER_CIRCLE_RADIUS),
	       grid_color, FieldSizes::LINE_THICKNESS * 100);

	    auto draw_side = [&](double side) {
		    float x_limit = side * FieldSizes::HALF_LENGTH;
	        float x_area = side * FieldSizes::GOAL_AREA_X;
	        float x_cross = side * FieldSizes::CROSS_X_POS;

	        cv::line(map,
	           coordinates->meter_to_minimap({x_area, FieldSizes::GOAL_AREA_HALF_WIDTH}, width, height),
	           coordinates->meter_to_minimap({x_area, -FieldSizes::GOAL_AREA_HALF_WIDTH}, width, height),
	           grid_color, FieldSizes::LINE_THICKNESS * 100);
	        cv::line(map,
	           coordinates->meter_to_minimap({x_area, FieldSizes::GOAL_AREA_HALF_WIDTH}, width, height),
	           coordinates->meter_to_minimap({x_limit, FieldSizes::GOAL_AREA_HALF_WIDTH}, width, height),
	           grid_color, FieldSizes::LINE_THICKNESS * 100);
	        cv::line(map,
	           coordinates->meter_to_minimap({x_area, -FieldSizes::GOAL_AREA_HALF_WIDTH}, width, height),
	           coordinates->meter_to_minimap({x_limit, -FieldSizes::GOAL_AREA_HALF_WIDTH}, width, height),
	           grid_color, FieldSizes::LINE_THICKNESS * 100);

	        cv::ellipse(map,
	              coordinates->meter_to_minimap({x_area, 0.0}, width, height),
	              cv::Size(coordinates->meter_to_pixel_scalar(FieldSizes::PENALTY_ARC_RADIUS), 
	                       coordinates->meter_to_pixel_scalar(FieldSizes::PENALTY_ARC_RADIUS)),
	              (side > 0 ? 0 : 180), 270, 90, grid_color, FieldSizes::LINE_THICKNESS * 100);

	        double ys[] = {0.0, FieldSizes::PENALTY_MARK_OFFSET, -FieldSizes::PENALTY_MARK_OFFSET};
	        for (float y : ys) {
	            cv::line(map,
	               coordinates->meter_to_minimap({x_cross, static_cast<float>(y - FieldSizes::CROSS_SIZE)}, width, height),
	               coordinates->meter_to_minimap({x_cross, static_cast<float>(y + FieldSizes::CROSS_SIZE)}, width, height),
	               grid_color, FieldSizes::LINE_THICKNESS * 100);
	            cv::line(map,
	               coordinates->meter_to_minimap({static_cast<float>(x_cross + FieldSizes::CROSS_SIZE), y}, width, height),
	               coordinates->meter_to_minimap({static_cast<float>(x_cross - FieldSizes::CROSS_SIZE), y}, width, height),
	               grid_color, FieldSizes::LINE_THICKNESS * 100);
	        }

	        double dot_xs[] = {side * FieldSizes::DOT_X_OFFSET_INNER, side * FieldSizes::DOT_X_OFFSET_OUTER};
	        double dot_ys[] = {FieldSizes::PENALTY_MARK_OFFSET, -FieldSizes::PENALTY_MARK_OFFSET};
	        
	        for (float dx : dot_xs) {
	            for (float dy : dot_ys) {
	                cv::circle(map,
	                   coordinates->meter_to_minimap({dx, dy}, width, height),
	                   coordinates->meter_to_pixel_scalar(FieldSizes::DOT_RADIUS),
	                   grid_color, cv::FILLED);
	            }
	        }
	    };

	    draw_side(1.0);
	    draw_side(-1.0);
	}
    for (auto& pair : robots) {
        Entity& r = pair.second;
        if (!r.initialized) continue;

        cv::Scalar color = r.visible ? cv::Scalar(0, 255, 0) : cv::Scalar(0, 255, 255);

        cv::Point center_px = coordinates->meter_to_minimap(r.pos, width, height);

        float arrow_len = 0.15f;
        cv::Point2f tip_m = {
            static_cast<float>(r.pos.x + arrow_len * std::cos(r.facing)),
            static_cast<float>(r.pos.y + arrow_len * std::sin(r.facing))
        };
        cv::Point tip_px = coordinates->meter_to_minimap(tip_m, width, height);

        cv::circle(map, center_px, 10, color, 2);
        cv::arrowedLine(map, center_px, tip_px, color, 2);

    	int acc_pct = static_cast<int>(r.accuracy * 100.0f);
    	std::string label = "ID:" + std::to_string(r.id) + " (" + std::to_string(acc_pct) + "%)";

    	cv::putText(map, label, center_px + cv::Point(-10, -15),
			cv::FONT_HERSHEY_SIMPLEX, 0.4, color, 1);
    }

	if (ball.initialized) {
		cv::Point ball_px = coordinates->meter_to_minimap(ball.pos, width, height);
		cv::Scalar ball_color = ball.visible ? cv::Scalar(0, 165, 255) : cv::Scalar(0, 0, 180);
		cv::circle(map, ball_px, 6, ball_color, -1);

		auto now = std::chrono::steady_clock::now();
		std::chrono::duration<double> elapsed = now - last_prediction_time;

		const int time_in_future = 1;
		if (elapsed.count() >= time_in_future) {
			fixed_future_ball_pos = ball.pos + (ball.vel * 30 * time_in_future);

			last_prediction_time = now;
			has_prediction = true;
		}

		if (has_prediction) {
			cv::Point future_px = coordinates->meter_to_minimap(fixed_future_ball_pos, width, height);
			cv::circle(map, future_px, 8, cv::Scalar(255, 255, 0), 2);
			cv::line(map, ball_px, future_px, cv::Scalar(80, 80, 80), 1, cv::LINE_4);
		}
	}

	#ifdef DEBUG_MODE
		cv::cuda::GpuMat gpu_image = gui->get_image();

		cv::Mat image;
		cudaDeviceSynchronize();
		gpu_image.download(image);
		cv::Mat overlay = coordinates->get_warped_image(image);

		cv::Mat result;
		double alpha = 0.3;
		double beta = 1.0 - alpha;
		double gamma = 0.0;
		cv::addWeighted(map, beta, overlay, alpha, gamma, result);
		cv::imshow("Kalman view", result);
	#endif
}