#!/bin/bash

SECTOR_SIZE=512
START_BLOCK=2048
FS=./ext2/ext2.fs

cd ..
echo "Arguments: -s $START_BLOCK -b $SECTOR_SIZE $FS"
gdb ./bin/ext2.fsck
