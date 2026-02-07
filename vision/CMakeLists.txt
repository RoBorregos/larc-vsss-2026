cmake_minimum_required(VERSION 3.22)

# --- 1. CRITICAL: FORCE CUDA PATHS BEFORE PROJECT() ---
# We point directly to your manual installation to avoid the system default (12.0)
set(CMAKE_CUDA_COMPILER /usr/local/cuda/bin/nvcc)
set(CUDA_TOOLKIT_ROOT_DIR /usr/local/cuda)

# --- 2. COMPILER CONFIGURATION ---
set(CMAKE_C_COMPILER gcc-13)
set(CMAKE_CXX_COMPILER g++-13)
set(CMAKE_CUDA_HOST_COMPILER g++-13)

# --- 3. LEGACY POLICY FIX ---
# OpenCV config uses the deprecated "FindCUDA" module.
# We must explicitly tell CMake to allow this to prevent the "CMP0146" warning/error.
cmake_policy(SET CMP0146 OLD)

# --- 4. PROJECT DEFINITION ---
project(opencv LANGUAGES CXX CUDA)

# --- 5. STANDARD & ARCHITECTURE ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CUDA_ARCHITECTURES 89)

# --- 6. PACKAGES ---
find_package(OpenCV REQUIRED)
find_package(CUDAToolkit REQUIRED)

# --- 7. INCLUDES ---
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(include)
include_directories(lib)
include_directories(detection)
include_directories(calibration)
include_directories(gui)
include_directories(field)

# --- 8. DEFINITIONS
add_definitions(-DDEBUG_MODE)

# --- 9. EXECUTABLE ---
add_executable(opencv main.cpp
		# Base Files
		include/app_settings.h
		include/robot_identities.h
		include/json.hpp

		# Image Processing Module
		lib/image_preprocessing.cpp         include/image_preprocessing.h
		lib/blob_detection.cpp              include/blob_detection.h

		# Detection Classes
		detection/detector.cpp              detection/detector.h
		detection/color_segmentation.cu     detection/color_segmentation.cuh
		detection/tracker.cpp               detection/tracker.h
		detection/coordinates.cpp           detection/coordinates.h

		# Calibration Classes
		calibration/blob_calibrator.cpp     calibration/blob_calibrator.h
		calibration/color_calibrator.cpp    calibration/color_calibrator.h

		# CUDA Kernels
		calibration/blob_kernels.cu         calibration/blob_kernels.h

		# GUI
		gui/gui.cpp                         gui/gui.h
		gui/widget.cpp                      gui/widget.h
		gui/button.cpp                      gui/button.h
		gui/slider.cpp                      gui/slider.h
		gui/interface_manager.cpp           gui/interface_manager.h
		gui/drawer.cpp                      gui/drawer.h
		calibration/camera_calibrator.cpp
		calibration/camera_calibrator.h
		gui/toggle.cpp
		gui/toggle.h
)

# --- 10. LINKING ---
target_link_libraries(opencv ${OpenCV_LIBS} CUDA::cudart)