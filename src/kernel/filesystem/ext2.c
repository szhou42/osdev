#include <ext2.h>
#include <system.h>
#include <string.h>
#include <printf.h>

uint32_t ext2_file_size(vfs_node_t * node) {
    ext2_fs_t * ext2fs = node->device;
    inode_t * inode = kmalloc(sizeof(inode_t));
    read_inode_metadata(ext2fs, inode, node->inode_num);
    uint32_t ret = inode->size;
    kfree(inode);
    return ret;
}
/*
 * Both ext2_mkdir and ext2_mkfile calls ext2_create_entry to create an entry under certain directory
 * */
void ext2_mkdir(vfs_node_t * parent, char * name, uint16_t permission) {
    ext2_fs_t * ext2fs = parent->device;
    uint32_t inode_idx = alloc_inode(ext2fs);
    inode_t * inode = kmalloc(sizeof(inode_t));
    read_inode_metadata(ext2fs, inode, inode_idx);
    inode->permission = EXT2_S_IFDIR;
    inode->permission |= 0xFFF & permission;
    inode->atime = 0;
    inode->ctime = 0;
    inode->dtime = 0;
    inode->gid = 0;
    inode->userid = 0;
    inode->f_block_addr = 0;
    inode->num_sectors = 0;
    inode->size = ext2fs->block_size;
    inode->hard_links = 2;
    inode->flags = 0;
    inode->file_acl = 0;
    inode->dir_acl = 0;
    inode->generation = 0;
    inode->os_specific1 = 0;
    memset(inode->blocks, 0, sizeof(inode->blocks));
    memset(inode->os_specific2, 0, 12);
    // Let's allocate one block for each directory made(temporary solution, we should actually expand size of directory when adding sub-dir to it)
    alloc_inode_block(ext2fs, inode, inode_idx, 0);
    write_inode_metadata(ext2fs, inode, inode_idx);
    ext2_create_entry(parent, name, inode_idx);

    // May be add a "." and ".." to the entry ?

    inode_t * p_inode = kmalloc(sizeof(inode_t));
    read_inode_metadata(ext2fs, p_inode, parent->inode_num);
    p_inode->hard_links++;
    write_inode_metadata(ext2fs, p_inode, parent->inode_num);
    rewrite_bgds(ext2fs);
}

/*
 * Both ext2_mkdir and ext2_mkfile calls ext2_create_entry to create an entry under certain directory
 * */
void ext2_mkfile(vfs_node_t * parent, char * name, uint16_t permission) {
    ext2_fs_t * ext2fs = parent->device;
    uint32_t inode_idx = alloc_inode(ext2fs);
    inode_t * inode = kmalloc(sizeof(inode_t));
    read_inode_metadata(ext2fs, inode, inode_idx);
    inode->permission = EXT2_S_IFREG;
    inode->permission |= 0xFFF & permission;
    inode->atime = 0;
    inode->ctime = 0;
    inode->dtime = 0;
    inode->gid = 0;
    inode->userid = 0;
    inode->f_block_addr = 0;
    inode->num_sectors = 0;
    inode->size = ext2fs->block_size;
    inode->hard_links = 2;
    inode->flags = 0;
    inode->file_acl = 0;
    inode->dir_acl = 0;
    inode->generation = 0;
    inode->os_specific1 = 0;
    memset(inode->blocks, 0, sizeof(inode->blocks));
    memset(inode->os_specific2, 0, 12);
    alloc_inode_block(ext2fs, inode, inode_idx, 0);
    write_inode_metadata(ext2fs, inode, inode_idx);
    ext2_create_entry(parent, name, inode_idx);

    inode_t * p_inode = kmalloc(sizeof(inode_t));
    read_inode_metadata(ext2fs, p_inode, parent->inode_num);
    p_inode->hard_links++;
    write_inode_metadata(ext2fs, p_inode, parent->inode_num);
    rewrite_bgds(ext2fs);
}

/*
 * Delete the file
 * */
void ext2_unlink(vfs_node_t * parent, char * name) {
    // OK... Just find the direntry and set inode = 0, there is a link count for each file/dir, which says you can only really delete the file(deallocate inode and blocks) when link count is 0
    // But we don't care because we don't deallocate anything at all ! and we actually have not supported any hard/soft links yet.
    ext2_fs_t * ext2fs = parent->device;
    ext2_remove_entry(parent, name);

    inode_t * p_inode = kmalloc(sizeof(inode_t));
    read_inode_metadata(ext2fs, p_inode, parent->inode_num);
    p_inode->hard_links--;
    write_inode_metadata(ext2fs, p_inode, parent->inode_num);
    rewrite_bgds(ext2fs);

}

/*
 * List directories under a certain directory
 * */
