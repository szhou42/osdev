#ifndef EXT2_H
#define EXT2_H
#include <vfs.h>

#define EXT2_DIRECT_BLOCKS 12


#define SUPERBLOCK_SIZE 1024
#define ROOT_INODE_NUMBER 2

#define EXT2_S_IFSOCK   0xC000
#define EXT2_S_IFLNK    0xA000
#define EXT2_S_IFREG    0x8000
#define EXT2_S_IFBLK    0x6000
#define EXT2_S_IFDIR    0x4000
#define EXT2_S_IFCHR    0x2000
#define EXT2_S_IFIFO    0x1000


// struct fields from http://wiki.osdev.org/Ext2#Locating_the_Superblock
typedef struct superblock {
    uint32_t total_inodes;
    uint32_t total_blocks;
    uint32_t su_blocks;
    uint32_t free_blocks;
    uint32_t free_inodes;
    uint32_t superblock_idx;
    uint32_t log2block_size;
    uint32_t log2frag_size;
    uint32_t blocks_per_group;
    uint32_t frags_per_group;
    uint32_t inodes_per_group;

    uint32_t mtime;
    uint32_t wtime;

    uint16_t mount_count;
    uint16_t mount_allowed_count;
    uint16_t ext2_magic;
    uint16_t fs_state;
    uint16_t err;
    uint16_t minor;

    uint32_t last_check;
    uint32_t interval;
    uint32_t os_id;
    uint32_t major;

    uint16_t r_userid;
    uint16_t r_groupid;

    // Extended features (not used for now since we're doing ext2 only)
    uint32_t first_inode;
    uint16_t inode_size;
    uint16_t superblock_group;
    uint32_t optional_feature;
    uint32_t required_feature;
    uint32_t readonly_feature;
    char fs_id[16];
    char vol_name[16];
    char last_mount_path[64];
    uint32_t compression_method;
    uint8_t file_pre_alloc_blocks;
    uint8_t dir_pre_alloc_blocks;
    uint16_t unused1;
    char journal_id[16];
    uint32_t journal_inode;
    uint32_t journal_device;
    uint32_t orphan_head;

    char unused2[1024-236];
}__attribute__ ((packed)) superblock_t;

typedef struct bgd {
    uint32_t block_bitmap;
    uint32_t inode_bitmap;
    uint32_t inode_table;
    uint32_t free_blocks;
    uint32_t free_inodes;
    uint32_t num_dirs;
    uint32_t unused1;
    uint32_t unused2;
}__attribute__ ((packed)) bgd_t;


typedef struct direntry {
    uint32_t inode;
    uint16_t size;
    uint8_t  name_len;
    uint8_t  type;
    char name[];
}__attribute__ ((packed)) direntry_t;

typedef struct inode {
    uint16_t permission;
    uint16_t userid;
    uint32_t size;
    uint32_t atime;
    uint32_t ctime;
    uint32_t mtime;
    uint32_t dtime;
    uint16_t gid;
    uint16_t hard_links;
    uint32_t num_sectors;
    uint32_t flags;
    uint32_t os_specific1;
    uint32_t blocks[EXT2_DIRECT_BLOCKS + 3];
    uint32_t generation;
    uint32_t file_acl;
    union {
        uint32_t dir_acl;
        uint32_t size_high;
    };
    uint32_t f_block_addr;
    char os_specific2[12];
}__attribute__ ((packed)) inode_t;

typedef struct ext2_cache {
    uint32_t block;
    uint32_t times;
    uint8_t dirty;
    char * block_data;
}ext2_cache_t;

typedef struct ext2_fs {
    // What device r u using? could be hard disk driver floppy disk driver, or USB driver.
    vfs_node_t * disk_device;

    // fs info
    superblock_t * sb;
    bgd_t * bgds;
    uint32_t block_size;
    uint32_t blocks_per_group;
    uint32_t inodes_per_group;
    uint32_t total_groups;

    uint32_t bgd_blocks;
}ext2_fs_t;

uint32_t ext2_file_size(vfs_node_t * node);

void ext2_mkdir(vfs_node_t * parent, char * name, uint16_t permission);

void ext2_mkfile(vfs_node_t * parent, char * name, uint16_t permission);

void ext2_create_entry(vfs_node_t * parent, char * entry_name, uint32_t entry_inode);

void ext2_unlink(vfs_node_t * parent, char * name);

char ** ext2_listdir(vfs_node_t * parent);

vfs_node_t * ext2_finddir(vfs_node_t * parent, char *name);

void ext2_create_entry(vfs_node_t * parent, char * entry_name, uint32_t entry_inode);

void ext2_remove_entry(vfs_node_t * parent, char * entry_name);

void ext2_chmod(vfs_node_t * file, uint32_t mode);

uint32_t ext2_read(vfs_node_t * file, uint32_t offset, uint32_t size, char * buf);

uint32_t ext2_write(vfs_node_t * file, uint32_t offset, uint32_t size, char * buf);

void ext2_open(vfs_node_t * file, uint32_t flags);

void ext2_close();

void read_inode_metadata(ext2_fs_t * ext2fs, inode_t * inode, uint32_t inode_idx);

void write_inode_metadata(ext2_fs_t * ext2fs, inode_t * inode, uint32_t inode_idx);

uint32_t read_inode_filedata(ext2_fs_t * ext2fs, inode_t * inode, uint32_t offset, uint32_t size, char * buf);

void write_inode_filedata(ext2_fs_t * ext2fs, inode_t * inode, uint32_t inode_idx, uint32_t offset, uint32_t size, char * buf);

char * read_inode_block(ext2_fs_t * ext2fs, inode_t * inode, uint32_t iblock);

void write_inode_block(ext2_fs_t * ext2fs, inode_t * inode, uint32_t iblock, char * buf);

void read_disk_block(ext2_fs_t * ext2fs, uint32_t block, char * buf);

void write_disk_block(ext2_fs_t * ext2fs, uint32_t block, char * buf);

void rewrite_bgds(ext2_fs_t * ext2fs);

void rewrite_superblock(ext2_fs_t * ext2fs);

int alloc_inode_metadata_block(uint32_t * block_ptr, ext2_fs_t * ext2fs, inode_t * inode, uint32_t inode_idx, char * buffer, unsigned int block_overwrite);

uint32_t get_disk_block_number(ext2_fs_t * ext2fs, inode_t * inode, uint32_t inode_block);

void set_disk_block_number(ext2_fs_t * ext2fs, inode_t * inode, uint32_t inode_idx, uint32_t inode_block, uint32_t disk_block);

uint32_t ext2_alloc_block(ext2_fs_t * ext2fs);

void ext2_free_block(ext2_fs_t * ext2fs, uint32_t block);

void alloc_inode_block(ext2_fs_t * ext2fs, inode_t * inode, uint32_t inode_idx, uint32_t block);

void free_inode_block(ext2_fs_t * ext2fs, inode_t * inode, uint32_t inode_idx, uint32_t block);

uint32_t alloc_inode(ext2_fs_t * ext2fs);

void free_inode(ext2_fs_t * ext2fs, uint32_t inode);

vfs_node_t * get_ext2_root(ext2_fs_t * ext2fs, inode_t * inode);

void ext2_init(char * device_path, char * mountpoint);

#endif

