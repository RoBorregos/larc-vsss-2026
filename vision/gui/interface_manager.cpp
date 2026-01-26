#include "interface_manager.h"

#include "camera_calibrator.h"

InterfaceManager::InterfaceManager(GUI* drawer, BlobCalibrator* blob_calibrator, ColorCalibrator* color_calibrator, CameraCalibrator* camera_calibrator, AppData* app_data, Detector* detector)
    : gui(drawer), blob_calibrator(blob_calibrator), color_calibrator(color_calibrator), camera_calibrator(camera_calibrator), app_data(app_data), detector(detector) {
    init_widgets();
}

void InterfaceManager::init_widgets() {
    main_menu_widgets.push_back(std::make_unique<Button>(gui, 2, "MODO: DETECTION", [this](){
        app_data->current_state = AppState::DETECTION;
    }));
    main_menu_widgets.push_back(std::make_unique<Button>(gui, 3, "MODO: BLOB CALIBRATION", [this](){
        app_data->current_state = AppState::BLOB_CALIBRATION_MENU;
    	app_data->paused = true;
    }));
	main_menu_widgets.push_back(std::make_unique<Button>(gui, 4, "MODO: COLOR CALIBRATION", [this](){
		app_data->current_state = AppState::COLOR_CALIBRATING;
    	app_data->paused = true;
	}));
	main_menu_widgets.push_back(std::make_unique<Button>(gui, 5, "MODO: CAMERA CALIBRATION", [this]() {
		app_data->current_state = AppState::CAMERA_CALIBRATING;
	}));
	main_menu_widgets.push_back(std::make_unique<Button>(gui, 6, "MODO: MASK CALIBRATION", [this](){
		app_data->current_state = AppState::MASK_CALIBRATING;
		app_data->paused = true;
	}));
	main_menu_widgets.push_back(std::make_unique<Button>(gui, 7, "MODO: ROI CALIBRATION", [this](){
		app_data->current_state = AppState::ROI_CALIBRATING;
    	app_data->paused = true;
	}));
	main_menu_widgets.push_back(std::make_unique<Button>(gui, 9, "SALIR", [this](){
		std::exit(0);
	}));

	auto pause_btn_ptr = std::make_unique<Button>(gui, 8, "PAUSAR", [this]() {
		app_data->paused = !app_data->paused;
	});
	pause_button = pause_btn_ptr.get();
	main_menu_widgets.push_back(std::move(pause_btn_ptr));


    auto select_color = [this](std::string color) {
        app_data->current_color = std::move(color);
        blob_calibrator->reset_points();
        app_data->current_state = AppState::BLOB_CALIBRATING;
    };

	auto create_color_btn = [&](const int row, const std::string& color) {
		auto btn = std::make_unique<Button>(gui, row, color, [=](){ select_color(color); });
		color_buttons[color] = btn.get();
		blob_calibration_menu_widgets.push_back(std::move(btn));
	};

	create_color_btn(1, "BLUE");
	create_color_btn(2, "YELLOW");
	create_color_btn(3, "RED");
	create_color_btn(4, "GREEN");
	create_color_btn(5, "CYAN");
	create_color_btn(6, "MAGENTA");
	create_color_btn(7, "ORANGE");

	blob_calibration_menu_widgets.push_back(std::make_unique<Button>(gui, 8, "VOLVER AL MENU", [this](){
        app_data->current_state = AppState::MAIN_MENU;
    }));
	blob_calibration_menu_widgets.push_back(std::make_unique<Button>(gui, 9, "IMPRIMIR CALIBRACIONES", [this](){
		BlobCalibrator::print_calibrations(app_data->match_calibration);
	}));

    blob_calibration_tool_widgets.push_back(std::make_unique<Button>(gui, 8, "Reset Points", [this](){
        blob_calibrator->reset_points();
    }));
    blob_calibration_tool_widgets.push_back(std::make_unique<Button>(gui, 9, "GUARDAR Y VOLVER", [this](){
    	save_current_blob_calibration();
    }));

	mask_calibration_tool_widgets.push_back(std::make_unique<Slider>(gui, 1, "H Min", &app_data->mask_params.h_min, 0, 180));
	mask_calibration_tool_widgets.push_back(std::make_unique<Slider>(gui, 2, "S Min", &app_data->mask_params.s_min, 0, 255));
	mask_calibration_tool_widgets.push_back(std::make_unique<Slider>(gui, 3, "V Min", &app_data->mask_params.v_min, 0, 255));
	mask_calibration_tool_widgets.push_back(std::make_unique<Slider>(gui, 4, "H Max", &app_data->mask_params.h_max, 0, 180));
	mask_calibration_tool_widgets.push_back(std::make_unique<Slider>(gui, 5, "S Max", &app_data->mask_params.s_max, 0, 255));
	mask_calibration_tool_widgets.push_back(std::make_unique<Slider>(gui, 6, "V Max", &app_data->mask_params.v_max, 0, 255));
	mask_calibration_tool_widgets.push_back(std::make_unique<Button>(gui, 8, "Reset Values", [this](){
		app_data->mask_params = MaskParams();
	}));
	mask_calibration_tool_widgets.push_back(std::make_unique<Button>(gui, 9, "GUARDAR Y VOLVER", [this](){
		app_data->current_state = AppState::MAIN_MENU;
		color_calibrator->save_calibration(app_data->calibration_filename);
	}));

	color_calibration_tool_widgets.push_back(std::make_unique<Slider>(gui, 0, "Saturacion", &app_data->color_params.saturation, 0.0f, 15.0f));
	color_calibration_tool_widgets.push_back(std::make_unique<Slider>(gui, 1, "Gamma S", &app_data->color_params.gamma_correction_s, 0.0f, 5.0f));
	color_calibration_tool_widgets.push_back(std::make_unique<Slider>(gui, 2, "Gamma V", &app_data->color_params.gamma_correction_v, 0.0f, 5.0f));
	color_calibration_tool_widgets.push_back(std::make_unique<Slider>(gui, 3, "CLAHE Clip", &app_data->color_params.clahe_clip_limit, 0.1f, 5.0f));
	color_calibration_tool_widgets.push_back(std::make_unique<Slider>(gui, 4, "Bilateral", &app_data->color_params.bilateral_sigma, 0.0f, 100.0f));
	color_calibration_tool_widgets.push_back(std::make_unique<Slider>(gui, 5, "Green Boost", &app_data->color_params.green_boost, 0.2f, 2.0f));
	color_calibration_tool_widgets.push_back(std::make_unique<Slider>(gui, 6, "Blue Boost", &app_data->color_params.blue_boost, 0.2f, 2.0f));
	color_calibration_tool_widgets.push_back(std::make_unique<Slider>(gui, 7, "Red Boost", &app_data->color_params.red_boost, 0.2f, 2.0f));
	color_calibration_tool_widgets.push_back(std::make_unique<Button>(gui, 8, "Reset Filter", [this](){
		app_data->color_params = VisionParams();
	}));
	color_calibration_tool_widgets.push_back(std::make_unique<Button>(gui, 9, "GUARDAR Y VOLVER", [this](){
		app_data->current_state = AppState::MAIN_MENU;
		color_calibrator->save_calibration(app_data->calibration_filename);
	}));

	auto cam_callback = [this] { camera_calibrator->upload_to_camera(); };
	camera_calibration_tool_widgets.push_back(std::make_unique<Slider>(gui, 0, "Brillo", &app_data->camera_params.brightness, 0.0f, 255.0f, cam_callback));
	camera_calibration_tool_widgets.push_back(std::make_unique<Slider>(gui, 1, "Contrast", &app_data->camera_params.contrast, 0.0f, 255.0f, cam_callback));
	camera_calibration_tool_widgets.push_back(std::make_unique<Toggle>(gui, 2, "Auto-exposure", &app_data->camera_params.auto_exposure, cam_callback));
	camera_calibration_tool_widgets.push_back(std::make_unique<Slider>(gui, 3, "Exposure", &app_data->camera_params.exposure, 0, 1000.0f, cam_callback));
	camera_calibration_tool_widgets.push_back(std::make_unique<Toggle>(gui, 4, "Auto-focus", &app_data->camera_params.auto_focus, cam_callback));
	camera_calibration_tool_widgets.push_back(std::make_unique<Slider>(gui, 5, "Focus", &app_data->camera_params.focus, 0.0f, 255.0f, cam_callback));
	camera_calibration_tool_widgets.push_back(std::make_unique<Button>(gui, 8, "Reset parameters", [this]() {
		camera_calibrator->reset_parameters();
		camera_calibrator->upload_to_camera();
	}));
	camera_calibration_tool_widgets.push_back(std::make_unique<Button>(gui, 9, "GUARDAR Y VOLVER", [this]() {
		app_data->current_state = AppState::MAIN_MENU;
		camera_calibrator->save_calibration(app_data->calibration_filename);
	}));


	roi_calibration_tool_widgets.push_back(std::make_unique<Button>(gui, 7, "Remove Last Point", [this](){
		color_calibrator->remove_last_point();
	}));
	roi_calibration_tool_widgets.push_back(std::make_unique<Button>(gui, 8, "Reset Points", [this](){
		color_calibrator->reset_points();
	}));
	roi_calibration_tool_widgets.push_back(std::make_unique<Button>(gui, 9, "GUARDAR Y VOLVER", [this](){
		app_data->current_state = AppState::MAIN_MENU;
		app_data->roi_points = color_calibrator->get_roi();
		color_calibrator->save_calibration(app_data->calibration_filename);
	}));
}

