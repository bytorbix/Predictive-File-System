
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdbool.h>

// Size of each block
#define BLOCK_SIZE 4096

// Structure of the disk
typedef struct Disk Disk;
struct Disk {
    int     fd;         // File Descriptor (Proccess ID While Program is running)
    size_t  blocks;     // Maximum Capacity of blocks in the disk
    size_t  reads;      // Amount of reading operations (Used for benchmarking)
    size_t  writes;     // Amount of writing operations (Used for benchmarking)
    bool    mounted;    // If disk is mounted or not
};


Disk * disk_open(const char *path, size_t blocks) {


    int fd = open(path, O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
        perror("disk_open: failed to open/create disk image");
        return NULL;
    }

    Disk *disk = (Disk *)calloc(1, sizeof(Disk));
    if (disk == NULL) {
        close(fd); 
        return NULL;
    }

    off_t total_size = (off_t)blocks * BLOCK_SIZE;
    if (ftruncate(fd, total_size) < 0) {
        perror("disk_open: failed to resize disk image");
        close(fd);
        free(disk); 
        return NULL;
    }

    disk->fd      = fd;
    disk->blocks  = blocks;
    disk->mounted = true;

    return disk;
}

void disk_close(Disk *disk) {
    if (disk == NULL) {
        return;
    }
    if (disk->fd >= 0) {
        if (close(disk->fd) < 0) {
            perror("disk_close: Error closing the disk");
        }
    }
    disk->mounted = false;
    disk->fd = -1; 
    free(disk);
}

ssize_t disk_write(Disk *disk, size_t block, char *data) {
    if (disk == NULL) {
        perror("disk_write: disk is invalid");
        return -1;
    }
    if (disk->mounted == false) {
        perror("disk_write: disk is not mounted");
        return -1;
    }
    if (block >= disk->blocks) {
        perror("disk_write: block size is invalid");
        return -1;
    }

    off_t offset = (off_t)block * BLOCK_SIZE;
    if (lseek(disk->fd, offset, SEEK_SET) < 0) {
        perror("disk_write: lseek has failed");
        return -1;
    }

    ssize_t written_bytes = write(disk->fd, data, BLOCK_SIZE);
    if (written_bytes != BLOCK_SIZE) {
        if (written_bytes < 0) {
            perror("disk_write: write system call failed");
        } else {
            fprintf(stderr, "disk_write: short write (%zd bytes written instead of %d)\n", written_bytes, BLOCK_SIZE);
        }
        return -1;
    }
    disk->writes++;
    return written_bytes;
}

ssize_t disk_read(Disk *disk, size_t block, char *data) {
    if (disk == NULL) {
        perror("disk_read: disk is invalid (NULL pointer)");
        return -1;
    }
    if (!disk->mounted) {
        perror("disk_write: disk is not mounted");
        return -1;
    }
    if (block >= disk->blocks) {
        fprintf(stderr, "disk_read: block number %zu out of bounds (max %zu)\n", 
                block, disk->blocks - 1);
        return -1;
    }
    off_t offset = (off_t)block * BLOCK_SIZE;

    if (lseek(disk->fd, offset, SEEK_SET) < 0) {
        perror("disk_read: lseek has failed");
        return -1;
    }


    ssize_t bytes_read = read(disk->fd, data, BLOCK_SIZE);
    if (bytes_read != BLOCK_SIZE) {
        if (bytes_read < 0) {
            perror("disk_read: system read call failed");
        } else if (bytes_read == 0) {
            fprintf(stderr, "disk_read: read failed, unexpectedly hit End-of-File (EOF)\n");
        } else { 
            fprintf(stderr, "disk_read: short read (%zd bytes read instead of %d)\n", 
                    bytes_read, BLOCK_SIZE);
        }
        return -1; 
    }

    disk->reads++;
    return bytes_read; 
}
