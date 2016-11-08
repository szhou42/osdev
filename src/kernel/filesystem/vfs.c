#include <vfs.h>
#include <ext2.h>
#include <kheap.h>
#include <string.h>
#include <printf.h>
#include <my_errno.h>

gtree_t * vfs_tree;
vfs_node_t * vfs_root;

uint32_t vfs_get_file_size(vfs_node_t * node) {
    if(node && node->get_file_size) {
        return node->get_file_size(node);
    }
    return 0;
}

/*
 * This function is for debug,purpose
 * Given a dir name, it will print out all the sub-directories/files under the dir
 * */
void vfs_db_listdir(char * name) {
    // Open the VFS node, call its list dir function
    vfs_node_t * n = file_open(name, 0);
    if(!n) {
        printf("Could not list a directory that does not exist\n");
        return;
    }
    if(!n->listdir) return;
    char ** files = n->listdir(n);
    char ** save = files;
    while(*files) {
        printf("%s ", *files);
        kfree(*files);
        files++;
    }
    kfree(save);
    printf("\n");
}
/*
 * Recuersively print the vfs tree and show the device mounted on each node
 *
 * For next level, indent offset + strlen(parent path) + 1
 * /(ext2)
 *   dev(empty)
 *       hda(...)
 *       hdb
 *       hdc
 *       hdd
 *
 * */
void print_vfstree_recur(gtreenode_t * node, int parent_offset) {
    if (!node) return;
    //db_print();
    char * tmp = kmalloc(512);
    //db_print();
    int len = 0;
    memset(tmp, 0, 512);
    for (unsigned int i = 0; i < parent_offset; ++i) {
        strcat(tmp, " ");
    }
    char * curr = tmp + strlen(tmp);
    struct vfs_entry * fnode = (struct vfs_entry *)node->value;
    if (fnode->file) {
        sprintf(curr, "%s(0x%x, %s)", fnode->name, (unsigned int)fnode->file, fnode->file->name);
    } else {
        sprintf(curr, "%s(empty)", fnode->name);
    }
    printf("%s\n", tmp);
    len = strlen(fnode->name);
    free(tmp);
    foreach(child, node->children) {
        print_vfstree_recur(child->val, parent_offset + len + 1);
    }
}
/*
 * Wrapper for vfs_db_print_recur
 * */
void print_vfstree() {
    print_vfstree_recur(vfs_tree->root, 0);
}

/*
 * Wrapper for physical filesystem read
 * */

unsigned int vfs_read(vfs_node_t *node, unsigned int offset, unsigned int size, char *buffer) {
    if (node && node->read) {
        unsigned int ret = node->read(node, offset, size, buffer);
        return ret;
    }
    return -1;
}

/*
 * Wrapper for physical filesystem write
 * */
unsigned int vfs_write(vfs_node_t *node, unsigned int offset, unsigned int size, char *buffer) {
    if (node && node->write) {
        unsigned int ret = node->write(node, offset, size, buffer);
        return ret;
    }
    return -1;
}


/*
 * Wrapper for physical filesystem open
 * */
void vfs_open(struct vfs_node *node, unsigned int flags) {
    if(!node) return;
    if(node->refcount >= 0) node->refcount++;
    node->open(node, flags);
}

/*
 * Wrapper for physical filesystem close
 * */
void vfs_close(vfs_node_t *node) {
    if(!node || node == vfs_root || node->refcount == -1) return;
    node->refcount--;
    if(node->refcount == 0)
        node->close(node); // Should I free the node ? No, the caller should do it.
}

/*
 * Wrapper for physical filesystem chmod
 * */
void vfs_chmod(vfs_node_t *node, uint32_t mode) {
    if (node->chmod) return node->chmod(node, mode);
}

/*
 * Wrapper for physical filesystem readdir
 * */
struct dirent *vfs_readdir(vfs_node_t *node, unsigned int index) {
    if(node && (node->flags & FS_DIRECTORY) && node->readdir)
        return node->readdir(node, index);
    return NULL;
}

/*
 * Wrapper for physical filesystem finddir
 * */
vfs_node_t *vfs_finddir(vfs_node_t *node, char *name) {
    if(node && (node->flags & FS_DIRECTORY) && node->finddir)
        return node->finddir(node, name);
    return NULL;
}

/*
 * Wrapper for physical filesystem ioctl
 * */
int vfs_ioctl(vfs_node_t *node, int request, void * argp) {
    if(!node) return -1;
    if(node->ioctl) return node->ioctl(node, request, argp);
    return ENOTTY;
}

/*
 * Wrapper for physical filesystem mkdir
 *
 * */
