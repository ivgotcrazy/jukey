#! /bin/bash

# Accept two command line parameters
if [ $# -ne 1 ]
then
	echo "usage: build-all.sh jukey-root-dir"
	exit 1
fi

# The fist parameter must be a directory
if [ ! -d $1 ]
then
	echo "$1 is not a directory!"
	exit 1
fi

# Input directory
ROOT_DIR=$1

# Trim suffix '/'
if [ ${ROOT_DIR: -1} = '/' ]
then
	ROOT_DIR=${ROOT_DIR%?}
fi

# Make directory
BUILD_DIR=$ROOT_DIR/build/linux

# Build modules
BUILD_MODULES=(
	common/util
	common/property
	common/protocol
	common/amqp-client
	core/base-frame
	core/net-frame
	core/transport
	service/service-box
	service/group-service
	service/terminal-service
	service/user-service
	service/stream-service
	service/transport-service
	service/proxy-service
)

# Build all modules
for module in ${BUILD_MODULES[@]}
do
	mkdir -p $BUILD_DIR/$module/build
	cd $BUILD_DIR/$module/build
	cmake3 ..
	make -j2
	cd -
done
