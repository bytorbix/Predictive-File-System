    #include "include/disk.h"
    #include "include/fs.h"
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>

void check_result(int status_code, char *name) {
    if (status_code == 1) {
        printf("[DEBUG]: %s worked succesfully", name);
    }
    else if (status_code == 0) {
        printf("[DEBUG]: %s failed", name);
    }
    else if (status_code == -1) {
        printf("[ERROR]: an illegal action occured in %s", name);
    }
}

int main() {
    Disk *disk = NULL;
    FileSystem fs = {0}; 
    
    disk = disk_open("mfs_test_disk.img", 100);
    fs.disk = disk;
    fs_format(disk);
    fs_mount(&fs, disk);
    fs_debug(disk);
    
    ssize_t inode = fs_create(&fs);
    printf("Found Inode available at: %zd\n", inode);
    ssize_t inode1 = fs_create(&fs);
    printf("Found Inode available at: %zd\n", inode1);
    ssize_t inode2 = fs_create(&fs);
    printf("Found Inode available at: %zd\n", inode2);
    ssize_t inode3 = fs_create(&fs);
    printf("Found Inode available at: %zd\n", inode3);
    ssize_t inode4 = fs_create(&fs);
    printf("Found Inode available at: %zd\n", inode4);

}