char ** ext2_listdir(vfs_node_t * parent) {
    ext2_fs_t * ext2fs = parent->device;
    inode_t * p_inode = kmalloc(sizeof(inode_t));
    read_inode_metadata(ext2fs, p_inode, parent->inode_num);
    uint32_t curr_offset = 0;
    uint32_t block_offset = 0;
    uint32_t in_block_offset = 0;
    int size = 0, cap = 10;
    char ** ret = kmalloc(sizeof(char*) * cap);
    char * block_buf = read_inode_block(ext2fs, p_inode, block_offset);
    while(curr_offset < p_inode->size) {
        if(in_block_offset >= ext2fs->block_size) {
            block_offset++;
            in_block_offset = 0;
            block_buf = read_inode_block(ext2fs, p_inode, block_offset);
        }
        if(size + 1 == cap) {
            ret = krealloc(ret, sizeof(char*) * cap * 2);
            cap = cap * 2;
        }

        direntry_t * curr_dir = (direntry_t*)(block_buf + in_block_offset);
        if(curr_dir->inode != 0) {
            char * temp = kcalloc(curr_dir->name_len + 1, 1);
            memcpy(temp, curr_dir->name, curr_dir->name_len);
            ret[size++] = temp;
        }
        uint32_t expected_size = ((sizeof(direntry_t) + curr_dir->name_len) & 0xfffffffc) + 0x4;
        uint32_t real_size = curr_dir->size;
        if(real_size != expected_size) {
            break;
        }
        in_block_offset += curr_dir->size;
        curr_offset += curr_dir->size;
    }
    ret[size] = NULL;
    return ret;
}
/*
 * Given a inode, dir entry, construct a vfs node and return
 * */
vfs_node_t * vfsnode_from_direntry(ext2_fs_t * ext2fs, direntry_t * dir, inode_t * inode) {
    vfs_node_t * ret = kcalloc(sizeof(vfs_node_t), 1);

    ret->device = (void*)ext2fs;
    ret->inode_num = dir->inode;
    memcpy(ret->name, dir->name, dir->name_len);

    ret->uid = inode->userid;
    ret->uid = inode->gid;
    ret->size = inode->size;
    ret->mask = inode->permission & 0xFFF;
    ret->nlink = inode->hard_links;

    ret->flags = 0;
    if ((inode->permission & EXT2_S_IFREG) == EXT2_S_IFREG) {
        ret->flags   |= FS_FILE;
        ret->read     = ext2_read;
        ret->write    = ext2_write;
        ret->unlink = ext2_unlink;
        ret->get_file_size = ext2_file_size;
    }
    if ((inode->permission & EXT2_S_IFDIR) == EXT2_S_IFDIR) {
        ret->flags   |= FS_DIRECTORY;
        ret->mkdir    = ext2_mkdir;
        ret->finddir  = ext2_finddir;
        ret->unlink   = ext2_unlink;
        ret->create   = ext2_mkfile;
        ret->listdir  = ext2_listdir;
        ret->read     = ext2_read;
        ret->write    = ext2_write;
    }
    if ((inode->permission & EXT2_S_IFBLK) == EXT2_S_IFBLK) {
        ret->flags |= FS_BLOCKDEVICE;
    }
    if ((inode->permission & EXT2_S_IFCHR) == EXT2_S_IFCHR) {
        ret->flags |= FS_CHARDEVICE;
    }
    if ((inode->permission & EXT2_S_IFIFO) == EXT2_S_IFIFO) {
        ret->flags |= FS_PIPE;
    }
    if ((inode->permission & EXT2_S_IFLNK) == EXT2_S_IFLNK) {
        ret->flags   |= FS_SYMLINK;
    }

    ret->access_time   = inode->atime;
    ret->modified_time   = inode->mtime;
    ret->create_time   = inode->ctime;

    ret->chmod   = ext2_chmod;
    ret->open    = ext2_open;
    ret->close   = ext2_close;
    return ret;
}

vfs_node_t * ext2_finddir(vfs_node_t * parent, char *name) {
    ext2_fs_t * ext2fs = parent->device;
    inode_t * p_inode = kmalloc(sizeof(inode_t));
    read_inode_metadata(ext2fs, p_inode, parent->inode_num);
    uint32_t curr_offset = 0;
    uint32_t block_offset = 0;
    uint32_t in_block_offset = 0;
    char * block_buf = read_inode_block(ext2fs, p_inode, block_offset);
    while(curr_offset < p_inode->size) {
        if(in_block_offset >= ext2fs->block_size) {
            block_offset++;
            in_block_offset = 0;
            block_buf = read_inode_block(ext2fs, p_inode, block_offset);
        }

        direntry_t * curr_dir = (direntry_t*)(block_buf + in_block_offset);
        char * temp = kcalloc(curr_dir->name_len + 1, 1);
        memcpy(temp, curr_dir->name, curr_dir->name_len);
        if(curr_dir->inode != 0 && !strcmp(temp, name)) {
             // Create a vfs node from the entry and return it
             inode_t * inode = kmalloc(sizeof(inode_t));
             read_inode_metadata(ext2fs, inode, curr_dir->inode);
             return vfsnode_from_direntry(ext2fs, curr_dir, inode);
        }
        uint32_t expected_size = ((sizeof(direntry_t) + curr_dir->name_len) & 0xfffffffc) + 0x4;
        uint32_t real_size = curr_dir->size;
        if(real_size != expected_size) {
            break;
        }
        in_block_offset += curr_dir->size;
        curr_offset += curr_dir->size;
    }
    return NULL;
}

