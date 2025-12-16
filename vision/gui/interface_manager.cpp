#include "interface_manager.h"

InterfaceManager::InterfaceManager(GUI* drawer, BlobCalibrator* blob_calibrator, ColorCalibrator* color_calibrator, AppData* app_data)
    : drawer(drawer), blob_calibrator(blob_calibrator), color_calibrator(color_calibrator), app_data(app_data) {
    init_widgets();
}

void InterfaceManager::init_widgets() {
    main_menu_widgets.push_back(std::make_unique<Button>(drawer, 2, "MODO: DETECTION", [this](){
        app_data->current_state = AppState::DETECTION;
    }));
    main_menu_widgets.push_back(std::make_unique<Button>(drawer, 4, "MODO: BLOB CALIBRATION", [this](){
        app_data->current_state = AppState::BLOB_CALIBRATION_MENU;
    }));
	main_menu_widgets.push_back(std::make_unique<Button>(drawer, 5, "MODO: COLOR CALIBRATION", [this](){
		app_data->current_state = AppState::COLOR_CALIBRATING;
	}));
	main_menu_widgets.push_back(std::make_unique<Button>(drawer, 6, "MODO: ROI CALIBRATION", [this](){
		app_data->current_state = AppState::ROI_CALIBRATIING;
	}));
	main_menu_widgets.push_back(std::make_unique<Button>(drawer, 9, "SALIR", [this](){
		std::exit(0);
	}));

	auto pause_btn_ptr = std::make_unique<Button>(drawer, 8, "PAUSAR", [this]() {
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
		auto btn = std::make_unique<Button>(drawer, row, color, [=](){ select_color(color); });
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

	blob_calibration_menu_widgets.push_back(std::make_unique<Button>(drawer, 8, "VOLVER AL MENU", [this](){
        app_data->current_state = AppState::MAIN_MENU;
    }));
	blob_calibration_menu_widgets.push_back(std::make_unique<Button>(drawer, 9, "IMPRIMIR CALIBRACIONES", [this](){
		BlobCalibrator::print_calibrations(match_calibration);
	}));

    blob_calibration_tool_widgets.push_back(std::make_unique<Button>(drawer, 8, "Reset Points", [this](){
        blob_calibrator->reset_points();
    }));
    blob_calibration_tool_widgets.push_back(std::make_unique<Button>(drawer, 9, "GUARDAR Y VOLVER", [this](){
    	save_current_blob_calibration();
    }));


	color_calibration_tool_widgets.push_back(std::make_unique<Slider>(drawer, 0, "Saturacion", &app_data->params.saturation, 0.0f, 15.0f));
	color_calibration_tool_widgets.push_back(std::make_unique<Slider>(drawer, 1, "Gamma", &app_data->params.gamma_correction, 0.0f, 5.0f));
	color_calibration_tool_widgets.push_back(std::make_unique<Slider>(drawer, 2, "CLAHE Clip", &app_data->params.clahe_clip_limit, 0.1f, 5.0f));
	color_calibration_tool_widgets.push_back(std::make_unique<Slider>(drawer, 3, "Bilateral", &app_data->params.bilateral_sigma, 0.0f, 100.0f));
	color_calibration_tool_widgets.push_back(std::make_unique<Slider>(drawer, 4, "Green Boost", &app_data->params.green_boost, 0.2f, 2.0f));
	color_calibration_tool_widgets.push_back(std::make_unique<Slider>(drawer, 5, "Blue Boost", &app_data->params.blue_boost, 0.2f, 2.0f));
	color_calibration_tool_widgets.push_back(std::make_unique<Slider>(drawer, 6, "Red Boost", &app_data->params.red_boost, 0.2f, 2.0f));

	color_calibration_tool_widgets.push_back(std::make_unique<Button>(drawer, 7, "Reset Filter", [this](){
		app_data->params = VisionParams();
	}));
	color_calibration_tool_widgets.push_back(std::make_unique<Button>(drawer, 9, "GUARDAR Y VOLVER", [this](){
		app_data->current_state = AppState::MAIN_MENU;
	}));

	roi_calibration_tool_widgets.push_back(std::make_unique<Button>(drawer, 7, "Remove Last Point", [this](){
		color_calibrator->remove_last_point();
	}));
	roi_calibration_tool_widgets.push_back(std::make_unique<Button>(drawer, 8, "Reset Points", [this](){
		color_calibrator->reset_points();
	}));
	roi_calibration_tool_widgets.push_back(std::make_unique<Button>(drawer, 9, "GUARDAR Y VOLVER", [this](){
		app_data->current_state = AppState::MAIN_MENU;
		app_data->roi_points = color_calibrator->get_roi();
	}));
}

void InterfaceManager::save_current_blob_calibration() {
	if (!last_calculated_result.has_value() || !last_calculated_result->valid) {
		std::cout << "Error: No hay calibración válida para guardar." << std::endl;
		return;
	}

	const std::string color = app_data->current_color;
	const CalibrationResult res = last_calculated_result.value();

	if (color == "BLUE") match_calibration.blue = res;
	else if (color == "YELLOW") match_calibration.yellow = res;
	else if (color == "RED") match_calibration.red = res;
	else if (color == "GREEN") match_calibration.green = res;
	else if (color == "CYAN") match_calibration.cyan = res;
	else if (color == "MAGENTA") match_calibration.magenta = res;
	else if (color == "ORANGE") match_calibration.orange = res;

	if (color_buttons.count(color)) {
		const std::string& current_label = color;
		color_buttons[color]->set_label(current_label + " - OK");
	}

	std::cout << "--> Guardado: " << color << " con parametros propios." << std::endl;
	app_data->current_state = AppState::BLOB_CALIBRATION_MENU;
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
            drawer->text("MAIN MENU", 1, 1.0, true, -10);
            current_widgets = &main_menu_widgets;
            break;
        case AppState::BLOB_CALIBRATION_MENU:
            drawer->text("SELECCIONAR COLOR", 0, 0.8, true);
            current_widgets = &blob_calibration_menu_widgets;
            break;
        case AppState::BLOB_CALIBRATING:
            current_widgets = &blob_calibration_tool_widgets;
    		last_calculated_result = blob_calibrator->calibrate_individual();
            break;
    	case AppState::COLOR_CALIBRATING:
    		drawer->text("MODO COLOR CALIBRATION", 0, 0.8, true);
    		current_widgets = &color_calibration_tool_widgets;
    		break;
    	case AppState::ROI_CALIBRATIING:
    		drawer->text("MODO ROI CALIBRATION", 0, 0.8, true);
			current_widgets = &roi_calibration_tool_widgets;
    		color_calibrator->calibrate_roi();
    		break;
        case AppState::DETECTION:
            drawer->text("MODO DETECTION (ESC para salir)", 1, 0.6, false);
            break;
    }

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
		drawer->zoom(x, y, 2);
		drawer->display_frame();
	}
	if (event == cv::EVENT_MBUTTONDOWN) {
		drawer->reset_zoom();
		drawer->display_frame();
	}

    switch (app_data->current_state) {
        case AppState::MAIN_MENU: current_widgets = &main_menu_widgets; break;
        case AppState::BLOB_CALIBRATION_MENU: current_widgets = &blob_calibration_menu_widgets; break;
        case AppState::BLOB_CALIBRATING: current_widgets = &blob_calibration_tool_widgets; break;
    	case AppState::COLOR_CALIBRATING: current_widgets = &color_calibration_tool_widgets; break;
    	case AppState::ROI_CALIBRATIING: current_widgets = &roi_calibration_tool_widgets; break;
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
	if (!widget_consumed && app_data->current_state == AppState::ROI_CALIBRATIING) {
		ColorCalibrator::on_mouse(event, x, y, 0, color_calibrator);
	}
}