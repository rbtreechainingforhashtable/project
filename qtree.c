#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "qtree.h"
#define BLACK 1
#define RED 2

node_t* node_make(double key, uint64_t hash_sum, uint64_t data, int color, node_t *parent, uint64_t count) {
    node_t *node = calloc(1, sizeof(*node));
    node->key = key;
    node->sum = hash_sum;
    node->data = data;
    node->color = color;
    node->parent = parent;
    node->count = count;

    return node;
}

int cmp_function(double expect, double key) {
    int rc;

    if (key > expect)
        return 1;

    if (key < expect)
        return -1;

    return 0;
}

void node_foreach_print(node_t *node, int indent) {
    if (!node)
        return;

    ++indent;
#if 0
    printf("node %p->%s, right %p->%s\n", node, node->key, node->right, node->right ? node->right->key : "NULL");
#endif
    node_foreach_print(node->right, indent);
    for (int i = 0; i < indent; ++i) printf("  ");
    printf("(%p)index='%lf', data='%s', color=%s (left:%p) (right:%p)\n", node, node->key, (char*)node->data, node->color == RED ? "\033[0;31mRED\033[0m" : "\033[0;30mBLACK\033[0m", node->left, node->right);
#if 0
    printf("node %p, left %p\n", node, node->left);
#endif
    node_foreach_print(node->left, indent);
}

void tree_print(tree_t *tree) {
    node_foreach_print(tree->root, 0);
}

void left_rotate(tree_t *tree, node_t *node) {
    node_t *branch = node->right;
    node->right = branch->left;
    if (branch->left) {
        branch->left->parent = node;
    }
    branch->parent = node->parent;
    if (!node->parent) {
        tree->root = branch;
    }
    else if (node == node->parent->left) {
        node->parent->left = branch;
    }
    else {
        node->parent->right = branch;
    }
    branch->left = node;
    node->parent = branch;

    branch->count = node->count;
    if (branch->right)
        node->count -= branch->right->count;
}

void right_rotate(tree_t *tree, node_t *node) {
    node_t *branch = node->left;
    node->left = branch->right;
    if (branch->right) {
        branch->right->parent = node;
    }
    branch->parent = node->parent;
    if (!node->parent) {
        tree->root = branch;
    }
    else if (node == node->parent->right) {
        node->parent->right = branch;
    }
    else {
        node->parent->left = branch;
    }
    branch->right = node;
    node->parent = branch;

    branch->count = node->count;
    if (branch->left)
        node->count -= branch->left->count;
}

void insertion_fixup(tree_t *tree, node_t *node) {
    if (!node || !node->parent)
        return;

    while(node->parent->color == RED) {
        if(node->parent == node->parent->parent->left) {

            node_t *gp_branch = node->parent->parent->right;

            if(gp_branch && gp_branch->color == RED) {
                node->parent->color = BLACK;
                gp_branch->color = BLACK;
                node->parent->parent->color = RED;
                node = node->parent->parent;
            }
            else {
                if(node == node->parent->right) {
                    node = node->parent;
                    left_rotate(tree, node);
                }
                node->parent->color = BLACK;
                node->parent->parent->color = RED;
                right_rotate(tree, node->parent->parent);
            }
        }
        else {
            node_t *gp_branch = node->parent->parent->left;

            if(gp_branch && gp_branch->color == RED) {
                node->parent->color = BLACK;
                gp_branch->color = BLACK;
                node->parent->parent->color = RED;
                node = node->parent->parent;
            }
            else {
                if(node == node->parent->left) {
                    node = node->parent;
                    right_rotate(tree, node);
                }
                node->parent->color = BLACK;
                node->parent->parent->color = RED;
                left_rotate(tree, node->parent->parent);
            }
        }

        if (!node->parent)
            break;
    }
    tree->root->color = BLACK;
}

node_t *tree_insert(tree_t *tree, double key, uint64_t hash_sum, uint64_t value, uint8_t tree_height) {
    node_t *node = tree->root;
    node_t *new = NULL;
    if (!node)
    {
        tree->root = new = node_make(key, hash_sum, value, BLACK, NULL, value);
        tree->count += value;
    }

    uint64_t i = 0;
    while (node) {
        if (i == tree_height) {
            tree->count += value;
            return node;
        }

        ++i;
        int rc = cmp_function(node->key, key);
        node->count += value;
        if (rc > 0)
            if (node->right)
                node = node->right;
            else
            {
                node->right = new = node_make(key, hash_sum, value, RED, node, value);
                tree->count += value;
                insertion_fixup(tree, new);
                return new;
            }
        else if (rc < 0)
            if (node->left)
                node = node->left;
            else
            {
                node->left = new = node_make(key, hash_sum, value, RED, node, value);
                tree->count += value;
                insertion_fixup(tree, new);
                return new;
            }
        else {
            tree->count += value;
            return node;
        }
    }

    return new;
}

