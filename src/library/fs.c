#include "fs.h"
#include "disk.h"
#include "utils.h"
#include <stdio.h> 
#include <string.h> 
#include <stdlib.h> 
#include <math.h>

/* File System Functions Definitions */

void fs_debug(Disk *disk) {
    // Vaidation Checks
    if (disk == NULL) {
        perror("fs_debug: Error disk is invalid");
        return;
    }
    if (!disk->mounted) { 
        fprintf(stderr, "fs_debug: Error disk is not mounted\n");
        return;
    }

    Block block_buffer;
    SuperBlock superblock;

    // Attempt to copy the content of super block into block_buffer
    if (disk_read(disk, 0, block_buffer.data) < 0) {
        perror("fs_debug: Failed to read SuperBlock from disk");
        return;
    }

    superblock = block_buffer.super; // Copy the initialized superblock into the union

    // super block validation check
    if (superblock.magic_number != MAGIC_NUMBER) {
        fprintf(stderr, 
                "fs_debug: Error Disk magic number (0x%x) is invalid. Expected (0x%x).\n"
                "The disk is either unformatted or corrupted.\n",
                superblock.magic_number, MAGIC_NUMBER);
        return;
    }
    
    // Printing The Super block values (Using %u for consistency)
    printf("SuperBlock:\n");
    printf("\tmagic number is valid\n");
    printf("\t%u blocks\n", superblock.blocks);
    printf("\t%u inode blocks\n", superblock.inode_blocks);
    printf("\t%u inodes\n", superblock.inodes); 

    // --- Scanning Inode Table ---
    Block inode_buffer;
    for (uint32_t i = 1; i <= superblock.inode_blocks; i++) // Iterate over all Inode Blocks (1 to N)
    {
        // Read the current Inode Block
        if (disk_read(disk, i, inode_buffer.data) < 0) {
            perror("fs_debug: Failed to read Inode Block from disk");
            return;
        }

        // Iterate through all Inodes within the current block (0 to 127)
        for (uint32_t j = 0; j < INODES_PER_BLOCK; j++) {
            Inode current_node = inode_buffer.inodes[j];
            
            if (current_node.valid) {   
                // Calculate and print the global inode index
                uint32_t inode_index = (i - 1) * INODES_PER_BLOCK + j; 
                printf("Inode %u:\n", inode_index);
                printf("\tsize: %u bytes\n", current_node.size);
                
                // Scan and print Direct Pointers
                printf("\tdirect blocks:");
                int count = 0;
                for (uint32_t k = 0; k < POINTERS_PER_INODE; k++) {
                    uint32_t block_num = current_node.direct[k];
                    if (block_num != 0) {
                        printf(" %u", block_num);
                        count++;
                    }
                }
                printf(" (%d total)\n", count); 

                // Scan and print Indirect Pointers
                uint32_t indirect_block_num = current_node.indirect;
                if (indirect_block_num != 0) {
                    printf("\tindirect block: %u\n", indirect_block_num);

                    Block indirect_buffer;
                    // Attempt to read the indirect block from the disk
                    if (disk_read(disk, indirect_block_num, indirect_buffer.data) < 0) {
                        perror("fs_debug: Failed to read indirect block");
                    }
                    else {
                        // read the pointers array inside the indirect block
                        printf("\tindirect pointers:");
                        int indirect_count = 0;

                        for (uint32_t k = 0; k < POINTERS_PER_BLOCK; k++) {
                            uint32_t data_block_num = indirect_buffer.pointers[k];
                            if (data_block_num != 0) {
                                printf(" %u", data_block_num);
                                indirect_count++;
                            }
                        }
                        printf(" (%d total)\n", indirect_count);
                    }
                }
            }
        }
    }
}

