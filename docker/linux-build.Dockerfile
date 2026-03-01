FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Etc/UTC

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    clang \
    autoconf \
    automake \
    libtool \
    ninja-build \
    git \
    curl \
    zip \
    unzip \
    tar \
    pkg-config \
    ca-certificates \
    python3 \
    python3-pip \
    python3-venv \
 && rm -rf /var/lib/apt/lists/*

# Ubuntu 24.04 ships CMake 3.28.x; this project requires >= 3.30.
RUN python3 -m pip install --break-system-packages --upgrade "cmake>=3.30"

WORKDIR /workspace

# Default command: GCC Release build on Linux with vcpkg dynamic triplet.
CMD ["bash", "-lc", "export PATH=/usr/local/bin:/root/.local/bin:$PATH && cmake --version && cmake -S . -B out-docker-gcc -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DVCPKG_TARGET_TRIPLET=x64-linux-dynamic && cmake --build out-docker-gcc --target cast main front -j$(nproc)"]
