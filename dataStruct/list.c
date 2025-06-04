#include "list.h"
#include "Kora.h"


void list_init(list_t *lst){
	lst->dmy.next = lst->dmy.prev = &(lst->dmy);
	lst->dmy.leader = lst;
	lst->dmy.value = 0;
	lst->list_len = 0;
}


/*
@ brief: Insert node to list end.
*/
int list_insert_end(list_t *lst, list_node_t *node) {
	if (node->leader != NULL)
		return RET_FAILED;

	node->leader = lst;

	list_node_t *dmy = &(lst->dmy);
	node->prev = dmy->prev;
	node->next = dmy;
	dmy->prev->next = node;
	dmy->prev = node;

	return ++(lst->list_len);
} 


int list_insert_before(list_t *lst, list_node_t *pos, list_node_t *node){
	if (node->leader != NULL)
		return RET_FAILED;

	node->leader = lst;

	node->prev = pos->prev;
	node->next = pos;
	pos->prev->next = node;
	pos->prev = node;

	return ++(lst->list_len);
}

/*
@ brief: Insert node, sort by node's value(rise)
*/
int list_insert(list_t *lst, list_node_t *node){
	if (node->leader != NULL)
		return RET_FAILED;

	list_node_t *it = lst->dmy.next;
	while (it != &(lst->dmy) && node->value >= it->value)
		it = it->next;

	node->leader = lst;

	node->prev = it->prev;
	node->next = it;
	it->prev->next = node;
	it->prev = node;

	return ++(lst->list_len);
}


int list_remove(list_node_t *node){
	list_t *lst = node->leader;
	if (lst == NULL)
		return RET_FAILED;

	node->prev->next = node->next;
	node->next->prev = node->prev;
	node->leader = NULL;
	return --(lst->list_len);
} 
