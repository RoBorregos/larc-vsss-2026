source /opt/ros/humble/setup.bash && \
cd vsss_ws && \
colcon build --packages-select vsss_vision strategy vsss_bringup communication && \
source install/setup.bash
