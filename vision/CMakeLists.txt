cmake_minimum_required(VERSION 3.22)
project(opencv)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)

# --- INCLUDES ---
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(include)
include_directories(lib)
include_directories(classes)
include_directories(gui)
include_directories(field)

# --- EJECUTABLE ---
add_executable(opencv main.cpp
		# Módulo de Procesamiento de Imagen
		lib/image_preprocessing.cpp         include/image_preprocessing.h
		lib/blob_detection.cpp              include/blob_detection.h

		# Módulo de Campo de Juego
		field/kinematics.cpp                field/kinematics.h
		field/field_object.cpp              field/field_object.h
		field/ball.cpp                      field/ball.h
		field/robot.cpp                     field/robot.h

		# Clases base
											include/app_settings.h
		classes/detection_method.cpp        classes/detection_method.h
		classes/calibrator.cpp              classes/calibrator.h

		# Interfaz Gráfica (GUI)
		gui/gui.cpp gui/gui.h
		gui/widget.cpp                      gui/widget.h
		gui/button.cpp                      gui/button.h
		gui/slider.cpp                      gui/slider.h
		gui/interface_manager.cpp           gui/interface_manager.h
)

# --- LINKING ---
target_link_libraries(opencv ${OpenCV_LIBS})