/*
 * Create an entry under certain dir
 * The parent should always be a directory
 *
 * OK, How are we gonna do this?
 * For a directory inode, its data is simply a bunch of directory entry that tells you where the sub-directories and files are
 * So, just add an entry.
 * Each directory entry could have different length, so we could think of it as a memory pool with memory chunks of different size in it, and manage it like what we did with malloc
 * But instead, I don't care about wasting a few hundred bytes because... this is just a toy os
 * So what we're going to do is just append an entry in the end
 * But even if we're avoiding complicated method, I believe my current impl still has many bugs for lage number of entries, but let's just make it work for now
 * */
void ext2_create_entry(vfs_node_t * parent, char * entry_name, uint32_t entry_inode) {
    ext2_fs_t * ext2fs = parent->device;
    inode_t * p_inode = kmalloc(sizeof(inode_t));
    read_inode_metadata(ext2fs, p_inode, parent->inode_num);
    uint32_t curr_offset = 0;
    uint32_t block_offset = 0;
    uint32_t in_block_offset = 0;
    uint32_t found = 0;
    uint32_t entry_name_len = strlen(entry_name);
    char * check = kcalloc(entry_name_len + 1, 1);
    char * block_buf = read_inode_block(ext2fs, p_inode, block_offset);
    // Note: It is required that no directory entry cross block boundary && each directory entry be aligned on 4-byte boundary
    while(curr_offset < p_inode->size) {
        if(in_block_offset >= ext2fs->block_size) {
            block_offset++;
            in_block_offset = 0;
            block_buf = read_inode_block(ext2fs, p_inode, block_offset);
        }
        direntry_t * curr_dir = (direntry_t*)(block_buf + in_block_offset);
        if(curr_dir->name_len == entry_name_len) {
            memcpy(check, curr_dir->name, entry_name_len);
            if(curr_dir->inode != 0 && !strcmp(entry_name, check)) {
                printf("Entry by the same name %s already exist\n", check);
                return;
            }
        }
        // Found the last entry
        if(found) {
            // Overwrite this last entry with our new entry
            curr_dir->inode = entry_inode;
            curr_dir->size = (uint32_t)block_buf + ext2fs->block_size - (uint32_t)curr_dir;
            curr_dir->name_len = strlen(entry_name);
            curr_dir->type = 0;
            // Must use memcpy instead of strcpy, because name in direntry does not contain ending '\0'
            memcpy(curr_dir->name, entry_name, strlen(entry_name));
            write_inode_block(ext2fs, p_inode, block_offset, block_buf);
            // Then, append a new ending entry
            // Be careful about a spcial case here, if not enough space for ending entry, put it in the new block
            in_block_offset += curr_dir->size;
            if(in_block_offset >= ext2fs->block_size) {
                 block_offset++;
                 in_block_offset = 0;
                 block_buf = read_inode_block(ext2fs, p_inode, block_offset);
            }
            curr_dir = (direntry_t*)(block_buf + in_block_offset);
            memset(curr_dir, 0, sizeof(direntry_t));
            write_inode_block(ext2fs, p_inode, block_offset, block_buf);
            return;
        }
        uint32_t expected_size = ((sizeof(direntry_t) + curr_dir->name_len) & 0xfffffffc) + 0x4;
        uint32_t real_size = curr_dir->size;
        if(real_size != expected_size) {
            // Mark found and fix the size
            found = 1;
            curr_dir->size = expected_size;
            in_block_offset += expected_size;
            curr_offset += expected_size;
            continue;
        }
        in_block_offset += curr_dir->size;
        curr_offset += curr_dir->size;
    }
}

/*
*  Remove an entry
*/
void ext2_remove_entry(vfs_node_t * parent, char * entry_name) {
    ext2_fs_t * ext2fs = parent->device;
    inode_t * p_inode = kmalloc(sizeof(inode_t));
    read_inode_metadata(ext2fs, p_inode, parent->inode_num);
    uint32_t curr_offset = 0;
    uint32_t block_offset = 0;
    uint32_t in_block_offset = 0;
    uint32_t entry_name_len = strlen(entry_name);
    char * check = kcalloc(entry_name_len + 1, 1);
    char * block_buf = read_inode_block(ext2fs, p_inode, block_offset);
    // Note: It is required that no directory entry cross block boundary && each directory entry be aligned on 4-byte boundary
    while(curr_offset < p_inode->size) {
        if(in_block_offset >= ext2fs->block_size) {
            block_offset++;
            in_block_offset = 0;
            block_buf = read_inode_block(ext2fs, p_inode, block_offset);
        }
        direntry_t * curr_dir = (direntry_t*)(block_buf + in_block_offset);
        if(curr_dir->name_len == entry_name_len) {
            memcpy(check, curr_dir->name, entry_name_len);
            if(curr_dir->inode != 0 && !strcmp(entry_name, check)) {
                curr_dir->inode = 0;
                write_inode_block(ext2fs, p_inode, block_offset, block_buf);
                return;
            }
        }
        uint32_t expected_size = ((sizeof(direntry_t) + curr_dir->name_len) & 0xfffffffc) + 0x4;
        uint32_t real_size = curr_dir->size;
        // Found the last entry
        if(real_size != expected_size)
            return;
        in_block_offset += curr_dir->size;
        curr_offset += curr_dir->size;
    }
}

