/*
 * bst.c:
 *
 * A binary search tree of ints, ported from TutorialsPoint's
 * BinarySearchTree.c. The original keeps one global tree; here each tree is a
 * bst_t.
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
 * New: The original never frees. Frees both subtrees before the node itself,
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
 * From the original's insert, with its loop replaced by recursion. Places
 * tempNode in the subtree rooted at n, and returns that subtree's root:
 * tempNode itself if the subtree was empty, otherwise n.
 */
static struct node *insert_subtree(struct node *n, struct node *tempNode) {
    if (n == NULL) return tempNode;

    if (tempNode->data < n->data) {
        n->left = insert_subtree(n->left, tempNode);
    } else {
        n->right = insert_subtree(n->right, tempNode);
    }

    return n;
}

/* From the original's insert. */
int bst_insert(bst_t *t, int key) {
    if (t == NULL) return -1;

    struct node *tempNode = malloc(sizeof *tempNode);
    if (tempNode == NULL) return -1;
    tempNode->data = key;
    tempNode->left = NULL;
    tempNode->right = NULL;

    t->root = insert_subtree(t->root, tempNode);

    return 0;
}

/* New: the original has no size. */
static size_t size_subtree(const struct node *n) {
    if (n == NULL) return 0;

    return 1 + size_subtree(n->left) + size_subtree(n->right);
}

/* New: the original has no size. */
size_t bst_size(const bst_t *t) {
    if (t == NULL) return 0;

    return size_subtree(t->root);
}

/* New: the original has no height. */
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
 * From the original's search, without the printing (and the NULL check that
 * only guarded a printf).
 *
 * Fixed: the original read root->data before checking root for NULL, so
 *        searching an empty tree crashed.
 */
bool bst_contains(const bst_t *t, int key, size_t *visits) {
    size_t count = 0;

    if (t == NULL || t->root == NULL) {
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

/*
 * Removes the first node holding key that a search from n reaches, and
 * returns the subtree's new root. key must be in the subtree.
 */
static struct node *remove_subtree(struct node *n, int key) {
    if (key < n->data) {
        n->left = remove_subtree(n->left, key);
        return n;
    }
    if (key > n->data) {
        n->right = remove_subtree(n->right, key);
        return n;
    }

    /* Two children: take the smallest key on the right, then remove its node. */
    if (n->left != NULL && n->right != NULL) {
        struct node *min = n->right;
        while (min->left != NULL) min = min->left;

        n->data = min->data;
        n->right = remove_subtree(n->right, n->data);
        return n;
    }

    /* No children, or one: the child, if any, takes n's place. */
    struct node *child = (n->left != NULL) ? n->left : n->right;
    free(n);

    return child;
}

/* New: the original has no remove. */
int bst_remove(bst_t *t, int key) {
    if (!bst_contains(t, key, NULL)) return -1;
    t->root = remove_subtree(t->root, key);

    return 0;
}
