#!/bin/bash

#
# Installing face detector on a new server
#
# Tested on the following operating systems:
#
#   - CentOS 6.7
#

APP_WEB_DIR=$1

function command_exists {
	type "$1" &> /dev/null
}

echo "Start installing FaceDetector"

ret=0
for pkg in yum git cmake
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

set -xe


#
# Install prerequisites
#
yum -y install gcc
yum -y install gcc-c++
yum -y install kernel-devel
yum -y install cmake
yum -y install git
yum -y install gtk2-devel
yum -y install pkgconfig.x86_64

# TODO: install libavcodec-dev libavformat-dev libswscale-dev

yum -y install python-devel
yum -y install numpy
yum -y install tbb tbb-devel
yum -y install libjpeg-devel
#yum -y install libpng-devel
yum -y install libtiff-devel
yum -y install libjasper-devel
yum -y install libdc1394-devel
yum -y install libgtk-x11-2.0.so.0


#
# Install OpenCV
#
rm -rf build
mkdir build
cd ./build

git clone https://github.com/Itseez/opencv.git
if [ $? -ne 0 ]; then
	echo "ERR: unable to download OpenCV sources"
	exit 1
fi

cmake -D CMAKE_BUILD_TYPE=RELEASE -D CMAKE_INSTALL_PREFIX=/usr -D WITH_IPP=FALSE opencv
make -j $(nproc) && make install


#
# Build FaceDetector
#
cmake .. && make


#
# Copy files to app web directory
#
if [ -d $APP_WEB_DIR ]; then
	cp -r ../web_app/* ./facecut $APP_WEB_DIR
else
	echo "Invalid application web directory: $APP_WEB_DIR"
fi

echo "Finished installing FaceDetector"

exit 0