void ext2_chmod(vfs_node_t * file, uint32_t mode) {
    ext2_fs_t * ext2fs = file->device;
    inode_t * inode = kmalloc(sizeof(inode_t));
    read_inode_metadata(ext2fs, inode, file->inode_num);
    inode->permission = (inode->permission & 0xFFFFF000) | mode;
    write_inode_metadata(ext2fs, inode, file->inode_num);
}

/*
 * Read n bytes to file starting from offset
 * */
uint32_t ext2_read(vfs_node_t * file, uint32_t offset, uint32_t size, char * buf) {
    // Extract the ext2 filesystem object and inode from vfs node
    ext2_fs_t * ext2fs = file->device;
    inode_t * inode = kmalloc(sizeof(inode_t));
    read_inode_metadata(ext2fs, inode, file->inode_num);
    read_inode_filedata(ext2fs, inode, offset, size, buf);
    return size;
}

/*
 * Write n bytes to file starting from offset
 * */
uint32_t ext2_write(vfs_node_t * file, uint32_t offset, uint32_t size, char * buf) {
    // Extract the ext2 filesystem object and inode from vfs node
    ext2_fs_t * ext2fs = file->device;
    inode_t * inode = kmalloc(sizeof(inode_t));
    read_inode_metadata(ext2fs, inode, file->inode_num);
    write_inode_filedata(ext2fs, inode, file->inode_num, offset, size, buf);
    return size;
}

/*
 * Open ext2 file/dir
 * */
void ext2_open(vfs_node_t * file, uint32_t flags) {
    ext2_fs_t * ext2fs = file->device;
    // Overwrite the file on open
    if (flags & O_TRUNC) {
        inode_t * inode = kmalloc(sizeof(inode_t));
        read_inode_metadata(ext2fs, inode, file->inode_num);
        inode->size = 0;
        write_inode_metadata(ext2fs, inode, file->inode_num);
    }
}

/*
 * Close ext2 file/dir
 * */
void ext2_close() {
    return;
}


/*
 * Given a inode number, find the inode on disk and read it
 * */
void read_inode_metadata(ext2_fs_t * ext2fs, inode_t * inode, uint32_t inode_idx) {
    // Which group the inode lives in
    uint32_t group = inode_idx / ext2fs->inodes_per_group;
    // The block that points to a table of all inodes
    uint32_t inode_table_block = ext2fs->bgds[group].inode_table;
    uint32_t idx_in_group = inode_idx - group * ext2fs->inodes_per_group;
    // Which block does the inode live in ? You may wonder why inode is subtracted by 1 here, it's because inode number starts from 1, not 0.(inode of 0 means error)
    uint32_t block_offset = (idx_in_group - 1) * ext2fs->sb->inode_size / ext2fs->block_size;
    // Offset within block
    uint32_t offset_in_block = (idx_in_group - 1) - block_offset * (ext2fs->block_size / ext2fs->sb->inode_size);
    char * block_buf = kmalloc(ext2fs->block_size);
    read_disk_block(ext2fs, inode_table_block + block_offset, block_buf);
    memcpy(inode, block_buf + offset_in_block * ext2fs->sb->inode_size, ext2fs->sb->inode_size);
    kfree(block_buf);
}

/*
 * Given a inode number, find the inode on disk and overwrite it with the provided inode
 * */
void write_inode_metadata(ext2_fs_t * ext2fs, inode_t * inode, uint32_t inode_idx) {
    // Which group the inode lives in
    uint32_t group = inode_idx / ext2fs->inodes_per_group;
    // The block that points to a table of all inodes
    uint32_t inode_table_block = ext2fs->bgds[group].inode_table;
    //uint32_t idx_in_group = inode_idx - group * ext2fs->inodes_per_group;
    // Which block does the inode live in ? You may wonder why inode is subtracted by 1 here, it's because inode number starts from 1, not 0.(inode of 0 means error)
    uint32_t block_offset = (inode_idx - 1) * ext2fs->sb->inode_size / ext2fs->block_size;
    // Offset within block
    uint32_t offset_in_block = (inode_idx - 1) - block_offset * (ext2fs->block_size / ext2fs->sb->inode_size);
    char * block_buf = kmalloc(ext2fs->block_size);
    read_disk_block(ext2fs, inode_table_block + block_offset, block_buf);
    memcpy(block_buf + offset_in_block * ext2fs->sb->inode_size, inode, ext2fs->sb->inode_size);
    write_disk_block(ext2fs, inode_table_block + block_offset, block_buf);
    kfree(block_buf);
}

/*
 * Given a inode, offset, and size, find the inode blocks and read
 * This function reads the actual file data referenced by the inode, not the metadata
 * */
