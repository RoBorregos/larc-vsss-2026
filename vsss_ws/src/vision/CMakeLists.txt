cmake_minimum_required(VERSION 3.22)
project(vsss_vision LANGUAGES C CXX CUDA)

# --- 1. CONFIGURACIÓN CRÍTICA DE COMPILADORES ---
set(CMAKE_CUDA_COMPILER /usr/local/cuda/bin/nvcc)
set(CUDA_TOOLKIT_ROOT_DIR /usr/local/cuda)
set(CMAKE_C_COMPILER gcc-13)
set(CMAKE_CXX_COMPILER g++-13)
set(CMAKE_CUDA_HOST_COMPILER g++-13)

# --- 2. ESTÁNDARES Y ARQUITECTURA ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_ARCHITECTURES 89)

# --- 3. BUSCAR DEPENDENCIAS (ROS 2 + VISIÓN) ---
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(OpenCV REQUIRED)
find_package(CUDAToolkit REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(rosidl_default_generators REQUIRED)

add_definitions(-DDEBUG_MODE)

# --- 4. GENERACIÓN DE INTERFACES (CUSTOM MSGS) ---
# Se define ANTES del ejecutable para que los targets de mensajes existan
rosidl_generate_interfaces(${PROJECT_NAME}
        "vision/ros/msg/EntityState.msg"
        DEPENDENCIES geometry_msgs
)

# --- 5. EJECUTABLE DE VISIÓN ---
set(VISION_DIR vision) # Eliminamos la barra final para consistencia

add_executable(vision_node
        ${VISION_DIR}/main.cpp
        ${VISION_DIR}/include/app_settings.h
        ${VISION_DIR}/include/robot_identities.h
        ${VISION_DIR}/include/json.hpp
        ${VISION_DIR}/lib/image_preprocessing.cpp
        ${VISION_DIR}/lib/blob_detection.cpp
        ${VISION_DIR}/detection/detector.cpp
        ${VISION_DIR}/detection/color_segmentation.cu
        ${VISION_DIR}/detection/tracker.cpp
        ${VISION_DIR}/detection/coordinates.cpp
        ${VISION_DIR}/calibration/blob_calibrator.cpp
        ${VISION_DIR}/calibration/color_calibrator.cpp
        ${VISION_DIR}/calibration/camera_calibrator.cpp
        ${VISION_DIR}/calibration/blob_kernels.cu
        ${VISION_DIR}/gui/gui.cpp
        ${VISION_DIR}/gui/widget.cpp
        ${VISION_DIR}/gui/button.cpp
        ${VISION_DIR}/gui/slider.cpp
        ${VISION_DIR}/gui/toggle.cpp
        ${VISION_DIR}/gui/interface_manager.cpp
        ${VISION_DIR}/gui/drawer.cpp
        ${VISION_DIR}/ros/publisher.cpp
)

# --- 6. DIRECTORIOS DE INCLUSIÓN (Scope por Target) ---
# Usamos target_include_directories para que CLion sepa exactamente qué pertenece a qué
target_include_directories(vision_node PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${VISION_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${VISION_DIR}/lib>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${VISION_DIR}/detection>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${VISION_DIR}/calibration>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${VISION_DIR}/gui>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${VISION_DIR}/field>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${VISION_DIR}/ros>
        # ESTA LÍNEA ES LA MAGIA PARA CLION Y DOCKER:
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/rosidl_generator_cpp>
)

# --- 7. VINCULACIÓN Y DEPENDENCIAS ---
rosidl_get_typesupport_target(cpp_typesupport_target "${PROJECT_NAME}" "rosidl_typesupport_cpp")

target_link_libraries(vision_node
        rclcpp::rclcpp
        cv_bridge::cv_bridge
        ${OpenCV_LIBS}
        CUDA::cudart
        "${cpp_typesupport_target}"
)

# Garantiza que los archivos .hpp existan antes de compilar el código fuente
add_dependencies(vision_node ${PROJECT_NAME}__rosidl_typesupport_cpp)

# --- 8. INSTALACIÓN ---
install(TARGETS vision_node
        DESTINATION lib/${PROJECT_NAME})

ament_package()