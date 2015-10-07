#!/bin/bash

VT_HD_SZ=131072
SECT_SIZE=512
VT_HD_NAME=ext2.fs
PART1_START=2048
MOUNT_START=1048576
USERNAME=$(whoami)

mkdir -p mnt
sudo mount -o loop,offset=$MOUNT_START $VT_HD_NAME -t ext2 mnt
