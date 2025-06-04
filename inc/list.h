#ifndef _OS_LIST_H
#define _OS_LIST_H

#include "KoraDef.h"


struct __list;

typedef struct __list_node {
	struct __list_node   *next;
	struct __list_node   *prev;
	struct __list        *leader;
	u_int                 value;
} list_node_t;


typedef struct __list {
	list_node_t     dmy;
	int             list_len;	
} list_t;

#define LIST_NODE_INIT(pnode) ((pnode)->leader = NULL)
#define IS_ORPHAN_NODE(pnode) ((pnode)->leader == NULL)
#define FIRST_OF(lst)         ((lst).dmy.next)

#define LIST_LEN(plst) ((plst)->list_len)
#define LIST_IS_EMPTY(plst) ((plst)->list_len == 0)
#define LIST_NOT_EMPTY(plst) ((plst)->list_len != 0)

void list_init(list_t *lst);
int list_insert(list_t *lst, list_node_t *node);
int list_insert_before(list_t *lst, list_node_t *pos, list_node_t *node);
int list_insert_end(list_t *lst, list_node_t* node);
int list_remove(list_node_t *node);


#endif
