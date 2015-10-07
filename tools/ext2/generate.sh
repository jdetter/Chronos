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

# Create a lot of files
mkdir files
cd files
touch file{1..1000}.txt
cd ..

# create /bin/bash
mkdir bin
cd bin
cp /bin/bash .
cd ..

# create a file with contents
mkdir content
cd content
cp /usr/include/curses.h .
cd ..


cd ..
sleep 1
sudo umount ./mnt
rmdir mnt
