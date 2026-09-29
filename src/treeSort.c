/* Tree sort: insert every element into a binary search tree (BST), then read
 * the tree back with an in-order traversal (left, node, right).
 *
 * This is the algorithm that was NOT covered in class. Its behaviour depends
 * entirely on the shape of the tree:
 *
 *   random input      -> tree height about 2 to 3 * log2(n) -> O(n log n)
 *   sorted / reversed -> every new key goes to the same side, the tree is a
 *                        linked list of height n              -> O(n^2)
 *
 * Implementation notes
 *
 *  - Nodes are not separate mallocs. Element i IS node i; two index arrays
 *    (left[], right[]) hold the children. This keeps the extra memory to
 *    exactly 2 * n * sizeof(size_t) and avoids thousands of small allocations.
 *
 *  - Stability: a key EQUAL to the current node goes to the right subtree.
 *    Equal keys therefore appear in the in-order walk in insertion order, so
 *    tree sort is stable (bench.c measures this, it is not just a claim).
 *
 *  - Insertion and traversal are both iterative. A degenerate tree has height
 *    n, and a recursive traversal would overflow the call stack for large n.
 *    The explicit traversal stack is allocated with exactly `height` slots.
 *
 *  - The sorted output is written to a buffer of n elements and copied back,
 *    so each element moves exactly twice: 2n moves, independent of the input.
 *
 * treeSortBalanced() is the same idea on an AVL tree (a self-balancing BST).
 * Rotations keep the height <= 1.44 * log2(n), which removes the O(n^2) worst
 * case. It is only used in the extra experiment (main.c --tree).
 */
#include "sort.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sortctx.h"

#define NIL SIZE_MAX

typedef struct Tree {
    SortCtx c;
    size_t *left;
    size_t *right;
    size_t root;
    size_t height; /* number of nodes on the longest root-to-leaf path */
} Tree;

static int treeAlloc(Tree *t, size_t n) {
    t->left = (size_t *)malloc(n * sizeof(size_t));
    t->right = (size_t *)malloc(n * sizeof(size_t));
    if (t->left == NULL || t->right == NULL) {
        free(t->left);
        free(t->right);
        return 0;
    }
    for (size_t i = 0; i < n; i++) {
        t->left[i] = NIL;
        t->right[i] = NIL;
    }
    sortAddExtra(&t->c, 2 * n * sizeof(size_t));
    t->root = NIL;
    t->height = 0;
    return 1;
}

static void treeFree(Tree *t) {
    free(t->left);
    free(t->right);
}

/* In-order traversal: writes the elements in sorted order into out[], then
 * copies out[] back over the input. Returns 0 if memory ran out. */
static int treeWriteBack(Tree *t, size_t n) {
    SortCtx *c = &t->c;
    size_t *stack = (size_t *)malloc((t->height + 1) * sizeof(size_t));
    char *out = (char *)malloc(n * c->size);
    if (stack == NULL || out == NULL) {
        free(stack);
        free(out);
        return 0;
    }
    sortAddExtra(c, (t->height + 1) * sizeof(size_t) + n * c->size);

    size_t top = 0;
    size_t k = 0;
    size_t node = t->root;
    while (node != NIL || top > 0) {
        while (node != NIL) { /* go as far left as possible */
            stack[top++] = node;
            node = t->left[node];
        }
        node = stack[--top];
        sortMove(c, out + k * c->size, sortElemAt(c, node));
        k++;
        node = t->right[node];
    }
    for (size_t i = 0; i < n; i++) {
        sortMove(c, sortElemAt(c, i), out + i * c->size);
    }
    free(stack);
    free(out);
    return 1;
}

/* --- plain (unbalanced) BST: the tree sort compared in the report -------- */

