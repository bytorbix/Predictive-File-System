#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "disk.h"
#include "fs.h"
#include "dir.h"

int main() {
    Disk *disk = disk_open("mfs_test_disk.img", 100);
    fs_format(disk);

    FileSystem fs = {0};
    fs_mount(&fs, disk);

    // create a file and a subdir, add both into root
    ssize_t file_a = fs_create(&fs);
    ssize_t subdir = dir_create(&fs);

    fs_write(&fs, file_a, "Hello!", 6, 0);

    dir_add(&fs, 0, "hello.txt", file_a);
    dir_add(&fs, 0, "docs",      subdir);

    // lookup
    ssize_t found = dir_lookup(&fs, 0, "hello.txt");
    printf("lookup hello.txt -> inode %zd (expected %zd): %s\n", found, file_a, found == file_a ? "OK" : "FAIL");

    found = dir_lookup(&fs, 0, "docs");
    printf("lookup docs      -> inode %zd (expected %zd): %s\n", found, subdir, found == subdir ? "OK" : "FAIL");

    found = dir_lookup(&fs, 0, "notexist");
    printf("lookup notexist  -> %zd (expected -1): %s\n", found, found == -1 ? "OK" : "FAIL");

    int r = dir_add(&fs, 0, "hello.txt", file_a); // duplicate
    printf("duplicate add    -> %s (expected FAIL)\n", r == -1 ? "FAIL" : "OK");

    // persistence
    fs_unmount(&fs);
    Disk *disk2 = disk_open("mfs_test_disk.img", 100);
    FileSystem fs2 = {0};
    fs_mount(&fs2, disk2);

    found = dir_lookup(&fs2, 0, "hello.txt");
    printf("after remount    -> inode %zd: %s\n", found, found == file_a ? "OK" : "FAIL");

    fs_unmount(&fs2);
    printf("\nDone!\n");
    return 0;
}
