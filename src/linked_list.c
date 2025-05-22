#include "linked_list.h"

#include <stddef.h>
#include <stdlib.h>

LinkedList *linked_list_create(LinkedList *next, LinkedList *prev, void *element)
{
	LinkedList *linked_list = (LinkedList *)malloc(sizeof(LinkedList));

	if (linked_list == NULL) {
		return NULL;
	}

	linked_list->prev = prev;
	linked_list->next = next;
	linked_list->element = element;

	return linked_list;
}

void linked_list_destroy(LinkedList *linked_list)
{
	free(linked_list);
}

LinkedList *linked_list_insert(LinkedList *it, void *element)
{
	LinkedList *new = linked_list_create(it, it->next, element);

	if (new == NULL) {
		return NULL;
	}

	it->next = new;
	new->next->prev = new;

	return new;
}

void linked_list_erase(LinkedList *it)
{
	if (it->prev != NULL) {
		it->prev->next = it->next;
	}

	if (it->next != NULL) {
		it->next->prev = it->prev;
	}

	linked_list_destroy(it);
}
