/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <kevinmottashed@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * -Kevin Mottashed
 * ----------------------------------------------------------------------------
 */

#include "list.h"

#include <stddef.h>
#include <assert.h>

void list_init(struct list_head * head)
{
  head->next = head;
  head->prev = head;
}

void list_push_front(struct list_head * head, struct list_head * node)
{
  assert(head != NULL);
  assert(head->next != NULL);
  assert(head->prev != NULL);
  assert(node != NULL);

  node->next = head->next;
  node->prev = head;
  head->next->prev = node;
  head->next = node;
}

void list_push_back(struct list_head * head, struct list_head * node)
{
  assert(head != NULL);
  assert(head->next != NULL);
  assert(head->prev != NULL);
  assert(node != NULL);

  node->next = head;
  node->prev = head->prev;
  head->prev->next = node;
  head->prev = node;
}

struct list_head * list_pop_front(struct list_head * head)
{
  assert(head);
  assert(head->next);
  struct list_head * result = list_front(head);
  if (result != NULL)
    list_remove(result);
  return result;
}

void list_remove(struct list_head * node)
{
  assert(node != NULL);
  assert(node->next != NULL);
  assert(node->prev != NULL);

  node->prev->next = node->next;
  node->next->prev = node->prev;
  node->next = node;
  node->prev = node;
}

void list_append(struct list_head * head, struct list_head * list)
{
  assert(head != NULL);
  assert(list != NULL);
  assert(head->prev != NULL);
  assert(list->next != NULL);

  if (list_empty(list))
  {
  }
  else if (list_empty(head))
  {
    head->next = list->next;
    head->prev = list->prev;
  }
  else
  {
    head->prev->next = list->next;
    list->next->prev = head->prev;
    head->prev = list->prev;
  }
}

void list_insert(struct list_head * position, struct list_head * node)
{
  list_push_front(position, node);
}

bool list_empty(struct list_head * head)
{
  assert(head != NULL);
  return head->next == head;
}

uint32_t list_size(struct list_head * head)
{
  uint32_t result = 0;
  struct list_head * node;
  list_for_each(node, head)
  {
    ++result;
  }
  return result;
}

struct list_head * list_front(struct list_head * head)
{
  assert(head);
  assert(head->next);
  if (head->next == head)
    return NULL;
  return head->next;
}

bool list_contains(struct list_head * head, struct list_head * node)
{
  struct list_head * it;
  list_for_each(it, head)
  {
    if (it == node)
    {
      return true;
    }
  }
  return false;
}
