#!/bin/bash

MODULE_NAME="predictfs"
KO_FILE="${MODULE_NAME}.ko"
IMAGE_PATH="/home/user/predictFS/disk.img"
MOUNT_POINT="/tmp/mnt"

sudo umount "$MOUNT_POINT" 2>/dev/null
sudo losetup -j "$IMAGE_PATH" | cut -d: -f1 | xargs -r sudo losetup -d
sudo rmmod "$MODULE_NAME" 2>/dev/null

make || exit 1
sudo insmod "$KO_FILE" || exit 1

mkdir -p "$MOUNT_POINT"
LOOP_DEV=$(sudo losetup -f --show "$IMAGE_PATH")
sudo mount -t "$MODULE_NAME" "$LOOP_DEV" "$MOUNT_POINT" && echo "SUCCESS: Mounted on $MOUNT_POINT"