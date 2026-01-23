cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_COMPILER gcc-10)
set(CMAKE_CXX_COMPILER g++-10)
set(CMAKE_CUDA_HOST_COMPILER g++-10)

project(opencv LANGUAGES CXX CUDA)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CUDA_ARCHITECTURES native)

find_package(OpenCV REQUIRED)
find_package(CUDAToolkit REQUIRED)

# --- INCLUDES ---
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(include)
include_directories(lib)
include_directories(detection)
include_directories(calibration)
include_directories(gui)
include_directories(field)

# --- DEFINITIONS ---
add_definitions(-DDEBUG_MODE)

# --- EJECUTABLE ---
add_executable(opencv main.cpp
		# Archivos base
		include/app_settings.h
		include/json.hpp

		# Módulo de Procesamiento de Imagen
		lib/image_preprocessing.cpp         include/image_preprocessing.h
		lib/blob_detection.cpp              include/blob_detection.h

		# Módulo de Campo de Juego
		field/kinematics.cpp                field/kinematics.h
		field/field_object.cpp              field/field_object.h
		#      field/ball.cpp.bak                  field/ball.h.bak
		field/robot.cpp                     field/robot.h

		# Clases de detección
		detection/detector.cpp              detection/detector.h
		detection/color_segmentation.cu     detection/color_segmentation.cuh

		# Clases de calibración
		calibration/blob_calibrator.cpp     calibration/blob_calibrator.h
		calibration/color_calibrator.cpp    calibration/color_calibrator.h

		# Archivo CUDA
		calibration/blob_kernels.cu calibration/blob_kernels.h

		# Interfaz Gráfica (GUI)
		gui/gui.cpp                         gui/gui.h
		gui/widget.cpp                      gui/widget.h
		gui/button.cpp                      gui/button.h
		gui/slider.cpp                      gui/slider.h
		gui/interface_manager.cpp           gui/interface_manager.h
		gui/drawer.cpp					    gui/drawer.h
)

# --- LINKING ---
target_link_libraries(opencv ${OpenCV_LIBS} CUDA::cudart)