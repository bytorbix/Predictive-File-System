/* Disk Emulator header file*/

#include <unistd.h>

typedef struct Disk Disk;
struct Disk {
    int fd; /* File descriptor of disk image */
    size_t blocks; /* Number of blocks in disk image*/
    size_t reads; /* Number of reads to disk image*/
    size_t  writes;     /* Number of writes to disk image       */
    bool    mounted;    /* Whether or not disk is mounted       */
};

/* Disk Functions */

Disk *  disk_open(const char *path, size_t blocks);
void    disk_close(Disk *disk);

ssize_t disk_read(Disk *disk, size_t block, char *data);
ssize_t disk_write(Disk *disk, size_t block, char *data);