static void bstInsert(Tree *t, size_t i) {
    if (t->root == NIL) {
        t->root = i;
        t->height = 1;
        return;
    }
    size_t node = t->root;
    size_t depth = 1;
    for (;;) {
        depth++;
        /* Strictly smaller goes left; equal goes right (keeps it stable). */
        size_t *next = (sortCompareAt(&t->c, i, node) < 0) ? &t->left[node]
                                                           : &t->right[node];
        if (*next == NIL) {
            *next = i;
            break;
        }
        node = *next;
    }
    if (depth > t->height) {
        t->height = depth;
    }
}

void treeSort(void *base, size_t n, size_t size, SortCompare cmp, SortStats *stats) {
    Tree t;
    if (!sortBegin(&t.c, base, n, size, cmp, stats)) {
        return;
    }
    if (treeAlloc(&t, n)) {
        for (size_t i = 0; i < n; i++) {
            bstInsert(&t, i);
        }
        sortNoteDepth(&t.c, t.height);
        treeWriteBack(&t, n);
        treeFree(&t);
    }
    sortEnd(&t.c);
}

/* --- AVL tree: self-balancing extension ---------------------------------- */

typedef struct Avl {
    Tree t;
    unsigned char *h; /* subtree height of each node (fits: <= 1.44 log2 n) */
} Avl;

static unsigned char avlH(const Avl *a, size_t node) {
    return node == NIL ? 0 : a->h[node];
}

static void avlFix(Avl *a, size_t node) {
    unsigned char l = avlH(a, a->t.left[node]);
    unsigned char r = avlH(a, a->t.right[node]);
    a->h[node] = (unsigned char)((l > r ? l : r) + 1);
}

static size_t avlRotateRight(Avl *a, size_t y) {
    size_t x = a->t.left[y];
    a->t.left[y] = a->t.right[x];
    a->t.right[x] = y;
    avlFix(a, y);
    avlFix(a, x);
    return x;
}

static size_t avlRotateLeft(Avl *a, size_t x) {
    size_t y = a->t.right[x];
    a->t.right[x] = a->t.left[y];
    a->t.left[y] = x;
    avlFix(a, x);
    avlFix(a, y);
    return y;
}

/* Recursive insert. Recursion depth is the AVL height, i.e. O(log n).
 * Rotations never change the in-order sequence, so stability is kept. */
static size_t avlInsert(Avl *a, size_t node, size_t i, size_t depth) {
    sortNoteDepth(&a->t.c, depth);
    if (node == NIL) {
        a->h[i] = 1;
        return i;
    }
    if (sortCompareAt(&a->t.c, i, node) < 0) {
        a->t.left[node] = avlInsert(a, a->t.left[node], i, depth + 1);
    } else {
        a->t.right[node] = avlInsert(a, a->t.right[node], i, depth + 1);
    }
    avlFix(a, node);

    int balance = (int)avlH(a, a->t.left[node]) - (int)avlH(a, a->t.right[node]);
    if (balance > 1) {
        size_t l = a->t.left[node];
        if (avlH(a, a->t.left[l]) < avlH(a, a->t.right[l])) {
            a->t.left[node] = avlRotateLeft(a, l); /* left-right case */
        }
        return avlRotateRight(a, node);
    }
    if (balance < -1) {
        size_t r = a->t.right[node];
        if (avlH(a, a->t.right[r]) < avlH(a, a->t.left[r])) {
            a->t.right[node] = avlRotateRight(a, r); /* right-left case */
        }
        return avlRotateLeft(a, node);
    }
    return node;
}

void treeSortBalanced(void *base, size_t n, size_t size, SortCompare cmp, SortStats *stats) {
    Avl a;
    if (!sortBegin(&a.t.c, base, n, size, cmp, stats)) {
        return;
    }
    a.h = (unsigned char *)malloc(n);
    if (a.h != NULL && treeAlloc(&a.t, n)) {
        sortAddExtra(&a.t.c, n);
        for (size_t i = 0; i < n; i++) {
            a.t.root = avlInsert(&a, a.t.root, i, 1);
        }
        a.t.height = avlH(&a, a.t.root);
        treeWriteBack(&a.t, n);
        treeFree(&a.t);
    }
    free(a.h);
    sortEnd(&a.t.c);
}
