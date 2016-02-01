#!/bin/bash

#
# Installing face detector on a new server
#
# Tested on the following operating systems:
#
#   - CentOS 6.7
#
#
#  Written by:
#
#       Ilya Nelkenbaum <ilya@nelkenbaum.com>
#
#

APP_WEB_DIR=$1

function show_usage {
	echo "Usage:"
	echo "    ./install <web app directory>"
}

function command_exists {
	type "$1" &> /dev/null
}

function check_pkgs {
	ret=0

	for pkg in $@
	do
		echo  -n "Checking for $pkg package ... "
		if ! command_exists $pkg ; then
			echo "failure"
			ret=1
			continue
		fi
		echo "success"
	done

	if [ $ret -ne 0 ]; then
		exit 1
	fi
}

function install_prereqs {
	pkgs=("gcc" \
	      "gcc-c++" \
	      "kernel-devel" \
	      "cmake" \
	      "git" \
	      "gtk2-devel" \
	      "pkgconfig.x86_64" \
	      "python-devel" \
	      "numpy" \
	      "tbb" "tbb-devel" \
	      "libjpeg-devel" \
	      "libtiff-devel" \
	      "libjasper-devel" \
	      "libdc1394-devel" \
	      "libgtk-x11-2.0.so.0")

# TODO: install libavcodec-dev libavformat-dev libswscale-dev
# TODO: install libpng-devel

	for pkg in ${pkgs[@]}
	do
		yum -y install $pkg
	done
}

function install_opencv {
	git clone https://github.com/Itseez/opencv.git
	if [ $? -ne 0 ]; then
		echo "ERR: unable to download OpenCV sources"
		exit 1
	fi

	cmake -D CMAKE_BUILD_TYPE=RELEASE \
	      -D CMAKE_INSTALL_PREFIX=/usr \
	      -D WITH_IPP=FALSE \
	      opencv
	make -j $(nproc) && make install
}

if [ -z $APP_WEB_DIR ]; then
	show_usage
	exit 1
fi

echo "Start installing FaceDetector"

check_pkgs "yum" "git" "cmake"

set -xe

#
# Install prerequisites
#
install_prereqs 

rm -rf build
mkdir build
cd ./build


#
# Install OpenCV
#
install_opencv


#
# Build FaceDetector
#
cmake .. && make


#
# Copy files to app web directory
#
if [ ! -d $APP_WEB_DIR ]; then
	mkdir -p $APP_WEB_DIR
fi

cp -r ../web_app/* ./facecut $APP_WEB_DIR
chown -R admin $APP_WEB_DIR/*

echo "Finished installing FaceDetector"

exit 0
