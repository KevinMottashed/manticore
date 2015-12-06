/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <kevinmottashed@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * -Kevin Mottashed
 * ----------------------------------------------------------------------------
 */

#ifndef TREE_H
#define TREE_H

#include "list.h"

struct tree_head
{
  struct tree_head * parent;
  struct list_head children;
  struct list_head siblings;
};

void tree_init(struct tree_head * tree);

// Add a child to the node.
void tree_add_child(struct tree_head * node, struct tree_head * child);

// The node is removed from the tree.
// All the node's children become the parent's children.
// WARNING: not tested
void tree_remove(struct tree_head * node);

// The node loses its parent and becomes the root node in a new tree.
// The node keeps all its children.
void tree_remove_child(struct tree_head * node);

// Returns true if the node has no children.
bool tree_no_children(struct tree_head * node);

// Returns the number of children that the node has.
uint32_t tree_num_children(struct tree_head * node);

// Returns the node first child.
struct tree_head * tree_first_child(struct tree_head * node);

// Iterate over each direct child of a node.
// It's not safe to delete children while iterating.
#define tree_for_each_direct(it, tree)\
  struct tree_head * UNIQUE_VAR(next);\
  for (it = container_of((tree)->children.next, struct tree_head, siblings),\
       UNIQUE_VAR(next) = container_of((tree)->children.next, struct tree_head, children);\
       UNIQUE_VAR(next) != (tree);\
       UNIQUE_VAR(next) = container_of(it->siblings.next, struct tree_head, children),\
       it = container_of(it->siblings.next, struct tree_head, siblings))

#endif
