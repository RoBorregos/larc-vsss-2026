FROM nvidia/cuda:12.8.0-cudnn-devel-ubuntu22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    locales \
    pkg-config \
    libgtk-3-dev \
    libv4l-dev \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    libjpeg-dev \
    libpng-dev \
    libtiff-dev \
    python3-dev \
    python3-numpy \
    git \
    && locale-gen en_US.UTF-8

ENV LANG=en_US.UTF-8

RUN apt-get update && apt-get install -y software-properties-common curl gnupg2 lsb-release \
    && add-apt-repository universe \
    && curl -sSL https://raw.githubusercontent.com/ros/rosdistro/master/ros.key -o /usr/share/keyrings/ros-archive-keyring.gpg \
    && echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/ros-archive-keyring.gpg] http://packages.ros.org/ros2/ubuntu $(lsb_release -cs) main" | tee /etc/apt/sources.list.d/ros2.list > /dev/null \
    && apt-get update && apt-get install -y \
    ros-humble-desktop \
    ros-dev-tools \
    python3-colcon-common-extensions \
    build-essential \
    cmake \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /opt
RUN git clone --branch 4.13.0 --depth 1 https://github.com/opencv/opencv.git && \
    git clone --branch 4.13.0 --depth 1 https://github.com/opencv/opencv_contrib.git

WORKDIR /opt/opencv/build
RUN cmake -D CMAKE_BUILD_TYPE=RELEASE \
    -D CMAKE_INSTALL_PREFIX=/usr/local \
    -D INSTALL_C_EXAMPLES=OFF \
    -D INSTALL_PYTHON_EXAMPLES=OFF \
    -D WITH_CUDA=ON \
    -D WITH_CUDNN=ON \
    -D WITH_CUBLAS=ON \
    -D WITH_TBB=ON \
    -D OPENCV_DNN_CUDA=ON \
    -D OPENCV_ENABLE_NONFREE=ON \
    -D CUDA_ARCH_BIN=12.0 \
    -D OPENCV_EXTRA_MODULES_PATH=/opt/opencv_contrib/modules \
    -D BUILD_EXAMPLES=OFF \
    -D HAVE_opencv_python3=ON \
    -D ENABLE_FAST_MATH=ON \
    -D CUDA_FAST_MATH=ON \
    -D WITH_V4L=ON \
    -D WITH_QT=OFF \
    -D WITH_GTK=ON \
    -D WITH_OPENGL=ON \
    .. && \
    make -j$(nproc) && \
    make install && \
    ldconfig

RUN apt-get update && \
    apt-get install -y software-properties-common && \
    add-apt-repository -y ppa:ubuntu-toolchain-r/test && \
    apt-get update && \
    apt-get install -y g++-13

RUN apt-get update && apt-get install -y \
    python3-pip \
    && apt-get remove -y cmake \
    && pip3 install cmake --upgrade

COPY . /ros2_ws/larc-vsss-2026-vision/

RUN hash -r
RUN ln -s /usr/local/bin/cmake /usr/bin/cmake

WORKDIR /ros2_ws
