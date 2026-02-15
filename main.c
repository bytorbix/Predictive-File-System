#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "disk.h"
#include "fs.h"
#include "utils.h"




int main() {
    Disk *disk = disk_open("mfs_test_disk.img", 100); 
    FileSystem fs = {0}; 
    fs.disk = disk;

    if (fs_mount(&fs, disk)) {
        printf("Disk mounted successfully. Loading existing data...\n");
    } 
    else {
        printf("Mount failed (New disk?). Formatting...\n");
        if (fs_format(disk)) {
            if (!fs_mount(&fs, disk)) {
                fprintf(stderr, "Critical Error: Failed to mount even after formatting.\n");
                return 1;
            }
        } else {
            fprintf(stderr, "Critical Error: Format failed.\n");
            return 1;
        }
    }

    disk_debug(disk);
    fs_debug(&fs);

    // Create a file and write some data to it
    ssize_t inode = fs_create(&fs);
    if (inode < 0) {
        fprintf(stderr, "Failed to create file\n");
        fs_unmount(&fs);
        return 1;
    }
    printf("\nCreated file (inode %zd)\n", inode);

    char *msg = "Hello from MFS!";
    ssize_t written = fs_write(&fs, inode, msg, strlen(msg), 0);
    printf("Wrote %zd / %zu bytes\n", written, strlen(msg));

    fs_debug(&fs);

    fs_unmount(&fs);
    return 0;
}