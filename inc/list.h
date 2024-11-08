#ifndef _OS_LIST_H
#define _OS_LIST_H

#include "prjdef.h"


struct list;

typedef struct base_node {
	struct base_node *next;
	struct base_node *prev;
	struct list *leader;
	u_int value;
} base_node;


typedef struct list {
	base_node dmy;
	base_node *iterator;
	int list_len;	
} list;

#define BASE_NODE_INIT(pnode) ((pnode)->leader = NULL)
#define IS_ORPHAN_NODE(pnode) ((pnode)->leader == NULL)

#define LIST_LEN(plst) ((plst)->list_len)
#define LIST_IS_EMPTY(plst) ((plst)->list_len == 0)
#define LIST_NOT_EMPTY(plst) ((plst)->list_len != 0)

void list_init(list *lst);
int list_insert(list *lst, base_node *new);
int list_insert_end(list *lst, base_node* new);
int list_remove(base_node *node);
base_node* list_get_first(list *lst);


#endif