void rb_delete_fixup(tree_t *tree, node_t *node) {
    while(node && node != tree->root && node->color == BLACK) {
        if (!node->parent)
            break;

        if(node == node->parent->left) {
            node_t *p_branch = node->parent->right;
            if (!p_branch)
                break;

            if(p_branch->color == RED) {
                p_branch->color = BLACK;
                node->parent->color = RED;
                left_rotate(tree, node->parent);
                p_branch = node->parent->right;
            }
            if(p_branch->left && p_branch->left->color == BLACK && p_branch->right && p_branch->right->color == BLACK) {
                p_branch->color = RED;
                node = node->parent;
            }
            else {
                if(p_branch->left && p_branch->right->color == BLACK) {
                    p_branch->left->color = BLACK;
                    p_branch->color = RED;
                    right_rotate(tree, p_branch);
                    p_branch = node->parent->right;
                }
                if (!p_branch->left)
                    break;

                p_branch->color = node->parent->color;
                node->parent->color = BLACK;
                p_branch->right->color = BLACK;
                left_rotate(tree, node->parent);
                node = tree->root;
            }
        }
        else {
            node_t *p_branch = node->parent->left;
            if (!p_branch)
                break;

            if(p_branch->color == RED) {
                p_branch->color = BLACK;
                node->parent->color = RED;
                right_rotate(tree, node->parent);
                p_branch = node->parent->left;
            }
            if(p_branch->right && p_branch->right->color == BLACK && p_branch->left && p_branch->left->color == BLACK) {
                p_branch->color = RED;
                node = node->parent;
            }
            else {
                if(p_branch->left && p_branch->right && p_branch->left->color == BLACK) {
                    p_branch->right->color = BLACK;
                    p_branch->color = RED;
                    left_rotate(tree, p_branch);
                    p_branch = node->parent->left;
                }
                if (!p_branch->left)
                    break;

                p_branch->color = node->parent->color;
                node->parent->color = BLACK;
                p_branch->left->color = BLACK;
                right_rotate(tree, node->parent);
                node = tree->root;
            }
        }
    }

    if (node)
        node->color = BLACK;
}


node_t* rb_transfer(tree_t *tree, node_t *node, node_t *branch) {
    if (!node->parent)
        tree->root = branch;
    else if (node == node->parent->left) {
        node->parent->left = branch;
    }
    else if (node == node->parent->right) {
        node->parent->right = branch;
    }

    if (branch) {
        branch->parent = node->parent;
    }

    return node;
}

node_t* minimum(node_t *node) {
    while (node && node->left)
        node = node->left;
    return node;
}

double getkey(node_t *node) {
    return node ? node->key : 0.0;
}

void rb_count_fixup(tree_t *tree, node_t *node) {
    node_t *cur = node;

    while (cur != tree->root) {
        if (cur->left)
            cur->count = cur->left->count;
        if (cur->right)
            cur->count = cur->right->count;

        cur = node->parent;
    }
}

node_t* rb_delete(tree_t *tree, node_t *node) {
    if (!node)
        return NULL;

    --node->count;
    if (node->count > 0)
        return node;

    node_t *y = node;
    node_t *x;
    int y_orignal_color = y->color;
    node_t* ret = node;

    if (!node->left) {
        x = node->right;
        ret = rb_transfer(tree, node, x);
    }
    else if (!node->right) {
        x = node->left;
        ret = rb_transfer(tree, node, x);
    }
    else {
        y = minimum(node->right);
        y_orignal_color = y->color;
        x = y->right;

        if (y->parent == node) {
            ret = rb_transfer(tree, node, y);
            y->left = node->left;
            node->left->parent = y;
        } else {
            // x block
            node_t *y_parent = y->parent;
            y_parent->left = x;

            if (x)
            {
                x->parent = y_parent;
            }

            y->right = node->right;

            // y block
            ret = rb_transfer(tree, node, y);
            y->left = node->left;
            y->right = node->right;
            y->parent = node->parent;
            node->left->parent = y;
            node->right->parent = y;

            y->color = node->color;
        }
    }

    if(y_orignal_color == BLACK)
        rb_delete_fixup(tree, x);

    rb_count_fixup(tree, x);
    --tree->count;
    return ret;
}

int mark_to_delete(tree_t *tree, node_t *node) {
    if(rb_delete(tree, node))
        return 1;
    else
        return 0;
}

int node_foreach_mark(tree_t *tree, node_t *node) {
    if (!node)
        return 0;

    int ret = 0;
    ret += mark_to_delete(tree, node);
    ret += node_foreach_mark(tree, node->left);
    ret += node_foreach_mark(tree, node->right);

    return ret;
}

void tree_foreach_node(node_t *node, void (*callback)(node_t*, void*, void*, void*), void *arg, void *arg2, void *arg3) {
    if (!node)
        return;

    tree_foreach_node(node->right, callback, arg, arg2, arg3);
    callback(node, arg, arg2, arg3);
    tree_foreach_node(node->left, callback, arg, arg2, arg3);
}

node_t* tree_get(tree_t *tree, double key) {
    node_t *node = tree->root;

    while (node) {
        int rc = cmp_function(node->key, key);
        if (rc > 0)
            node = node->right;
        else if (rc < 0)
            node = node->left;
        else
            return node;
    }

    return node;
}

node_t* tree_quantile(tree_t *tree, double quantile) {
    node_t *node = tree->root;
    if (!node)
        return node;

    uint64_t sum;
    if (node->left)
        sum = node->left->count;
    else if (node->right)
        sum = node->right->count;
    else
    {
        return node;
    }

    uint64_t searchIndex = floor(quantile * tree->count);
    while (sum != searchIndex) {
        int rc = sum < searchIndex;
        if (rc && node->right)
            node = node->right;
        else if (node->left)
            node = node->left;
        else
            break;
    }

    return node;
}

void run_del(tree_t *tree, double key) {
    node_t *node = rb_delete(tree, tree_get(tree, key));
    free(node);
}
