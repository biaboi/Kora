#include "../inc/list.h"


void list_init(list *lst){
	lst->dmy.next = lst->dmy.prev = &(lst->dmy);
	lst->dmy.leader = lst;
	lst->dmy.value = 0;
	lst->iterator = &(lst->dmy);
	lst->list_len = 0;
}


int list_insert_end(list *lst, base_node *new) {
	if (new->leader == lst)
		return lst->list_len;

	base_node *dh = &(lst->dmy);
	new->leader = lst;
	new->prev = dh->prev;
	new->next = dh;
	dh->prev->next = new;
	dh->prev = new;

	return ++(lst->list_len);
} 


// sort by new node's value
int list_insert(list *lst, base_node *new){
	if (new->leader == lst)
		return lst->list_len;
	
	base_node *it = lst->dmy.next;
	while (it != &(lst->dmy) && new->value >= it->value)
		it = it->next;

	new->leader = lst;
	new->prev = it->prev;
	new->next = it;
	it->prev->next = new;
	it->prev = new;

	return ++(lst->list_len);
}

int list_remove(base_node *node){
	list *lst = node->leader;
	if (lst == NULL)
		return -1;
	if (lst->iterator == node)
		lst->iterator = node->prev;

	node->prev->next = node->next;
	node->next->prev = node->prev;
	node->leader = NULL;
	return --(lst->list_len);
} 


base_node* list_get_first(list *lst){
	if (lst->list_len == 0)
		return NULL;
	return lst->dmy.next;
}

