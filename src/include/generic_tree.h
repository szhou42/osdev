#ifndef GENERIC_TREE_H
#define GENERIC_TREE_H
#include <system.h>
#include <list.h>

typedef struct gtreenode {
    list_t * children;
    void * value;
}gtreenode_t;

typedef struct gtree {
    gtreenode_t * root;
}gtree_t;

gtree_t * tree_create();

gtreenode_t * treenode_create(void * value);

gtreenode_t * tree_insert(gtree_t * tree, gtreenode_t * subroot, void * value);

#endif
