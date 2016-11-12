#include <generic_tree.h>

/*
 * Create a tree with root = NULL
 * */
gtree_t * tree_create() {
    return (gtree_t*)kcalloc(sizeof(gtree_t), 1);
}

/*
 * Create a tree node with specified value, and a list of 0 children
 * */
gtreenode_t * treenode_create(void * value) {
    gtreenode_t * n = kcalloc(sizeof(gtreenode_t), 1);
    n->value = value;
    n->children = list_create();
    return n;
}

/*
 * Insert a node under subroot
 * */
gtreenode_t * tree_insert(gtree_t * tree, gtreenode_t * subroot, void * value) {
    // Create a treenode
    gtreenode_t * treenode = kcalloc(sizeof(gtreenode_t), 1);
    treenode->children = list_create();
    treenode->value = value;

    // Insert it
    if(!tree->root) {
        tree->root = treenode;
        return treenode;
    }
    list_insert_front(subroot->children, treenode);
    return treenode;
}

gtreenode_t * tree_find_parent(gtree_t * tree, gtreenode_t * remove_node, int * child_index) {
    // If subroot is the parent
    if(remove_node == tree->root) return NULL;
    return tree_find_parent_recur(tree, remove_node, tree->root, child_index);
}

gtreenode_t * tree_find_parent_recur(gtree_t * tree, gtreenode_t * remove_node, gtreenode_t * subroot, int * child_index) {
    int idx;
    if((idx = list_contain(subroot->children, remove_node)) != -1) {
        *child_index = idx;
        return subroot;
    }
    foreach(child, subroot->children) {
        gtreenode_t * ret = tree_find_parent_recur(tree, remove_node, child->val, child_index);
        if(ret != NULL) {
            return ret;
        }
        // ret is NULL, keep searching.
    }
    return NULL;
}

void tree_remove(gtree_t * tree, gtreenode_t * remove_node) {
    // Search for tree's parent and remove the node from parent's children list
    // If parent is NULL, then just set tree->root to NULL(yeah, I dont care about mem leaks)
    int child_index = -1;
    gtreenode_t * parent = tree_find_parent(tree, remove_node, &child_index);
    // Do treenode remove in here:
    if(parent != NULL) {
        list_remove_by_index(parent->children, child_index);
    }
}


