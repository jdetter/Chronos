#!/bin/bash

SECTOR_SIZE=512
START_BLOCK=2048
FS=./ext2/ext2.fs

cd ..
./bin/ext2.fsck -s $START_BLOCK -b $SECTOR_SIZE $FS
