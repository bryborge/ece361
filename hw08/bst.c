/*
 * bst.c:
 *
 * A binary search tree of ints, ported from TutorialsPoint's
 * BinarySearchTree.c. The original keeps one global tree; here each tree is a
 * bst_t, and the node struct keeps the original's names.
 */
#include <stdlib.h>

#include "bst.h"

/* From the original's struct node, with leftChild and rightChild shortened. */
struct node {
    int data;
    struct node *left, *right;
};

/* Replaces the original's global root. */
struct bst {
    struct node *root;
};

/* From the original's `struct node *root = NULL;`, made per tree. */
bst_t *bst_create(void) {
    bst_t *t = malloc(sizeof *t);

    if (t == NULL) return NULL;
    t->root = NULL;

    return t;
}

/*
 * New: the original never frees. Frees both subtrees before the node itself,
 * since their pointers live inside it. n must not be NULL.
 */
static void free_subtree(struct node *n) {
    if (n->left != NULL) free_subtree(n->left);
    if (n->right != NULL) free_subtree(n->right);

    free(n);
}

/* New: the original never frees. */
void bst_destroy(bst_t *t) {
    if (t == NULL) return;
    if (t->root != NULL) free_subtree(t->root);

    free(t);
}

/*
 * From the original's insert. Allocation is checked before the tree is
 * touched, so a failure leaves it unchanged.
 */
int bst_insert(bst_t *t, int key) {
    if (t == NULL) return -1;

    struct node *tempNode = malloc(sizeof *tempNode);
    if (tempNode == NULL) return -1;
    tempNode->data = key;
    tempNode->left = NULL;
    tempNode->right = NULL;

    if (t->root == NULL) {
        t->root = tempNode;
        return 0;
    }

    struct node *current = t->root;
    struct node *parent;

    while (1) {
        parent = current;

        if (key < parent->data) {
            current = current->left;
            if (current == NULL) {
                parent->left = tempNode;
                return 0;
            }
        } else {
            current = current->right;
            if (current == NULL) {
                parent->right = tempNode;
                return 0;
            }
        }
    }
}

/* New: the original has no size. An empty subtree holds no keys. */
static size_t size_subtree(const struct node *n) {
    if (n == NULL) return 0;

    return 1 + size_subtree(n->left) + size_subtree(n->right);
}

/* New: the original has no size. */
size_t bst_size(const bst_t *t) {
    if (t == NULL) return 0;

    return size_subtree(t->root);
}

/* New: the original has no height. An empty subtree has height 0. */
static size_t height_subtree(const struct node *n) {
    if (n == NULL) return 0;

    size_t left = height_subtree(n->left);
    size_t right = height_subtree(n->right);

    return 1 + (left > right ? left : right);
}

/* New: the original has no height. */
size_t bst_height(const bst_t *t) {
    if (t == NULL) return 0;

    return height_subtree(t->root);
}

/*
 * From the original's search, ported faithfully, loop and all, but without
 * the printing (and the NULL check that only guarded a printf). A visit is
 * one node whose data is compared with key: one per pass through the loop,
 * plus the node that ends it by matching.
 */
bool bst_contains(const bst_t *t, int key, size_t *visits) {
    size_t count = 0;

    if (t == NULL) {
        if (visits != NULL) *visits = 0;
        return false;
    }

    const struct node *current = t->root;

    while (current->data != key) {
        count++;

        if (current->data > key) {
            current = current->left;
        } else {
            current = current->right;
        }

        if (current == NULL) {
            if (visits != NULL) *visits = count;
            return false;
        }
    }
    count++;

    if (visits != NULL) *visits = count;
    return true;
}