void InterfaceManager::save_current_blob_calibration() {
	if (!last_calculated_result.has_value() || !last_calculated_result->valid) {
		std::cout << "Error: No hay calibración válida para guardar." << std::endl;
		return;
	}

	const std::string color = app_data->current_color;
	const CalibrationResult res = last_calculated_result.value();
	unsigned long size = 0;

	if (color == "BLUE") {
		app_data->match_calibration.blue.push_back(res);
		size = app_data->match_calibration.blue.size();
	} else if (color == "YELLOW") {
		app_data->match_calibration.yellow.push_back(res);
		size = app_data->match_calibration.yellow.size();
	} else if (color == "RED") {
		app_data->match_calibration.red.push_back(res);
		size = app_data->match_calibration.red.size();
	} else if (color == "GREEN") {
		app_data->match_calibration.green.push_back(res);
		size = app_data->match_calibration.green.size();
	} else if (color == "CYAN") {
		app_data->match_calibration.cyan.push_back(res);
		size = app_data->match_calibration.cyan.size();
	} else if (color == "MAGENTA") {
		app_data->match_calibration.magenta.push_back(res);
		size = app_data->match_calibration.magenta.size();
	} else if (color == "ORANGE") {
		app_data->match_calibration.orange.push_back(res);
		size = app_data->match_calibration.orange.size();
	}

	if (color_buttons.count(color)) {
		const std::string& current_label = color;
		color_buttons[color]->set_label(current_label + " - OK (" + std::to_string(size) + ")");
	}

	std::cout << "--> Guardado: " << color << " con parametros propios." << std::endl;
	app_data->current_state = AppState::BLOB_CALIBRATION_MENU;
	blob_calibrator->save_calibration(app_data->calibration_filename);
}


