/* File System Header */

#pragma once

#include <linux/types.h>
#include "inode.h"


// File System Constants
#define MAGIC_NUMBER (0xf0f03410)
#define INODES_PER_BLOCK (PFS_BLOCK_SIZE / sizeof(Inode))



// File System Structure

typedef struct SuperBlock SuperBlock;
struct SuperBlock {
    uint32_t magic_number;  // File system type identifier
    uint32_t blocks;        // Total number of blocks in the file system
    uint32_t inode_blocks;  // Total number of blocks reserved for the Inode Table
    uint32_t inodes;        // Total number of Inode structures
    uint32_t bitmap_blocks;  // Total amount of bitmap blocks
};