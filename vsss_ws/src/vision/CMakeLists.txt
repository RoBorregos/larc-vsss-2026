cmake_minimum_required(VERSION 3.22)
project(vsss_vision LANGUAGES C CXX CUDA)

set(CMAKE_CUDA_COMPILER /usr/local/cuda/bin/nvcc)
set(CUDA_TOOLKIT_ROOT_DIR /usr/local/cuda)
set(CMAKE_C_COMPILER gcc-13)
set(CMAKE_CXX_COMPILER g++-13)
set(CMAKE_CUDA_HOST_COMPILER g++-13)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_ARCHITECTURES 89)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(OpenCV REQUIRED)
find_package(CUDAToolkit REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(visualization_msgs REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(cv_bridge REQUIRED)

add_definitions(-DDEBUG_MODE)

rosidl_generate_interfaces(${PROJECT_NAME}
        "vision/ros/msg/EntityState.msg"
        DEPENDENCIES geometry_msgs
)

set(VISION_DIR vision)

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
        ${VISION_DIR}/ros/ros_handler.cpp
)

target_include_directories(vision_node PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${VISION_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${VISION_DIR}/lib>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${VISION_DIR}/detection>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${VISION_DIR}/calibration>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${VISION_DIR}/gui>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${VISION_DIR}/field>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${VISION_DIR}/ros>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/rosidl_generator_cpp>
)

rosidl_get_typesupport_target(cpp_typesupport_target "${PROJECT_NAME}" "rosidl_typesupport_cpp")

target_link_libraries(vision_node
        rclcpp::rclcpp
        cv_bridge::cv_bridge
        tf2::tf2
        tf2_ros::tf2_ros
        visualization_msgs::visualization_msgs__rosidl_typesupport_cpp
        ${OpenCV_LIBS}
        CUDA::cudart
        "${cpp_typesupport_target}"
)

add_dependencies(vision_node ${PROJECT_NAME}__rosidl_typesupport_cpp)

install(TARGETS vision_node
        DESTINATION lib/${PROJECT_NAME})

ament_package()