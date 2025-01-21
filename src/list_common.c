// 
// list.c
// Example of List
// 
// Created by Sumir Kumar Jha on 12/08/19.
// Copyright © 2019 Sumir Kumar Jha. All rights reserved.
// 

#include "list_common.h"

void
listInit(List_t *list)
{
    list->next = list;
    list->prev = list;
}

void
listInsert(List_t *list, List_t *elm)
{
    elm->prev = list;
    elm->next = list->next;
    list->next->prev = elm;
    list->next = elm;
}

void
listRemove(List_t *elm)
{
    elm->next->prev = elm->prev;
    elm->prev->next = elm->next;
    elm->next = NULL;
    elm->prev = NULL;
}

void
listInsertFront(List_t *list, List_t *elm)
{
    listInsert(list, elm);
}

void
listInsertBack(List_t *list, List_t *elm)
{
    elm->next = list;
    elm->prev = list->prev;
    list->prev->next = elm;
    list->prev = elm;
}

List_t *
listRemoveFront(List_t *list)
{
    if (listLength(list) <= 0) return NULL;

    List_t *elm = list->next;

    listRemove(elm);
    return elm;
}

List_t *
listPeekFront(List_t *list)
{
    if (listLength(list) <= 0) return NULL;

    List_t *elm = list->next;
    return elm;
}

List_t *
listRemoveBack(List_t *list)
{
    if (listLength(list) <= 0) return NULL;

    List_t *elm = list->prev;

    listRemove(elm);
    return elm;
}

bool
listEmpty(List_t *list)
{
    return list->next == list;
}

int
listLength(List_t *list)
{
    int count = 0;

    for (List_t *head = list->next; head != list; head = head->next)
        count++;
    return count;
}
