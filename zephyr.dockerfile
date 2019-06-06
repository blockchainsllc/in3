FROM agustinhenze/zephyr-arm
ENV TERM xterm-256color
ENV LANG C.UTF-8
ENV LC_ALL C.UTF-8
ENV ZEPHYR_GCC_VARIANT gccarmemb
ENV GCCARMEMB_TOOLCHAIN_PATH /usr
ENV CROSS_COMPILE /usr/bin/arm-none-eabi-
ENV ZEPHYR_SDK_VERSION 0.9.5
ENV ZEPHYR_VERSION v1.14.0-rc1
COPY ./src src/ 
SHELL ["/bin/bash", "-c"]
RUN apt-get update && eatmydata apt-get install -y curl qemu-system-arm --no-install-recommends && rm -rf /var/lib/apt; \ 
    curl -L https://github.com/Kitware/CMake/releases/download/v3.13.2/cmake-3.13.2-Linux-x86_64.tar.gz | tar -xzf - ; \ 
    export PATH="`pwd`/cmake-3.13.2-Linux-x86_64/bin:$PATH"; \ 
    wget --quiet https://github.com/zephyrproject-rtos/meta-zephyr-sdk/releases/download/${ZEPHYR_SDK_VERSION}/zephyr-sdk-${ZEPHYR_SDK_VERSION}-setup.run; \ 
    chmod +x zephyr-sdk-${ZEPHYR_SDK_VERSION}-setup.run; \ 
    ./zephyr-sdk-${ZEPHYR_SDK_VERSION}-setup.run --quiet -- -d /opt/sdk/zephyr-sdk-${ZEPHYR_SDK_VERSION}; \ 
    rm zephyr-sdk-${ZEPHYR_SDK_VERSION}-setup.run; rm -rf zephyr; \
    git clone --branch ${ZEPHYR_VERSION} https://github.com/zephyrproject-rtos/zephyr.git; \ 
    source zephyr/zephyr-env.sh; \ 
    export ZEPHYR_SDK_INSTALL_DIR="/opt/sdk/zephyr-sdk-${ZEPHYR_SDK_VERSION}"; \ 
    cd src/zephyr; rm -rf build; mkdir build; cd build; \ 
    cmake -DBOARD=nrf52840_pca10056 -DCMAKE_BUILD_TYPE=Release ..; \ 
    make
WORKDIR src/zephyr/build
