#!/bin/bash

set -e
set -x

if [[ "$(uname -s)" == 'Darwin' ]]; then
    brew update || brew update
    brew outdated pyenv || brew upgrade pyenv
    brew install pyenv-virtualenv
    brew install cmake || true

    if which pyenv > /dev/null; then
        eval "$(pyenv init -)"
    fi

    pyenv install 2.7.10
    pyenv virtualenv 2.7.10 conan
    pyenv rehash
    pyenv activate conan
fi

if [[ "$(uname -s)" == 'Linux' ]]; then
    sudo apt-get update -qq
    sudo apt-get install -y python-software-properties

    sudo apt-add-repository -y ppa:lttng/ppa
    sudo apt-get update -qq
    sudo apt-get install -y lttng-tools lttng-modules-dkms babeltrace liblttng-ust-dev

    # opengl dependencies
    sudo apt-get install -y xorg-dev libglu1-mesa-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev
    # opencl dependencies
    sudo apt-get install -y ocl-icd-libopencl1 opencl-headers
fi

pip install conan --upgrade
pip install conan_package_tools --upgrade

conan user