void InterfaceManager::draw_interface() {
    std::vector<std::unique_ptr<Widget>>* current_widgets = nullptr;

	if (app_data->paused) {
		pause_button->set_label("REANUDAR");
	} else {
		pause_button->set_label("PAUSAR");
	}

    switch (app_data->current_state) {
        case AppState::MAIN_MENU:
            gui->text("MAIN MENU", 1, Drawer::Layer::INTERFACE, 1.0, true, -10);
            current_widgets = &main_menu_widgets;
            break;
        case AppState::BLOB_CALIBRATION_MENU:
            gui->text("SELECCIONAR COLOR", 0, Drawer::Layer::INTERFACE, 0.8, true);
            current_widgets = &blob_calibration_menu_widgets;
            break;
        case AppState::BLOB_CALIBRATING:
            current_widgets = &blob_calibration_tool_widgets;
    		last_calculated_result = blob_calibrator->calibrate_individual();
            break;
    	case AppState::COLOR_CALIBRATING:
    		current_widgets = &color_calibration_tool_widgets;
    		break;
		case AppState::CAMERA_CALIBRATING:
    		current_widgets = &camera_calibration_tool_widgets;
    		break;
    	case AppState::MASK_CALIBRATING:
    		gui->text("MODO MASK CALIBRATION", 0, Drawer::Layer::INTERFACE, 0.8, true);
			current_widgets = &mask_calibration_tool_widgets;
			break;
    	case AppState::ROI_CALIBRATING:
    		gui->text("MODO ROI CALIBRATION", 0, Drawer::Layer::INTERFACE, 0.8, true);
			current_widgets = &roi_calibration_tool_widgets;
    		color_calibrator->calibrate_roi();
    		break;
        case AppState::DETECTION:
            gui->text("MODO DETECTION (ESC para salir)", 1, Drawer::Layer::INTERFACE, 0.6, false);
            break;
    }
	// detector->display_debug_info();

    if (current_widgets) {
        for (const auto& widget : *current_widgets) {
            widget->draw();
        }
    }
}

