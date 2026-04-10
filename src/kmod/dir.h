/* Directory Layer */

#pragma once

#include "predictfs.h"
#include <linux/types.h>

#define ENTRY_NAME_LENGTH (28)

typedef struct DirEntry DirEntry; 
struct DirEntry {
    uint32_t inode_number; // UINT32_MAX = empty/deleted slot
    char name[28]; // Dir Name
};

/* Directory Functions Prototypes (Declarations) */
