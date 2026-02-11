import os

from glob import glob
from setuptools import find_packages, setup

package_name = 'omni_drive_sim'

data_files = []
data_files.append(('share/ament_index/resource_index/packages', ['resource/' + package_name]))
data_files.append(('share/' + package_name, ['package.xml']))
## Launch Files
data_files.append(
    (os.path.join('share', package_name, 'launch'), glob(os.path.join('launch', '*launch.[pxy][yma]*')))
)
## Resource files
data_files.append(('share/' + package_name + '/resource', ['resource/rosmasterx3_multi_robot.urdf']))
data_files.append(('share/' + package_name + '/resource', ['resource/rviz_config.rviz']))
data_files.append(('share/' + package_name + '/resource', ['resource/rviz_camera_config.rviz']))
data_files.append(('share/' + package_name + '/resource', ['resource/slam_toolbox_params.yaml']))
data_files.append(('share/' + package_name + '/resource', ['resource/cartographer_params.lua']))
## World files
data_files.append(('share/' + package_name + '/worlds', ['worlds/Simulacion2.wbt']))


setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(exclude=['test']),
    data_files=data_files,
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='luis',
    maintainer_email='luis@todo.todo',
    description='TODO: Package description',
    license='TODO: License declaration',
    entry_points={
        'console_scripts': [
            "odometry_publisher = omni_drive_sim.odometry_publisher:main"
        ],
    },
)