void InterfaceManager::on_mouse(int event, int x, int y, int flags, void* userdata) {
    auto* manager = static_cast<InterfaceManager*>(userdata);
    manager->handle_input(event, x, y);
}

void InterfaceManager::handle_input(int event, int x, int y) {
	std::vector<std::unique_ptr<Widget>>* current_widgets = nullptr;

	if (event == cv::EVENT_RBUTTONDOWN) {
		gui->zoom(x, y, 2);
		gui->display_frame();
	}
	if (event == cv::EVENT_MBUTTONDOWN) {
		gui->reset_zoom();
		gui->display_frame();
	}

	if (event == cv::EVENT_LBUTTONDBLCLK) {
		cv::Point real_coords = gui->screen_to_world(cv::Point(x, y));

		const cv::cuda::GpuMat img = gui->get_image(cv::COLOR_BGR2HSV);

		if (real_coords.x >= 0 && real_coords.y >= 0 &&
			real_coords.x < img.cols && real_coords.y < img.rows) {

			std::cout << "Clicked at screen(" << x << ", " << y << ")"
					  << " world: [" << real_coords.x << ", " << real_coords.y << "]" << std::endl;

			const cv::Rect roi(real_coords.x, real_coords.y, 1, 1);
			const cv::cuda::GpuMat gpu_pixel_region(img, roi);

			cv::Mat cpu_pixel_1x1;
			gpu_pixel_region.download(cpu_pixel_1x1);

			const cv::Vec3b hsv_pixel = cpu_pixel_1x1.at<cv::Vec3b>(0, 0);
			std::cout << "HSV Value: [" << static_cast<int>(hsv_pixel[0]) << ", "
					  << static_cast<int>(hsv_pixel[1]) << ", "
					  << static_cast<int>(hsv_pixel[2]) << "]" << std::endl;
			} else {
				std::cout << "Ignorado: Click fuera de los límites de la imagen." << std::endl;
			}
	}

    switch (app_data->current_state) {
        case AppState::MAIN_MENU: current_widgets = &main_menu_widgets; break;
        case AppState::BLOB_CALIBRATION_MENU: current_widgets = &blob_calibration_menu_widgets; break;
        case AppState::BLOB_CALIBRATING: current_widgets = &blob_calibration_tool_widgets; break;
		case AppState::CAMERA_CALIBRATING: current_widgets = &camera_calibration_tool_widgets; break;
    	case AppState::COLOR_CALIBRATING: current_widgets = &color_calibration_tool_widgets; break;
    	case AppState::MASK_CALIBRATING: current_widgets = &mask_calibration_tool_widgets; break;
    	case AppState::ROI_CALIBRATING: current_widgets = &roi_calibration_tool_widgets; break;
        default: break;
    }

    bool widget_consumed = false;
    if (current_widgets) {
        for (const auto& widget : *current_widgets) {
            if (widget->handle_input(x, y, event)) {
                widget_consumed = true;
            }
        }
    }

	if (!widget_consumed && app_data->current_state == AppState::BLOB_CALIBRATING) {
        BlobCalibrator::on_mouse(event, x, y, 0, blob_calibrator);
    }
	if (!widget_consumed && app_data->current_state == AppState::ROI_CALIBRATING) {
		ColorCalibrator::on_mouse(event, x, y, 0, color_calibrator);
	}
}