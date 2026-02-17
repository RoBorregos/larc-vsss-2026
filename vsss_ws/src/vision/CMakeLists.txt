cmake_minimum_required(VERSION 3.22)
project(vsss_vision LANGUAGES CXX CUDA)

# --- 1. CONFIGURACIÓN CRÍTICA DE COMPILADORES ---
set(CMAKE_CUDA_COMPILER /usr/local/cuda/bin/nvcc)
set(CUDA_TOOLKIT_ROOT_DIR /usr/local/cuda)
set(CMAKE_C_COMPILER gcc-13)
set(CMAKE_CXX_COMPILER g++-13)
set(CMAKE_CUDA_HOST_COMPILER g++-13)

# Política para permitir el módulo FindCUDA antiguo que usa OpenCV internamente
#cmake_policy(SET CMP0146 OLD)

# --- 2. ESTÁNDARES Y ARQUITECTURA ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_ARCHITECTURES 89)

# --- 3. BUSCAR DEPENDENCIAS (ROS 2 + VISIÓN) ---
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(OpenCV REQUIRED)
find_package(CUDAToolkit REQUIRED)

# --- 4. DEFINICIONES Y DIRECTORIOS ---
add_definitions(-DDEBUG_MODE)

# Ajustamos las rutas para que apunten a la carpeta 'vision'
set(VISION_DIR vision/)
include_directories(
        ${VISION_DIR}/include
        ${VISION_DIR}/lib
        ${VISION_DIR}/detection
        ${VISION_DIR}/calibration
        ${VISION_DIR}/gui
        ${VISION_DIR}/field
)

# --- 5. EJECUTABLE DE VISIÓN ---
add_executable(vision_node
        ${VISION_DIR}/main.cpp
        # Base Files
        ${VISION_DIR}/include/app_settings.h
        ${VISION_DIR}/include/robot_identities.h
        ${VISION_DIR}/include/json.hpp

        # Image Processing Module
        ${VISION_DIR}/lib/image_preprocessing.cpp
        ${VISION_DIR}/lib/blob_detection.cpp

        # Detection Classes
        ${VISION_DIR}/detection/detector.cpp
        ${VISION_DIR}/detection/color_segmentation.cu
        ${VISION_DIR}/detection/tracker.cpp
        ${VISION_DIR}/detection/coordinates.cpp

        # Calibration Classes
        ${VISION_DIR}/calibration/blob_calibrator.cpp
        ${VISION_DIR}/calibration/color_calibrator.cpp
        ${VISION_DIR}/calibration/camera_calibrator.cpp

        # CUDA Kernels
        ${VISION_DIR}/calibration/blob_kernels.cu

        # GUI
        ${VISION_DIR}/gui/gui.cpp
        ${VISION_DIR}/gui/widget.cpp
        ${VISION_DIR}/gui/button.cpp
        ${VISION_DIR}/gui/slider.cpp
        ${VISION_DIR}/gui/toggle.cpp
        ${VISION_DIR}/gui/interface_manager.cpp
        ${VISION_DIR}/gui/drawer.cpp
)

# --- 6. VINCULACIÓN (LINKING) ---
target_link_libraries(vision_node
        rclcpp::rclcpp
        cv_bridge::cv_bridge
        ${OpenCV_LIBS}
        CUDA::cudart
)

# --- 7. OTROS PROGRAMAS (Ejemplo: Estrategia) ---
# Si luego creas otro programa en otra carpeta, solo repites el add_executable:
# add_executable(strategy_node src/strategy.cpp)
# target_link_libraries(strategy_node rclcpp::rclcpp)

# --- 8. INSTALACIÓN (Obligatorio para ROS 2) ---
install(TARGETS vision_node
        DESTINATION lib/${PROJECT_NAME})

# Instalar carpetas de recursos si las tienes (ej. configs o modelos)
# install(DIRECTORY config DESTINATION share/${PROJECT_NAME})

ament_package()
