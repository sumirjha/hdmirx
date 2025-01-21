// 
// list.h
// cbhandler
// 
// Created by Sumir Kumar Jha on 12/08/19.
// Copyright © 2019 Sumir Kumar Jha. All rights reserved.
// 

#ifndef __LIST_COMMON_H__
#  define __LIST_COMMON_H__

#  include <stdio.h>
#  include <stdbool.h>
#  include <stdint.h>
#  include <stdlib.h>

/**
 * A node structure which link a previous
 * as well as a next node in a chain of
 * nodes
 */

typedef struct list
{

    /** Previous list element */
    struct list *prev;

    /** Next list element */
    struct list *next;
} List_t;

/**
 * @note
 * A wrapper directive which will find
 * the pointer of parent structure by passing \
 * it a pointer of any member.
 *
 * @param ptr pointer to member of the structure
 * @param sample a null pointer of structure type \
 * this will hold the final pointer of needed
 * structure.
 * @param member name of the member, which is
 * pointed by ptr
 */

#  define LIST_CONTAINER_OF(ptr, sample, member)                \
    (void *)((char *)(ptr)    -                        \
    ((char *)&(sample)->member - (char *)(sample)))

/**
 * @note
 * foreach loop, which iterate through each node
 * of list, at each iteration pos will hold the
 * pointer to current node.
 *
 * @param pos pointer to current node in interation
 * @param head pointer to list itself
 * @param member name of the member of list type
 */

#  define LIST_FOR_EACH(pos, head, member)\
    for (pos = LIST_CONTAINER_OF((head)->next, pos, member);  \
        &pos->member != (head); \
        pos = LIST_CONTAINER_OF(pos->member.next, pos, member))

/**
 * @note
 * Same as above, but loop will run in reverse order
 */

#  define LIST_FOR_EACH_REVERSE(pos, head, member) \
    for (pos = LIST_CONTAINER_OF((head)->prev, pos, member);  \
        &pos->member != (head); \
        pos = LIST_CONTAINER_OF(pos->member.prev, pos, member))

#  define LIST_FOR_EACH_SAFE(pos, tmp, head, member)            \
    for (pos = 0, tmp = 0,                         \
         pos = LIST_CONTAINER_OF((head)->next, pos, member),        \
         tmp = LIST_CONTAINER_OF((pos)->member.next, tmp, member);    \
         &pos->member != (head);                    \
         pos = tmp,                            \
         tmp = LIST_CONTAINER_OF(pos->member.next, tmp, member))

#  define LIST_POP_FRONT(ptr, head, member) \
    List_t *node = listRemoveFront(head);\
    if ( node != NULL) { \
        ptr = LIST_CONTAINER_OF(node, ptr, member); \
    }

#  define LIST_PEEK_FRONT(ptr, head, member) \
    List_t *node = listPeekFront(head);\
    if ( node != NULL) { \
        ptr = LIST_CONTAINER_OF(node, ptr, member); \
    }

#  define LIST_POP_BACK(ptr, head, member)          \
    List_t *node = listRemoveBack(head);            \
    ptr = NULL;                                     \
    if ( node != NULL) {                            \
        ptr = LIST_CONTAINER_OF(node, ptr, member); \
    }

// --------- Some list manupulation function --------

/**
 * @note
 * Initialize the list, this function must be called first
 * and once at the time of creation of list.
 * @param list pointer to list
 */
void listInit(List_t *list);

/**
 * @note
 * Insert a node into the list
 * @param list pointer to list
 * @param elm pointer of node to be inserted
 */
void listInsert(List_t *list, List_t *elm);

void listInsertFront(List_t *list, List_t *elm);

void listInsertBack(List_t *list, List_t *elm);

List_t *listRemoveFront(List_t *list);

List_t * listPeekFront(List_t *list);

List_t *listRemoveBack(List_t *list);

/**
 * @note
 * Remove a node from the list
 * @param elm pointer of node to be removed
 */
void listRemove(List_t *elm);

/**
 * @note
 * Check if list is empty or not
 * @param list pointer to list
 * @return True if empty otherwise False
 */
bool listEmpty(List_t *list);

/**
 * @note
 * Find length of list
 * @param list pointer to list
 * @return Number of elements in the list
 */
int listLength(List_t *list);

#endif // __LIST_COMMON_H__