uint32_t read_inode_filedata(ext2_fs_t * ext2fs, inode_t * inode, uint32_t offset, uint32_t size, char * buf) {
    uint32_t end_offset = (inode->size >= offset + size) ? (offset + size) : (inode->size);
    // Convert the offset/size to some starting/end iblock numbers
    uint32_t start_block = offset / ext2fs->block_size;
    uint32_t end_block = end_offset / ext2fs->block_size;
    // What's the offset into the start block
    uint32_t start_off = offset % ext2fs->block_size;
    // How much bytes to read for the end block
    uint32_t end_size = end_offset - end_block * ext2fs->block_size;

    uint32_t i = start_block;
    uint32_t curr_off = 0;
    while(i <= end_block) {
        uint32_t left = 0, right = ext2fs->block_size - 1;
        char * block_buf = read_inode_block(ext2fs, inode, i);
        if(i == start_block)
            left = start_off;
        if(i == end_block)
            right = end_size - 1;
        memcpy(buf + curr_off, block_buf + left, (right - left + 1));
        curr_off = curr_off + (right - left + 1);
        kfree(block_buf);
        i++;
    }
    return end_offset - offset;
}

/*
 * Given a inode, offset, and size, find the inode blocks and write
 * This function writes to the actual file data referenced by the inode, not the metadata
 * */
void write_inode_filedata(ext2_fs_t * ext2fs, inode_t * inode, uint32_t inode_idx, uint32_t offset, uint32_t size, char * buf) {
    if(offset + size > inode->size) {
        inode->size = offset + size;
        write_inode_metadata(ext2fs, inode, inode_idx);
    }
    // Writing inode filedata is similar to reading inode filedata
    uint32_t end_offset = (inode->size >= offset + size) ? (offset + size) : (inode->size);
    // Convert the offset/size to some starting/end iblock numbers
    uint32_t start_block = offset / ext2fs->block_size;
    uint32_t end_block = end_offset / ext2fs->block_size;
    // What's the offset into the start block
    uint32_t start_off = offset % ext2fs->block_size;
    // How much bytes to read for the end block
    uint32_t end_size = end_offset - end_block * ext2fs->block_size;

    uint32_t i = start_block;
    uint32_t curr_off = 0;
    while(i <= end_block) {
        uint32_t left = 0, right = ext2fs->block_size;
        char * block_buf = read_inode_block(ext2fs, inode, i);

        if(i == start_block)
            left = start_off;
        if(i == end_block)
            right = end_size - 1;
        memcpy(block_buf + left, buf + curr_off, (right - left + 1));
        curr_off = curr_off + (right - left + 1);
        write_inode_block(ext2fs, inode, i, block_buf);
        kfree(block_buf);
        i++;
    }
}

/*
 * Read a block in in specified inode
 * For example, iblock = 3 reads the direct block with index 3
 * */
char * read_inode_block(ext2_fs_t * ext2fs, inode_t * inode, uint32_t iblock) {
    char * buf = kmalloc(ext2fs->block_size);
    // Get the actual disk block number of the iblock
    uint32_t disk_block = get_disk_block_number(ext2fs, inode, iblock);
    // Then just read a disk block
    read_disk_block(ext2fs, disk_block, buf);
    return buf;
}

/*
 * Write a block in in specified inode
 * For example, iblock = 3 writes to the direct block with index 3
 * */
void write_inode_block(ext2_fs_t * ext2fs, inode_t * inode, uint32_t iblock, char * buf) {
    // Get the actual disk block number of the iblock
    uint32_t disk_block = get_disk_block_number(ext2fs, inode, iblock);
    // Then just write a disk block
    write_disk_block(ext2fs, disk_block, buf);
}

/*
 * Read buffer from disk block specified by block
 * */
void read_disk_block(ext2_fs_t * ext2fs, uint32_t block, char * buf) {
    // Simply call the hard disk/floppy/whatever driver to read two consecutive sectors
    vfs_read(ext2fs->disk_device, ext2fs->block_size * block, ext2fs->block_size, buf);
}

/*
 * Write buffer to disk block specified by block
 * */
void write_disk_block(ext2_fs_t * ext2fs, uint32_t block, char * buf) {
    // Simply call the hard disk/floppy/whatever driver to write two consecutive sectors
    vfs_write(ext2fs->disk_device, ext2fs->block_size * block, ext2fs->block_size, buf);
}

void rewrite_bgds(ext2_fs_t * ext2fs) {
    for(uint32_t i = 0; i < ext2fs->bgd_blocks; i++)
        write_disk_block(ext2fs, 2, (void*)ext2fs->bgds + i * ext2fs->block_size);
}

void rewrite_superblock(ext2_fs_t * ext2fs) {
    write_disk_block(ext2fs, 1, (void*)ext2fs->sb);
}

/*
 * A helper function for set_block_number as the same thing has bee done many times
 *Allocate block for a inode if the table entry says the block is not already allocated
 * */

