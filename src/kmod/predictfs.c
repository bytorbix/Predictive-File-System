#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/fs_context.h>
#include <linux/buffer_head.h>


#include "predictfs.h"
#include "inode.h"
#include "dir.h"


#define MAGIC_NUMBER (0xf0f03410)
#define UINT32_MAX 0xFFFFFFFFU

static struct inode_operations predictfs_inode_ops;
static struct file_operations predictfs_file_ops;




static int predictfs_iterate_shared(struct file *file, struct dir_context *ctx) {
    printk(KERN_INFO "predictfs: iterate_shared is working");
    printk(KERN_INFO "ctx->pos: %lld", ctx->pos);
    int inode_dir_number = file->f_inode->i_ino;
    int block_idx = 1 + (inode_dir_number / INODES_PER_BLOCK);

    struct buffer_head *bh = sb_bread(file->f_inode->i_sb, block_idx);
    if (!bh) return -EIO;

    Inode *inode_block = (Inode *)bh->b_data;
    Inode *dir_inode = &inode_block[inode_dir_number % INODES_PER_BLOCK];

    struct buffer_head *bh2 = sb_bread(file->f_inode->i_sb, dir_inode->extents[0].start);
    if (!bh2) return -EIO;
    struct DirEntry *entries_block = (DirEntry *)bh2->b_data;

    if (!dir_emit_dots(file, ctx)) return 0;
    int start = ctx->pos - 2;
    for (size_t i = 0; i < dir_inode->size / sizeof(DirEntry); i++) {
        printk(KERN_INFO "Iterating %s Num: %d", entries_block[i].name, entries_block[i].inode_number);
        if ((ssize_t)i < start) continue;
        DirEntry entry;
        entry = entries_block[i];
        if (entry.inode_number != UINT32_MAX) {
            if (!dir_emit(ctx, entry.name, ENTRY_NAME_LENGTH, entry.inode_number, DT_UNKNOWN)) {
                return 0;
            }
        }
        ctx->pos++;
    }
    brelse(bh);
    brelse(bh2);
    return 0;
}

// Searches a directory for a named entry and returns its inode number, -1 if not found
struct dentry *lookup(struct inode *dir, struct dentry *dentry, unsigned int flags)
{
    // Read the directory inode and confirm it's a directory
    uint32_t inode_block_idx = 1 + (dir->i_ino / INODES_PER_BLOCK);
    uint32_t inode_offset = dir->i_ino % INODES_PER_BLOCK;

    struct buffer_head *bh = sb_bread(dir->i_sb, inode_block_idx);
    if (!bh) return ERR_PTR(-EIO);
    Inode *target = &((Inode *)bh->b_data)[inode_offset];
    
    struct buffer_head *data_bh = sb_bread(dir->i_sb, target->extents[0].start);
    if (!data_bh) return ERR_PTR(-EIO);
    DirEntry *entries = (DirEntry *)data_bh->b_data;

    // Scan entries for a name match, skipping deleted slots
    for (size_t i = 0; i < target->size/32; i++)
    {   
        if (entries[i].inode_number != UINT32_MAX && strcmp(entries[i].name, dentry->d_name.name) == 0) {
            struct inode *inode = iget_locked(dir->i_sb, entries[i].inode_number);
            if (!inode) return ERR_PTR(-ENOMEM);
            if (inode->i_state & I_NEW) {

                inode_block_idx = 1 + (entries[i].inode_number / INODES_PER_BLOCK);
                inode_offset = entries[i].inode_number % INODES_PER_BLOCK;

                struct buffer_head *bh2 = sb_bread(dir->i_sb, inode_block_idx);
                if (!bh2) {
                    iput(inode);
                    return ERR_PTR(-EIO);
                }
                target = &((Inode *)bh2->b_data)[inode_offset];

                inode->i_size = target->size;
                inode->i_mode = (target->valid == INODE_DIR) ? S_IFDIR | 0755 : S_IFREG | 0644;
                inode->i_uid = GLOBAL_ROOT_UID;
                inode->i_gid = GLOBAL_ROOT_GID;
                inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);
                inode->i_op = &predictfs_inode_ops;
                inode->i_fop = &predictfs_file_ops;
                unlock_new_inode(inode);
            }
            d_add(dentry, inode);
            return NULL;
        }
    }
    d_add(dentry, NULL); return NULL;
}

static struct file_operations predictfs_file_ops = {
    .open = simple_open,
    .release = dcache_dir_close,
    .llseek = generic_file_llseek,
    .read = generic_read_dir,
    .fsync = noop_fsync,
    .iterate_shared = predictfs_iterate_shared,
};

static const struct super_operations predictfs_super_ops = {
    
};

static struct inode_operations predictfs_inode_ops = {
    .lookup = lookup,
};



static int predictfs_fill_super(struct super_block *sb, struct fs_context *fc) {
    // initialize superblock fields
    if (sb_set_blocksize(sb, PFS_BLOCK_SIZE) == 0) {
        return -1; //error
    }
    sb->s_magic = MAGIC_NUMBER;
    sb->s_op = &predictfs_super_ops;
    
    
    struct buffer_head *bh = sb_bread(sb, 0);
    if (!bh) return -EIO;

    SuperBlock *disk_sb = (SuperBlock *)bh->b_data;
    // magic verify
    if (disk_sb->magic_number != MAGIC_NUMBER) {
        brelse(bh);
        printk(KERN_ERR "predictfs: invalid magic number: %x\n", disk_sb->magic_number);
        return -EINVAL;
    }

    struct inode *sb_inode = new_inode(sb);
    if (!sb_inode) {
        brelse(bh);
        return -1;// error
    }
    set_nlink(sb_inode, 2);
    sb_inode->i_mode = S_IFDIR | 0755;
    sb_inode->i_ino = 0;
    sb_inode->i_op = &predictfs_inode_ops;
    sb_inode->i_fop = &predictfs_file_ops;
    
    struct dentry *sb_dentry = d_make_root(sb_inode);
    if (!sb_dentry) {
        brelse(bh);
        return -1;// error
    }
    sb->s_root = sb_dentry;

    brelse(bh);
    return 0;
};

static int predictfs_get_tree(struct fs_context *fc) {
    return get_tree_bdev(fc, predictfs_fill_super);
}


static const struct fs_context_operations predictfs_context_ops = {
    .get_tree = predictfs_get_tree,
};

int predictfs_init_fs_context(struct fs_context *fc) {
    fc->ops = &predictfs_context_ops;
    return 0;
}





static struct file_system_type fs = {
    .name = "predictfs",
    .owner = THIS_MODULE,
    .init_fs_context = predictfs_init_fs_context,
    .kill_sb = kill_block_super,
    .fs_flags = FS_REQUIRES_DEV,
};



static int __init pfs_init(void) {
    register_filesystem(&fs);
    printk(KERN_INFO "predictfs: loaded!\n");
    return 0;
}

static void __exit pfs_exit(void) {
    unregister_filesystem(&fs);
    printk(KERN_INFO "predictfs: exiting!\n");
    return;
}

module_init(pfs_init);
module_exit(pfs_exit);
MODULE_LICENSE("GPL");