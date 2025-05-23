FROM ubuntu:22.04

ARG UID=1000
ARG GID=1000

SHELL ["/bin/bash", "-c"]

ENV DEBIAN_FRONTEND=noninteractive
ARG ZSDK_VERSION=0.16.8
ENV ZSDK_VERSION=$ZSDK_VERSION
ARG BSIM_VERSION=v2.3
ENV BSIM_VERSION=$BSIM_VERSION
ARG WGET_ARGS="-q --show-progress --progress=bar:force:noscroll"

RUN apt-get -y update && \
	apt-get -y upgrade && \
	apt-get install -y --no-install-recommends \
	git cmake ninja-build gperf \
	ccache dfu-util device-tree-compiler wget \
	python3-dev python3-pip python3-setuptools python-is-python3 python3-tk python3-wheel \
	xz-utils file make gcc gcc-multilib g++-multilib libsdl2-dev libmagic1 libfftw3-3

# Install gdbserver for enabling GUI debugging in VSCode
# It's ok to make another layer for this IMO
RUN apt-get install --no-install-recommends -y \
	gdb gdbserver

# Initialize system locale
RUN apt-get install -y --no-install-recommends \
	liblocale-gettext-perl locales && \
	locale-gen en_US.UTF-8
ENV LANG=en_US.UTF-8
ENV LANGUAGE=en_US:en
ENV LC_ALL=en_US.UTF-8

# Install python deps
RUN pip3 install -U pip && \
	pip3 install west && \
	pip3 install -r https://raw.githubusercontent.com/zephyrproject-rtos/zephyr/main/scripts/requirements.txt

# But what about second python deps?
# See .github/workflows/compliance.yml *cries in requirements.txt*
RUN pip3 install python-magic lxml junitparser gitlint pylint pykwalify yamllint clang-format unidiff

# Install (minimal) Zephyr SDK
RUN mkdir -p /opt/toolchains && \
	cd /opt/toolchains && \
	wget ${WGET_ARGS} https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v${ZSDK_VERSION}/zephyr-sdk-${ZSDK_VERSION}_linux-${HOSTTYPE}_minimal.tar.xz && \
	tar xf zephyr-sdk-${ZSDK_VERSION}_linux-${HOSTTYPE}_minimal.tar.xz && \
	zephyr-sdk-${ZSDK_VERSION}/setup.sh -t x86_64-zephyr-elf -t arm-zephyr-eabi -h -c && \
	rm zephyr-sdk-${ZSDK_VERSION}_linux-${HOSTTYPE}_minimal.tar.xz

# Create 'user' account
RUN groupadd -g $GID -o user

RUN apt-get install -y --no-install-recommends \
	sudo && \
	useradd -u $UID -m -g user -G plugdev user \
	&& echo 'user ALL = NOPASSWD: ALL' > /etc/sudoers.d/user \
	&& chmod 0440 /etc/sudoers.d/user

# TODO: is this necessary?
RUN usermod -s /usr/bin/bash user

USER user

# Run the Zephyr SDK setup script as 'user' in order to ensure that the
# `Zephyr-sdk` CMake package is located in the package registry under the
# user's home directory.

RUN sudo -E -- bash -c ' \
	/opt/toolchains/zephyr-sdk-${ZSDK_VERSION}/setup.sh -c && \
	chown -R user:user /home/user/.cmake \
	'

# Add a cache of the west workspace to speed-up checkout
COPY west.yml /west-cache.yml
COPY scripts/west-cache.sh /west-cache.sh
RUN /west-cache.sh

USER root

# Install required dependencies
RUN apt-get update && apt-get install -y \
    wget \
    udev \
    libudev1 \
    libusb-1.0-0 \
    build-essential

# Create directory for tools
WORKDIR /opt/tools

# Download and install nRF Command Line Tools
RUN wget https://www.nordicsemi.com/-/media/Software-and-other-downloads/Desktop-software/nRF-command-line-tools/sw/Versions-10-x-x/10-24-2/nrf-command-line-tools_10.24.2_amd64.deb

RUN printf '#!/bin/bash\necho not running udevadm "$@"' > /usr/bin/udevadm && chmod +x /usr/bin/udevadm

RUN dpkg -i nrf-command-line-tools_10.24.2_amd64.deb \
    && apt install -y /opt/nrf-command-line-tools/share/JLink_Linux_V794e_x86_64.deb --fix-broken \
    && rm nrf-command-line-tools_10.24.2_amd64.deb

# Copy JLink udev rules
RUN echo 'ATTRS{idVendor}=="1366", ATTRS{idProduct}=="0101", MODE="0666"\n\
ATTRS{idVendor}=="1366", ATTRS{idProduct}=="0102", MODE="0666"\n\
ATTRS{idVendor}=="1366", ATTRS{idProduct}=="0103", MODE="0666"\n\
ATTRS{idVendor}=="1366", ATTRS{idProduct}=="0104", MODE="0666"' > /etc/udev/rules.d/99-jlink.rules

USER user

# Set up environment
ENV PATH="/opt/tools/JLink_Linux_V*:/opt/nrf-command-line-tools/bin:${PATH}"

ENV ZEPHYR_TOOLCHAIN_VARIANT=zephyr

CMD /usr/bin/bash