int alloc_inode_metadata_block(uint32_t * block_ptr, ext2_fs_t * ext2fs, inode_t * inode, uint32_t inode_idx, char * buffer, unsigned int block_overwrite) {
    if(!(*block_ptr)) {
        unsigned int block_no = ext2_alloc_block(ext2fs);
        if(!block_no) return 0;
        *block_ptr = block_no;
        if(buffer)
            write_disk_block(ext2fs, block_overwrite, (void*)buffer);
        else
            write_inode_metadata(ext2fs, inode, inode_idx);
        return 1;
    }
    return 0;
}
/*
 * It calculate where iblock is located within the block tables, and get the actuual index of the block on disk
 * Note that iblock refers to the linear block address of the inode, whereas dblock refers to the block on disk
 * */
uint32_t get_disk_block_number(ext2_fs_t * ext2fs, inode_t * inode, uint32_t inode_block) {
    unsigned int p = ext2fs->block_size / 4, ret;
    int a, b, c, d, e, f, g;
    unsigned int * tmp = kmalloc(ext2fs->block_size);
    // How many blocks are left except for direct blocks ?
    a = inode_block - EXT2_DIRECT_BLOCKS;
    if(a < 0) {
        ret = inode->blocks[inode_block];
        goto done;
    }
    b = a - p;
    if(b < 0) {
        read_disk_block(ext2fs, inode->blocks[EXT2_DIRECT_BLOCKS], (void*)tmp);
        ret = tmp[a];
        goto done;
    }
    c = b - p * p;
    if(c < 0) {
        c = b / p;
        d = b - c * p;
        read_disk_block(ext2fs, inode->blocks[EXT2_DIRECT_BLOCKS + 1], (void*)tmp);
        read_disk_block(ext2fs, tmp[c], (void*)tmp);
        ret = tmp[d];
        goto done;
    }
    d = c - p * p * p;
    if(d < 0) {
        e = c / (p * p);
        f = (c - e * p * p) / p;
        g = (c - e * p * p - f * p);
        read_disk_block(ext2fs, inode->blocks[EXT2_DIRECT_BLOCKS + 2], (void*)tmp);
        read_disk_block(ext2fs, tmp[e], (void*)tmp);
        read_disk_block(ext2fs, tmp[f], (void*)tmp);
        ret = tmp[g];
        goto done;
    }
done:
    kfree(tmp);
    return ret;
}
/*
 * It calculate where iblock is located within the block tables, and then assign a block number to it
 * Note that iblock refers to the linear block address of the inode, whereas dblock refers to the block on disk
 * */
void set_disk_block_number(ext2_fs_t * ext2fs, inode_t * inode, uint32_t inode_idx, uint32_t inode_block, uint32_t disk_block) {
    unsigned int p = ext2fs->block_size / 4;
    int a, b, c, d, e, f, g;
    int iblock = inode_block;
    unsigned int * tmp = kmalloc(ext2fs->block_size);

    a = iblock - EXT2_DIRECT_BLOCKS;
    if(a <= 0) {
        inode->blocks[inode_block] = disk_block;
        goto done;
    }
    b = a - p;
    if(b <= 0) {
        if(!alloc_inode_metadata_block(&(inode->blocks[EXT2_DIRECT_BLOCKS]), ext2fs, inode, inode_idx, NULL, 0));
        read_disk_block(ext2fs, inode->blocks[EXT2_DIRECT_BLOCKS], (void*)tmp);
        ((unsigned int*)tmp)[a] = disk_block;
        write_disk_block(ext2fs, inode->blocks[EXT2_DIRECT_BLOCKS], (void*)tmp);
        tmp[a] = disk_block;
        goto done;
    }
    c = b - p * p;
    if(c <= 0) {
        c = b / p;
        d = b - c * p;
        if(!alloc_inode_metadata_block(&(inode->blocks[EXT2_DIRECT_BLOCKS + 1]), ext2fs, inode, inode_idx, NULL, 0));
        read_disk_block(ext2fs, inode->blocks[EXT2_DIRECT_BLOCKS + 1], (void*)tmp);
        if(!alloc_inode_metadata_block(&(tmp[c]), ext2fs, inode, inode_idx, (void*)tmp, inode->blocks[EXT2_DIRECT_BLOCKS + 1]));
        unsigned int temp = tmp[c];
        read_disk_block(ext2fs, temp, (void*)tmp);
        tmp[d] = disk_block;
        write_disk_block(ext2fs, temp, (void*)tmp);
        goto done;
    }
    d = c - p * p * p;
    if(d <= 0) {
        e = c / (p * p);
        f = (c - e * p * p) / p;
        g = (c - e * p * p - f * p);
        if(!alloc_inode_metadata_block(&(inode->blocks[EXT2_DIRECT_BLOCKS + 2]), ext2fs, inode, inode_idx, NULL, 0));
        read_disk_block(ext2fs, inode->blocks[EXT2_DIRECT_BLOCKS + 2], (void*)tmp);
        if(!alloc_inode_metadata_block(&(tmp[e]), ext2fs, inode, inode_idx, (void*)tmp, inode->blocks[EXT2_DIRECT_BLOCKS + 2]));
        unsigned int temp = tmp[e];
        read_disk_block(ext2fs, tmp[e], (void*)tmp);
        if(!alloc_inode_metadata_block(&(tmp[f]), ext2fs, inode, inode_idx, (void*)tmp, temp));
        temp = tmp[f];
        read_disk_block(ext2fs, tmp[f], (void*)tmp);
        tmp[g] = disk_block;
        write_disk_block(ext2fs, temp, (void*)tmp);
        goto done;
    }
done:
    kfree(tmp);
}

