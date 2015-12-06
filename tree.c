/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <kevinmottashed@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * -Kevin Mottashed
 * ----------------------------------------------------------------------------
 */

#include "tree.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

void tree_init(struct tree_head * tree)
{
  tree->parent = tree;
  list_init(&tree->children);
  list_init(&tree->siblings);
}

void tree_add_child(struct tree_head * node, struct tree_head * child)
{
  child->parent = node;
  list_push_back(&node->children, &child->siblings);
}

void tree_remove(struct tree_head * node)
{
  assert(node != NULL);

  if (node->parent == node)
  {
    // The node is at the root of the tree.
    // Every child will become the root node in their own trees.
    struct list_head * it;
    list_for_each(it, &node->children)
    {
      struct tree_head * child = container_of(it, struct tree_head, siblings);
      child->parent = child;
    }
  }
  else
  {
    // The node is in the middle of the tree.
    // Every child will inherit our parent.
    struct list_head * it;
    list_for_each(it, &node->children)
    {
      struct tree_head * child = container_of(it, struct tree_head, siblings);
      child->parent = node->parent;
    }

    // Our children become our parent's children
    list_append(&node->parent->children, &node->children);
  }

  // Remove ourselves from our parent's children
  list_remove(&node->siblings);
}

void tree_remove_child(struct tree_head * node)
{
  assert(node != NULL);

  // Become a root node.
  node->parent = node;

  // Remove ourselves from our parent's children
  list_remove(&node->siblings);
}

bool tree_no_children(struct tree_head * node)
{
  assert(node != NULL);
  return list_empty(&node->children);
}

uint32_t tree_num_children(struct tree_head * node)
{
  assert(node != NULL);
  return list_size(&node->children);
}

struct tree_head * tree_first_child(struct tree_head * node)
{
  return container_of(list_front(&node->children), struct tree_head, siblings);
}
