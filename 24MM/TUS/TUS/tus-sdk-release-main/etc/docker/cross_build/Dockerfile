#TMC CONFIDENTIAL
#$TUSLibId$
#Copyright (C) 2022 TOYOTA MOTOR CORPORATION
#All Rights Reserved.

FROM ubuntu:20.04
ENV DEBIAN_FRONTEND=noninteractive

# based on the Essentials packages listed in
#  https://docs.yoctoproject.org/ref-manual/system-requirements.html#ubuntu-and-debian
# and locales (to have en_US.UTF-8), and vim (for debugging)
RUN apt-get update && apt-get install -y \
  build-essential \
  ca-certificates \
  chrpath \
  cpio \
  debianutils \
  diffstat \
  gawk \
  gcc-multilib \
  git-core \
  iputils-ping \
  libegl1-mesa \
  liblz4-tool \
  libsdl1.2-dev \
  llvm \
  locales \
  mesa-common-dev\
  pylint3 \
  python3 \
  python3-git \
  python3-jinja2 \
  python3-pexpect \
  python3-pip \
  python3-subunit \
  socat \
  squashfs-tools \
  texinfo \
  unzip \
  vim \
  wget \
  xterm \
  xz-utils \
  zstd \
  && apt-get clean

RUN dpkg-reconfigure locales \
  && locale-gen en_US.UTF-8 \
  && update-locale LC_ALL=en_US.UTF-8 LANG=en_US.UTF-8

ENV LC_ALL   en_US.UTF-8
ENV LANG     en_US.UTF-8
ENV LANGUAGE en_US.UTF-8

# retrieve local.conf to build the Yocto SDK
RUN mkdir /cross_build
COPY ./etc/docker/cross_build/local.conf /cross_build/

# Refer to https://wiki.yoctoproject.org/wiki/Releases for Yocto Codename
RUN git clone --depth 1 -b yocto-4.0.1 git://git.yoctoproject.org/poky.git
RUN git clone --depth 1 -b kirkstone git://git.openembedded.org/meta-openembedded.git
RUN git clone --depth 1 -b kirkstone git://git.yoctoproject.org/git/meta-raspberrypi


