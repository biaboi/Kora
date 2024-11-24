#ifndef _OS_LIST_H
#define _OS_LIST_H

#include "KoraDef.h"


struct kernel_list;

typedef struct task_node {
	struct task_node     *next;
	struct task_node     *prev;
	struct kernel_list   *leader;
	u_int                 value;
} kernel_node;


typedef struct kernel_list {
	kernel_node     dmy;
	kernel_node    *iterator;
	int             list_len;	
} kernel_list;


#define TASK_NODE_INIT(pnode) ((pnode)->leader = NULL)
#define IS_ORPHAN_NODE(pnode) ((pnode)->leader == NULL)

#define LIST_LEN(plst) ((plst)->list_len)
#define LIST_IS_EMPTY(plst) ((plst)->list_len == 0)
#define LIST_NOT_EMPTY(plst) ((plst)->list_len != 0)

void list_init(kernel_list *lst);
int list_insert(kernel_list *lst, kernel_node *new);
int list_insert_end(kernel_list *lst, kernel_node* new);
int list_remove(kernel_node *node);


#endif