/*
 * Allocate block from the ext2 block bitmaps
 * */
uint32_t ext2_alloc_block(ext2_fs_t * ext2fs) {
    uint32_t * buf = kcalloc(ext2fs->block_size, 1);
    // Read the inode bitmap, find free inode, return its index
    for(uint32_t i = 0; i < ext2fs->total_groups; i++) {
        if(!ext2fs->bgds[i].free_blocks)
            continue;

        uint32_t bitmap_block = ext2fs->bgds[i].block_bitmap;
        read_disk_block(ext2fs, bitmap_block, (void*)buf);
        for(uint32_t j = 0; j < ext2fs->block_size / 4; j++) {
            uint32_t sub_bitmap = buf[j];
            if(sub_bitmap == 0xFFFFFFFF)
                continue;
            for(uint32_t k = 0; k < 32; k++) {
                uint32_t free = !((sub_bitmap >> k) & 0x1);
                if(free) {
                    // Set bitmap and return
                    uint32_t mask = (0x1 << k);
                    buf[j] = buf[j] | mask;
                    write_disk_block(ext2fs, bitmap_block, (void*)buf);
                    // update free_inodes
                    ext2fs->bgds[i].free_blocks--;
                    rewrite_bgds(ext2fs);
                    return i * ext2fs->blocks_per_group + j * 32 + k;
                }
            }
        }
    }
    PANIC("We're out of blocks!\n");
    return (uint32_t)-1;

}

/*
 * Free block from the ext2 block bitmaps
 * */
void ext2_free_block(ext2_fs_t * ext2fs, uint32_t block) {
    uint32_t * buf = kcalloc(ext2fs->block_size, 1);
    // Which group it belongs to ?
    uint32_t group_idx = block / ext2fs->blocks_per_group;
    // Which sub_bitmap it belongs to ?
    uint32_t sub_bitmap_idx = (block - (ext2fs->blocks_per_group * group_idx)) / 4;
    // Index in sub_bitmap ?
    uint32_t idx = (block - (ext2fs->blocks_per_group * group_idx)) % 4;

    uint32_t bitmap_block = ext2fs->bgds[group_idx].block_bitmap;
    read_disk_block(ext2fs, bitmap_block, (void*)buf);

    // Mask out that inode and write back the bitmap
    uint32_t mask = ~(0x1 << idx);
    buf[sub_bitmap_idx] = buf[sub_bitmap_idx] & mask;

    write_disk_block(ext2fs, bitmap_block, (void*)buf);

    // update free_inodes
    ext2fs->bgds[group_idx].free_blocks++;
    rewrite_bgds(ext2fs);

}

void alloc_inode_block(ext2_fs_t * ext2fs, inode_t * inode, uint32_t inode_idx, uint32_t block) {
    uint32_t ret = ext2_alloc_block(ext2fs);
    set_disk_block_number(ext2fs, inode, inode_idx, block, ret);
    inode->num_sectors = (block + 1) * (ext2fs->block_size / 512);
    write_inode_metadata(ext2fs, inode, inode_idx);
}

void free_inode_block(ext2_fs_t * ext2fs, inode_t * inode, uint32_t inode_idx, uint32_t block) {
    uint32_t ret = get_disk_block_number(ext2fs, inode, block);
    ext2_free_block(ext2fs, ret);
    set_disk_block_number(ext2fs, inode, inode_idx, ret, 0);
    write_inode_metadata(ext2fs, inode, inode_idx);
}

/*
 * Allocate an inode from inode bitmap
 * */
uint32_t alloc_inode(ext2_fs_t * ext2fs) {
    uint32_t * buf = kcalloc(ext2fs->block_size, 1);
    // Read the inode bitmap, find free inode, return its index
    for(uint32_t i = 0; i < ext2fs->total_groups; i++) {
        if(!ext2fs->bgds[i].free_inodes)
            continue;

        uint32_t bitmap_block = ext2fs->bgds[i].inode_bitmap;
        read_disk_block(ext2fs, bitmap_block, (void*)buf);
        for(uint32_t j = 0; j < ext2fs->block_size / 4; j++) {
            uint32_t sub_bitmap = buf[j];
            if(sub_bitmap == 0xFFFFFFFF)
                continue;
            for(uint32_t k = 0; k < 32; k++) {
                uint32_t free = !((sub_bitmap >> k) & 0x1);
                if(free) {
                    // Set bitmap and return
                    uint32_t mask = (0x1 << k);
                    buf[j] = buf[j] | mask;
                    write_disk_block(ext2fs, bitmap_block, (void*)buf);
                    // update free_inodes
                    ext2fs->bgds[i].free_inodes--;
                    rewrite_bgds(ext2fs);
                    return i * ext2fs->inodes_per_group + j * 32 + k;
                }
            }
        }
    }
    PANIC("We're out of inodes!\n");
    return (uint32_t)-1;
}