void vfs_mkdir(char *name, unsigned short permission) {
    // First, extract the parent directory and the directory to be made
    int i = strlen(name);
    char * dirname = strdup(name);
    char * save_dirname = dirname;
    char * parent_path = "/";
    while(i >= 0) {
        if(dirname[i] == '/') {
            if(i != 0) {
                dirname[i] = '\0';
                parent_path = dirname;
            }
            dirname = &dirname[i+1];
            break;
        }
        i--;
    }

    // Second, file_open
    vfs_node_t * parent_node = file_open(parent_path, 0);
    if(!parent_node) {
        kfree(save_dirname);
    }

    // Third, call mkdir
    if(parent_node->mkdir)
        parent_node->mkdir(parent_node, dirname, permission);
    kfree(save_dirname);

    vfs_close(parent_node);
}

/*
 * Wrapper for physical filesystem create file under a directory
 *
 * */

int vfs_create_file(char *name, unsigned short permission) {
    // First, extract the parent directory and the directory to be made
    int i = strlen(name);
    char * dirname = strdup(name);
    char * save_dirname = dirname;
    char * parent_path = "/";
    while(i >= 0) {
        if(dirname[i] == '/') {
            if(i != 0) {
                dirname[i] = '\0';
                parent_path = dirname;
            }
            dirname = &dirname[i+1];
            break;
        }
        i--;
    }

    // Second, file_open
    vfs_node_t * parent_node = file_open(parent_path, 0);
    if(!parent_node) {
        kfree(save_dirname);
        return -1;
    }
    if(parent_node->create)
        parent_node->create(parent_node, dirname, permission);
    kfree(save_dirname);
    vfs_close(parent_node);
    return 0;


}
/*
 * Wrapper for physical filesystem unlink (delete) file under a directory
 *
 * */

void vfs_unlink(char * name) {
    // First, extract the parent directory and the directory to be made
    int i = strlen(name);
    char * dirname = strdup(name);
    char * save_dirname = dirname;
    char * parent_path = "/";
    while(i >= 0) {
        if(dirname[i] == '/') {
            if(i != 0) {
                dirname[i] = '\0';
                parent_path = dirname;
            }
            dirname = &dirname[i+1];
            break;
        }
        i--;
    }

    // Second, file_open
    vfs_node_t * parent_node = file_open(parent_path, 0);
    if(!parent_node) {
        kfree(save_dirname);
    }
    if(parent_node->unlink)
        parent_node->unlink(parent_node, dirname);
    kfree(save_dirname);
    vfs_close(parent_node);
}


/*
 * Parse the path and replace special meaning dir name such as "." and ".."
    For example, /abc/def/.. is converted to /abc

stack:
 .. def abc
for normal token, just pop it out
when .. is seen, pop it and pop one more time
*/
char *expand_path(char *input) {
    // First, push all of them onto a stack
    list_t * input_list = str_split(input, "/", NULL);
    char * ret = list2str(input_list, "/");
    return ret;
}

/*
 * Helper function for file_open
 * Given a filename, return the vfs_node_t of the path on which it's mounted
 * For example, in our case /home/szhou42 is mounted on the vfs_node_t with path "/" (ext2 is mounted for root dir)
 *
 * */
vfs_node_t * get_mountpoint_recur(char ** path, gtreenode_t * subroot) {
    int found = 0;
    char * curr_token = strsep(path, "/");
    // Basecase, not getting any more tokens, stop and return the last one
    if(curr_token == NULL || !strcmp(curr_token, "")) {
        struct vfs_entry * ent = (struct vfs_entry*)subroot->value;
        return ent->file;
    }
    // Find if subroot's children contain any that matches the current token
    foreach(child, subroot->children) {
        gtreenode_t * tchild = (gtreenode_t*)child->val;
        struct vfs_entry * ent = (struct vfs_entry*)(tchild->value);
        if(strcmp(ent->name, curr_token) == 0) {
            found = 1;
            subroot = tchild;
            break;
        }
    }

    if(!found) {
        // This token is not found, make path point to this token so that file_open knows from where to start searching in the physical filesystem
        // In another words, for a path like "/home/szhou42", the vfs tree is only aware of the root path "/", because it's the only thing mounted here
        // For the rest of the path "home/szhou42", file_open have to find them in the physical filesystem(such as ext2)
        *path = curr_token;
        return ((struct vfs_entry*)(subroot->value))->file;
    }
    // Recursion !
    return get_mountpoint_recur(path, subroot);
}
/*
 * Wrapper function for get_mountpoint_recur
 *
 * */
