FROM nvcr.io/nvidia/isaac-sim:5.1.0

ENV ISAAC_SIM_PATH=/isaac-sim
ENV ISAACLAB_PATH=/workspace/IsaacLab
ENV DEBIAN_FRONTEND=noninteractive

USER root

RUN apt-get update && apt-get install -y --no-install-recommends \
    cmake \
    git \
    wget \
    nano \
    libglib2.0-0 \
    python3-pip \
    libegl1-mesa-dev \
    mesa-common-dev \
    libgles2-mesa-dev \
    libx11-dev \
    build-essential \
    cmake \
    && apt-get clean && rm -rf /var/lib/apt/lists/*
WORKDIR /workspace

RUN git clone https://github.com/isaac-sim/IsaacLab.git ${ISAACLAB_PATH}

WORKDIR ${ISAACLAB_PATH}

RUN ln -s /isaac-sim _isaac_sim && \
    ln -s /usr/bin/python3 /usr/bin/python && \
    TERM=linux ./isaaclab.sh --install

RUN ${ISAAC_SIM_PATH}/python.sh -m pip install \
    stable-baselines3 \
    tensorboard \
    moviepy \
    torchvision

WORKDIR /workspace/project