/*
 * Free an inode from inode bitmap
 * */
void free_inode(ext2_fs_t * ext2fs, uint32_t inode) {
    uint32_t * buf = kcalloc(ext2fs->block_size, 1);
    // Which group it belongs to ?
    uint32_t group_idx = inode / ext2fs->inodes_per_group;
    // Which sub_bitmap it belongs to ?
    uint32_t sub_bitmap_idx = (inode - (ext2fs->inodes_per_group * group_idx)) / 4;
    // Index in sub_bitmap ?
    uint32_t idx = (inode - (ext2fs->inodes_per_group * group_idx)) % 4;

    uint32_t bitmap_block = ext2fs->bgds[group_idx].inode_bitmap;
    read_disk_block(ext2fs, bitmap_block, (void*)buf);

    // Mask out that inode and write back the bitmap
    uint32_t mask = ~(0x1 << idx);
    buf[sub_bitmap_idx] = buf[sub_bitmap_idx] & mask;

    write_disk_block(ext2fs, bitmap_block, (void*)buf);

    // update free_inodes
    ext2fs->bgds[group_idx].free_inodes++;
    rewrite_bgds(ext2fs);
}

/*
 * On a formatted disk, the root node is already created, with inode number = 2
 * This function constructs a vfs_node based on the root inode's information
 * */
vfs_node_t * get_ext2_root(ext2_fs_t * ext2fs, inode_t * inode) {
    vfs_node_t * ext2root = kcalloc(sizeof(vfs_node_t), 1);
    strcpy(ext2root->name, "/");
    ext2root->device = ext2fs;
    ext2root->mask = inode->permission;
    ext2root->inode_num = ROOT_INODE_NUMBER;

    ext2root->access_time   = inode->atime;
    ext2root->modified_time   = inode->mtime;
    ext2root->create_time   = inode->ctime;

    ext2root->flags |= FS_DIRECTORY;
    ext2root->read    = NULL;
    ext2root->write   = NULL;
    ext2root->chmod   = ext2_chmod;
    ext2root->open    = ext2_open;
    ext2root->close   = ext2_close;
    ext2root->read    = ext2_read;
    ext2root->write   = ext2_write;
    ext2root->mkdir   = ext2_mkdir;
    ext2root->create  = ext2_mkfile;
    ext2root->listdir = ext2_listdir;
    ext2root->finddir = ext2_finddir;
    ext2root->unlink  = ext2_unlink;
    return ext2root;
}

/*
 *
 * Then call vfs to mount ext2 onto the vfs tree
 * device_path: which device to use for sector read/write
 * mountpoint: where to mount ext2, we usually mount ext2 to root path "/"
 * */
void ext2_init(char * device_path, char * mountpoint) {
    // First, we need to store some information about the ext2-formatted disk
    ext2_fs_t * ext2fs = kcalloc(sizeof(ext2_fs_t), 1);
    ext2fs->disk_device= file_open(device_path, 0);
    ext2fs->sb = kmalloc(SUPERBLOCK_SIZE);

    // Set a temporary block_size, because we need to call read_disk_block, which requires a block size
    ext2fs->block_size = 1024;

    // Read supedisk_block from disk
    read_disk_block(ext2fs, 1, (void*)ext2fs->sb);
    // Determine some helpful vars
    ext2fs->block_size = (1024 << ext2fs->sb->log2block_size);
    ext2fs->blocks_per_group = ext2fs->sb->blocks_per_group;
    ext2fs->inodes_per_group = ext2fs->sb->inodes_per_group;

    ext2fs->total_groups = ext2fs->sb->total_blocks / ext2fs->blocks_per_group;
    if(ext2fs->blocks_per_group * ext2fs->total_groups < ext2fs->total_groups)
        ext2fs->total_groups++;

    // Now that we know the total number of groups, we can read in the BGD(Block Group Descriptors), it's placed immediately after supedisk_block
    // But how many disk blocks the BGD take?
    ext2fs->bgd_blocks = (ext2fs->total_groups * sizeof(bgd_t)) / ext2fs->block_size;
    if(ext2fs->bgd_blocks * ext2fs->block_size < ext2fs->total_groups * sizeof(bgd_t))
        ext2fs->bgd_blocks++;

    ext2fs->bgds = kcalloc(sizeof(bgd_t), ext2fs->bgd_blocks * ext2fs->block_size);
    for(uint32_t i = 0; i < ext2fs->bgd_blocks; i++) {
        read_disk_block(ext2fs, 2, (void*)ext2fs->bgds + i * ext2fs->block_size);
    }

    // Then, mount it onto the vfs tree
    inode_t * root_inode = kcalloc(sizeof(inode_t), 1);
    read_inode_metadata(ext2fs, root_inode, ROOT_INODE_NUMBER);
    vfs_mount(mountpoint, get_ext2_root(ext2fs, root_inode));
}
