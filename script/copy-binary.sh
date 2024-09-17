#! /bin/bash

# Accept two command line parameters
if [ $# -ne 2 ]
then
	echo "usage: copy-binary.sh jukey-root-dir target-dir"
	exit 1
fi

# The fist parameter must be a directory
if [ ! -d $1 ]
then
	echo "$1 is not a directory!"
	exit 1
fi

# The second parameter must be a direcotry
if [ ! -d $2 ]
then
	echo "$2 is not a directory!"
	exit 1
fi

# Input directories
ROOT_DIR=$1
DEST_DIR=$2

# Trim suffix '/'
if [ ${ROOT_DIR: -1} = '/' ]
then
	ROOT_DIR=${ROOT_DIR%?}
fi

# Make directory
OUTPUT_DIR=$ROOT_DIR/output

# Binary modules
BIN_MODULES=(
        common/property
        common/amqp-client
        common/timer
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

# Copy binaries
for module in ${BIN_MODULES[@]}
do
	cp -f $OUTPUT_DIR/$module/linux/release/* $DEST_DIR 
done