bool fs_format(Disk *disk) 
{
    if (disk == NULL) {
        perror("fs_format: Error disk is invalid");
        return false;
    }
    if (disk->mounted) { 
        fprintf(stderr, "fs_format: Error disk is mounted, aborting to prevent data loss\n");
        return false;
    }

    // Initializing the super block
    SuperBlock superblock; 
    superblock.magic_number = MAGIC_NUMBER;
    superblock.blocks = (uint32_t)disk->blocks;

    // Inodes
    double percent_blocks = (double)superblock.blocks * 0.10;   
    superblock.inode_blocks = (uint32_t)ceil(percent_blocks);
    superblock.inodes = superblock.inode_blocks * INODES_PER_BLOCK;


    
    // Capacity check
    if (1 + superblock.inode_blocks > superblock.blocks+1) {
        fprintf(stderr, "fs_format: Error metadata blocks amount (%u) exceeds disk capacity (%u)\n", 
                1 + superblock.inode_blocks, superblock.blocks);
        return false;
    }

    // Initializing The Block Buffer
    Block block_buffer;
    for (int i = 0; i < BLOCK_SIZE; i++) {
        block_buffer.data[i] = 0;
    }
    block_buffer.super = superblock; // Copy the initialized superblock into the union
    

    // attempt to write the metadata into the magic block
    if (disk_write(disk, 0, block_buffer.data) < 0) {
        perror("fs_format: Failed to write SuperBlock to disk");
        return false;
    }
    

    memset(block_buffer.data, 0, BLOCK_SIZE);

    // Clean the inode table
    for (uint32_t i = 1; i <= superblock.inode_blocks+1; i++) {
        if (disk_write(disk, i, block_buffer.data) < 0) {
            perror("fs_format: Failed to clear inode table blocks");
            return false;
        }
    }
    // success
    return true;
}

bool fs_mount(FileSystem *fs, Disk *disk) {
    if (fs == NULL || disk == NULL) {
        perror("fs_mount: Error fs or disk is invalid (NULL)"); 
        return false;
    }
    if (disk->mounted) { 
        fprintf(stderr, "fs_mount: Error disk is already mounted, cannot mount it\n");
        return false;
    }

    Block block_buffer;
    if (disk_read(disk, 0, block_buffer.data)< 0) {
        perror("fs_mount: Failed to read super block from disk");
        return false;
    }

    SuperBlock superblock = block_buffer.super;
    if (superblock.magic_number != MAGIC_NUMBER) {
        fprintf(stderr, 
                "fs_mount: Error Disk magic number (0x%x) is invalid. Expected (0x%x).\n"
                "The disk is either unformatted or corrupted.\n",
                superblock.magic_number, MAGIC_NUMBER);
        return false;
    }
    if (superblock.blocks != disk->blocks) {
        fprintf(stderr, "fs_mount: Error Super block amount of blocks (%u) mismatch the disk capacity (%zu), aborting...\n",
                superblock.blocks, disk->blocks);
        return false;
    }

    // Allocate memory for the SuperBlock metadata and copy it
    fs->meta_data = (SuperBlock *)malloc(sizeof(SuperBlock));
    if (fs->meta_data == NULL) {
        perror("fs_mount: Failed to allocate memory for SuperBlock metadata");
        return false;
    }

    *(fs->meta_data) = superblock; // Copy the structure contents
    fs->disk = disk;

    uint32_t total_inodes = fs->meta_data->inodes;
    uint32_t total_blocks = fs->meta_data->blocks;
    // Data Bitmap
    ssize_t bitmap_block = fs->meta_data->inode_blocks + 1; // Bitmap always sits right after the inode table
    uint32_t bitmap_words = (total_blocks + BITS_PER_WORD -1)  / BITS_PER_WORD;
    uint32_t *bitmap = calloc(bitmap_words, sizeof(uint32_t));
    if (bitmap == NULL) {
        perror("fs_mount: Failed to allocate memory for bitmap array");
        free(fs->meta_data);
        return false;
    }
    fs->bitmap = bitmap;


    // Inodes Bitmap
    // ibitmap in it's initial form iterates through all the inodes and read if valid or not
    uint32_t ibitmap_words = (total_inodes + BITS_PER_WORD - 1) / BITS_PER_WORD;
    uint32_t *ibitmap = calloc(ibitmap_words, sizeof(uint32_t));
    if (ibitmap == NULL) {
        perror("fs_mount: Failed to allocate memory for ibitmap array");
        free(fs->meta_data);
        return false;
    }
    fs->ibitmap = ibitmap;

    set_bit(bitmap, 0, 1); // Mark the superblock as allocated
    set_bit(bitmap, fs->meta_data->inode_blocks+1, 1);
    
    uint32_t inode_blocks_end = fs->meta_data->inode_blocks;
    
    // Scanning The Inode Tables
    for (uint32_t i = 1; i <= inode_blocks_end; i++) {

        // Mark Inode blocks (Blocks 1 To N) as allocated for bitmap
        set_bit(bitmap, i, 1);

        Block inode_buffer;
        // Attempt to copy the inode data on the disk into inode_buffer
        if (disk_read(fs->disk, i, inode_buffer.data) < 0)
        {
            perror("fs_mount: failed to read Inode table");
            free(fs->meta_data);
            free(fs->bitmap);
            free(fs->ibitmap);
            return false;
        }
        // Iterate through all the inodes inside the block
        for (uint32_t j = 0; j < INODES_PER_BLOCK; j++)
        {
            // Check if inode is valid
            if (inode_buffer.inodes[j].valid == true )
                {
                // Load Inodes validation to the ibitmap
                uint32_t inode_idx = ((i-1) * INODES_PER_BLOCK) + j;
                if (inode_idx >= total_inodes) break;
                if (inode_buffer.inodes[j].valid == 1) {
                    set_bit(ibitmap, inode_idx, 1);
                }

                for (uint32_t k = 0; k < POINTERS_PER_INODE; k++)
                {
                    uint32_t block_num = inode_buffer.inodes[j].direct[k];
                    // Check if pointer is non-zero AND within total disk bounds
                    if (block_num != 0 && block_num < fs->meta_data->blocks) { 
                        set_bit(bitmap, block_num, 1); // Mark as Allocated
                    }
                }
                uint32_t indirect_block_num = inode_buffer.inodes[j].indirect;
                uint32_t total_blocks = fs->meta_data->blocks;
                if (indirect_block_num != 0 && indirect_block_num < total_blocks) {
                    set_bit(bitmap, indirect_block_num, 1);

                    Block indirect_buffer;
                    // Attempt to copy the indirect pointer into the indirect_buffer (basically the address)
                    if (disk_read(fs->disk, indirect_block_num, indirect_buffer.data) < 0) {
                        // Handle the read error and CLEAN UP ALL allocated memory
                        perror("fs_mount: Failed to read indirect block during scan");
                        free(fs->meta_data);
                        free(fs->bitmap);
                        free(fs->ibitmap);
                        return false;
                    }

                    for (uint32_t k = 0; k < POINTERS_PER_BLOCK; k++) {
                            uint32_t data_block_num = indirect_buffer.pointers[k];

                            // Check if the pointer is non-zero AND within total disk bounds
                            if (data_block_num != 0 && data_block_num < total_blocks) {
                                set_bit(bitmap, data_block_num, 1); // Mark the data block as allocated
                            }
                    }
                }
                
            }
        }
        
    }
    disk->mounted=true;
    return true;
}

