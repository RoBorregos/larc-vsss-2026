cmake_minimum_required(VERSION 3.8)
project(vsss_strategy)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 1. Encontrar dependencias
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(std_msgs REQUIRED)

# 2. Crear el ejecutable
# Asegúrate de que tu archivo se llame vsss_player.cpp y esté dentro de la carpeta src/
add_executable(vsss_player_node vsss_player.cpp)

# 3. Vincular librerías
ament_target_dependencies(vsss_player_node
        rclcpp
        geometry_msgs
        std_msgs
)

# 4. INSTALAR (Esta es la parte que suele faltar)
install(TARGETS
        vsss_player_node
        DESTINATION lib/${PROJECT_NAME}
)

ament_package()