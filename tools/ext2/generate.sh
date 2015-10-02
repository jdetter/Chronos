#!/bin/bash
set -e

VT_HD_SZ=131072
SECT_SIZE=512
VT_HD_NAME=ext2.fs
PART1_START=2048
MOUNT_START=1048576
USERNAME=$(whoami)

dd if=/dev/zero of=$VT_HD_NAME bs=$SECT_SIZE count=$VT_HD_SZ
printf "n\n\n\n\n\nw\n" |  fdisk -b 512 $VT_HD_NAME
echo "yes" | mkfs.ext2 -Eoffset=$MOUNT_START $VT_HD_NAME

mkdir -p mnt
sudo mount -o loop,offset=$MOUNT_START $VT_HD_NAME -t ext2 mnt
sudo chown -R $USERNAME:$USERNAME ./mnt

cd mnt
touch file{1..1000}.txt

mkdir directory
cd directory
cp /bin/bash .
cd ../..

sleep 1
sudo umount ./mnt
rmdir mnt