void fs_unmount(FileSystem *fs) {
    // Error Checks
    if (fs == NULL) {
        perror("fs_unmount: Error file system pointer is NULL");
        return;
    }
    
    // Check disk pointer 
    if (fs->disk == NULL) {
        perror("fs_unmount: Warning, disk pointer is NULL but continuing cleanup");
    } else if (!fs->disk->mounted) { 
        fprintf(stderr, "fs_unmount: Warning disk is already unmounted\n");
        // Continue cleanup in case memory was still allocated
    }

    // Memory Cleanup 
    if (fs->meta_data != NULL) {
        free(fs->meta_data);
        fs->meta_data = NULL;
    }
    if (fs->bitmap != NULL) {
        free(fs->bitmap);
        fs->bitmap = NULL;
    }

    if (fs->disk != NULL) {
        disk_close(fs->disk);
    }
}


/**
 * @brief Attempts to find and allocate a contiguous run of blocks using the First Fit algorithm.
 *
 * @param fs Pointer to the FileSystem structure.
 * @param blocks_to_reserve The number of contiguous blocks requested.
 * @return size_t* A pointer to an array of allocated block numbers (indices) on success.
 * The array must be freed by the caller. Returns NULL on failure (disk full or fragmentation).
 */
size_t* fs_allocate(FileSystem *fs, size_t blocks_to_reserve) {
    // Initial Validation Checks
    if (fs == NULL || fs->meta_data == NULL || fs->bitmap == NULL || disk == NULL) { 
        perror("fs_allocate: Error fs, metadata, bitmap, or disk is invalid (NULL)"); 
        return NULL;
    }

    if (!(disk->mounted()) { 
        fprintf(stderr, "fs_allocate: Error disk is not mounted, cannot access the disk\n");
        return NULL;
    }

    if (blocks_to_reserve == 0) {
        return NULL; 
    }

    // init of the bitmap
    uint32_t *bitmap = fs->bitmap;
    uint32_t total_blocks = fs->meta_data->blocks;
    size_t meta_blocks = 3 + (fs->meta_data->inode_blocks); 

    // Declaring the allocation blocks array
    size_t *allocated_array = calloc(blocks_to_reserve, sizeof(size_t)); 
    if (allocated_array == NULL) {
        perror("fs_allocate: Failed to allocate memory for return array");
        return NULL; 
    }
    
    // temp_count: a temporary count to achieve the a row of blocks in the amount desired
    // start_block_index: we mark the first block index of the row that fits the desired blocks_to_reserve
    size_t temp_count = 0;
    size_t start_block_index = 0;

    // Iteration of the whole blocks on the disk
    for (size_t i = meta_blocks; i < total_blocks; i++)
    {
        // Checking if a block is not allocated (free)
        if (!(get_bit(bitmap, i))) 
        {
            // Marking the first block index
            if (temp_count == 0) {
                start_block_index = i; 
            }
            temp_count++; 

            // Row Found, Proceeding with allocation
            if (temp_count == blocks_to_reserve) {
                // Inner loop for saving the results on the allocation array
                for (size_t j = 0; j < blocks_to_reserve; j++)
                {
                    // Marking the index block to iterate inside the row
                    size_t block_index = start_block_index + j;
                    
                    // Attempt to allocate the block in the bitmap (set the bit to 1)
                    if (!set_bit(bitmap, block_index, 1)) {
                        // Allocation failed, Rolling back and cleaning the changes on the bitmap
                        fprintf(stderr, "fs_allocate: Error setting bit for block %zu. Rolling back.\n", block_index);
                        
                        for (size_t k = 0; k < j; k++) {
                            set_bit(bitmap, start_block_index + k, 0); 
                        }

                        free(allocated_array);
                        return NULL;
                    }
                    allocated_array[j] = block_index;
                }
                // Returning the allocated array
                return allocated_array;
            }
        }
        // Block is allocated
        else { 
            temp_count = 0; // Reset the counter
        }
    }

    // Failure, loop finished without finding enough space for the row
    free(allocated_array);
    fprintf(stderr, "fs_allocate: Not enough contiguous space available for %zu blocks (Fragmentation).\n", blocks_to_reserve);
    return NULL;
}


ssize_t fs_create(FileSystem *fs) {
    // Validation check
    if (fs == NULL || fs->disk == NULL) {
        perror("fs_create: Error fs or disk is invalid (NULL)"); 
        return -1;
    }
    if (!fs->disk->mounted) { 
        fprintf(stderr, "fs_create: Error disk is not mounted, cannot procceed t\n");
        return -1;
    }

    int inode_num = -1;
    uint32_t *ibitmap = fs->ibitmap;
    uint32_t total_inodes = fs->meta_data->inodes;

    for (size_t i = 0; i < total_inodes; i++)
    {
        if (!(get_bit(ibitmap, i)) {
            inode_num = i;
            set_bit(ibitmap, i, 1);
            break;
        }
    }

    if (inode_num == -1) return -1;

    uint32_t block_idx = 1 + (inode_num / INODES_PER_BLOCK);
    uint32_t offset = inode_num % INODES_PER_BLOCK;

    Block block_buffer;
    if (disk_read(fs->disk, block_idx, buffer.data) < 0) return -1;

    Inode *target = &buffer.inodes[offset];
    target->valid = 1;
    target->size = 0;

    // Zeroing Out pointers
    for (size_t i = 0; i < 5; i++) target->direct[i] = 0;
    target->indirect=0;


    if (disk_write(fs->disk, block_idx, buffer.data) < 0) return -1;
    return (ssize_t)inode_num;
}

ssize_t fs_write(FileSystem *fs, size_t inode_number, char *data, size_t length, size_t offset) 
{
    // Validation check
     if (fs == NULL || fs->disk == NULL) {
        perror("fs_write: Error fs or disk is invalid (NULL)"); 
        return -1;
    }
    if (!fs->disk->mounted) { 
        fprintf(stderr, "fs_write: Error disk is not mounted, cannot procceed t\n");
        return -1;
    }
    if (inode_number >= fs->meta_data->inodes) {
        // Inode number is invalid (too high)
        fprintf(stderr, "fs_write: Error inode_number is out of bounds, cannot procceed t\n");
        return -1;
    }

    
    

}

bool fs_remove(FileSystem *fs, size_t inode_number) {return false;}
ssize_t fs_stat(FileSystem *fs, size_t inode_number) {return -1;}
ssize_t fs_read(FileSystem *fs, size_t inode_number, char *data, size_t length, size_t offset) {return -1;}
