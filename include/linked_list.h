#ifndef JD297_LINKED_LIST_H
#define JD297_LINKED_LIST_H

#include <stddef.h>

typedef struct LinkedList {
	struct LinkedList *prev;
	struct LinkedList *next;
	void *element;
} LinkedList;


extern LinkedList *linked_list_create(LinkedList *next, LinkedList *prev, void *element);

extern void linked_list_destroy(LinkedList *linked_list);

extern LinkedList *linked_list_insert(LinkedList *it, void *element);

extern void linked_list_erase(LinkedList *it);

#endif