vfs_node_t * get_mountpoint(char ** path) {
    // Adjust input, delete trailing slash
    if(strlen(*path) > 1 && (*path)[strlen(*path) - 1] == '/')
        *(path)[strlen(*path) - 1] = '\0';
    if(!*path || *(path)[0]!= '/') return NULL;
    if(strlen(*path) == 1) {
         // root, clear the path
        *path = '\0';
        struct vfs_entry * ent = (struct vfs_entry*)vfs_tree->root->value;
        return ent->file;

    }
    (*path)++;
    return get_mountpoint_recur(path, vfs_tree->root);
}
/*
 * Given filename, return a vfs_node(Then you can do reading/writing on the file! Pretty much like fopen)
 * */
vfs_node_t *file_open(const char * file_name, unsigned int flags) {
    /* First, find the mountpoint of the file(i.e find which filesystem the file belongs to so that vfs can call the right functions for accessing directory/files)
     Since the vfs tree doesn't store directory tree within a physical filesystem(i.e when ext2 is mounted on /abc, the vfs tree doesn't maintain any sub-directories under /abc),
     we will need to traverse the tree using callback provided by physical filesystem(ext2 for example)
     When it gets to  directory containing the file, simply invoke the callback open provided by the physical filesystem
    */
    char * curr_token = NULL;
    char * filename = strdup(file_name);
    char * free_filename = filename;
    char * save = strdup(filename);
    char * original_filename = filename;
    char * new_start = NULL;
    vfs_node_t * nextnode = NULL;
    vfs_node_t * startpoint = get_mountpoint(&filename);
    if(!startpoint) return NULL;
    if(filename)
        new_start = strstr(save + (filename - original_filename), filename);
    while( filename != NULL  && ((curr_token = strsep(&new_start, "/")) != NULL)) {
        nextnode = vfs_finddir(startpoint, curr_token);
        if(!nextnode) return NULL;
        startpoint = nextnode;
    }
    if(!nextnode)
        nextnode = startpoint;
    vfs_open(nextnode, flags);
    kfree(save);
    kfree(free_filename);
    return nextnode;
}

/*
 * Set up a filesystem tree, for which device and filesystem can be mounted on
 * Set up hashmap, filesysyems can register its initialization callback to the vfs
 * */
void vfs_init() {
    vfs_tree = tree_create();
    struct vfs_entry * root = kmalloc(sizeof(struct vfs_entry));
    root->name = strdup("root");
    root->file = NULL;
    tree_insert(vfs_tree, NULL, root);
}

/*
 * Mounting a device is no different from inserting a plain node onto the tree
 * */
void vfs_mount_dev(char * mountpoint, vfs_node_t * node) {
    vfs_mount(mountpoint, node);
}

/*
 * Helper function for vfs_mount
 * This is simply inserting a node
 * */
void vfs_mount_recur(char * path, gtreenode_t * subroot, vfs_node_t * fs_obj) {
    int found = 0;
    char * curr_token = strsep(&path, "/");

    if(curr_token == NULL || !strcmp(curr_token, "")) {
        // return the subroot, it's where u should mount!
        struct vfs_entry * ent = (struct vfs_entry*)subroot->value;
        if(ent->file) {
            printf("The path is already mounted, plz unmount before mounting again\n");
            return;
        }
        if(!strcmp(ent->name, "/")) vfs_root = fs_obj; // Keep a shortcut for root node
        ent->file = fs_obj;
        return;
    }

    foreach(child, subroot->children) {
        gtreenode_t * tchild = (gtreenode_t*)child->val;
        struct vfs_entry * ent = (struct vfs_entry*)(tchild->value);
        if(strcmp(ent->name, curr_token) == 0) {
            found = 1;
            subroot = tchild;
        }
    }

    if(!found) {
         // Insert the node by myself
         struct vfs_entry * ent = kcalloc(sizeof(struct vfs_entry), 1);
         ent->name = strdup(curr_token);
         subroot = tree_insert(vfs_tree, subroot, ent);
    }
    // Recursion!
    vfs_mount_recur(path, subroot, fs_obj);
}
/* Wrapper function for vfs_mount_recur
 * Simply insert a node to the tree
 * */
void vfs_mount(char * path, vfs_node_t * fs_obj) {
    fs_obj->refcount = -1;
    fs_obj->fs_type= 0;
    if(path[0] == '/' && strlen(path) == 1) {
        struct vfs_entry * ent = (struct vfs_entry*)vfs_tree->root->value;
        if(ent->file) {
            printf("The path is already mounted, plz unmount before mounting again\n");
            return;
        }
        vfs_root = fs_obj; // Keep a shortcut for root node
        ent->file = fs_obj;
        return;

    }
    vfs_mount_recur(path + 1, vfs_tree->root, fs_obj);
}
