#include "include/disk.h"
#include "include/fs.h"
#include "fs.h" 
#include "utils.h" 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>




void fs_debug_bitmap(FileSystem *fs) 
{
    // --- Validation Checks ---
    if (fs == NULL || fs->disk == NULL || fs->meta_data == NULL) {
        perror("fs_debug_bitmap: Error, FileSystem not initialized or mounted");
        return;
    }
    if (fs->bitmap == NULL || *(fs->bitmap) == NULL) {
        fprintf(stderr, "fs_debug_bitmap: Error, in-memory bitmap is NULL (not allocated or mounted)\n");
        return;
    }

    // Correctly get the pointer to the array of words (uint32_t *)
    // from the pointer-to-pointer (uint32_t **).
    uint32_t *bitmap_array = fs->bitmap;

    uint32_t total_blocks = fs->meta_data->blocks;
    uint32_t count = 0;

    printf("\n--- FileSystem Bitmap Debug ---\n");
    printf("Total disk blocks: %u\n", total_blocks);
    
    // Check allocation status for every block
    for (uint32_t i = 0; i < total_blocks; i++) {
        // Pass the array pointer to the get_bit function
        if (get_bit(bitmap_array, i)) { 
            // The block is marked as allocated
            if (count == 0) {
                printf("Allocated blocks:");
            }
            printf(" %u", i);
            count++;
        }
    }

    if (count > 0) {
        printf("\nTotal allocated blocks: %u\n", count);
    } else {
        printf("No blocks are currently marked as allocated.\n");
    }
    printf("-------------------------------\n");
}

int main() {
    Disk *disk = NULL;
    FileSystem fs = {0}; 
    
    disk = disk_open("mfs_test_disk.img", 100); 
    fs.disk = disk;
    fs_format(disk);
    fs_mount(&fs, disk);
    fs_debug(disk);
    fs_debug_bitmap(&fs);
    
}