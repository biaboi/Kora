#ifndef _OS_LIST_H
#define _OS_LIST_H

#include "KoraDef.h"


struct list;

typedef struct dll_node {
	struct dll_node *next;
	struct dll_node *prev;
} dll_node;

typedef struct task_node {
	struct task_node *next;
	struct task_node *prev;
	struct list *leader;
	u_int value;
} task_node_t;


typedef struct list {
	task_node_t dmy;
	task_node_t *iterator;
	int list_len;	
} list;

#define TASK_NODE_INIT(pnode) ((pnode)->leader = NULL)
#define IS_ORPHAN_NODE(pnode) ((pnode)->leader == NULL)

#define LIST_LEN(plst) ((plst)->list_len)
#define LIST_IS_EMPTY(plst) ((plst)->list_len == 0)
#define LIST_NOT_EMPTY(plst) ((plst)->list_len != 0)

void list_init(list *lst);
int list_insert(list *lst, task_node_t *new);
int list_insert_end(list *lst, task_node_t* new);
int list_remove(task_node_t *node);
task_node_t* list_get_first(list *lst);


#endif
