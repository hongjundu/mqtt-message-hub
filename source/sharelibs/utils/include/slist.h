#ifndef SLIST_H
#define SLIST_H

#ifdef __cplusplus
extern "C" {
#endif

#define NODE_T(ptr, type) ((type*)ptr)

struct slist_node {
    struct slist_node * next;
};

typedef void (*list_op_free_node)(struct slist_node *node);
/*
 * return 0 on hit key, else return none zero
 */
typedef int (*list_op_key_hit_test)(struct slist_node *node, void *key);

struct single_list {
    /* all the members must not be changed manually by callee */
    struct slist_node * head;
    struct slist_node * tail;
    int size; /* length of the list, do not change it manually*/

    /* free method to delete the node
     */
    void (*free_node)(struct slist_node *node);
    /*
     * should be set by callee, used to locate node by key(*_by_key() method)
     * return 0 on hit key, else return none zero
     */
    int (*key_hit_test)(struct slist_node *node, void *key);

    struct single_list *(*add)(struct single_list * list, struct slist_node * node);
    struct single_list *(*insert)(struct single_list * list, int pos, struct slist_node *node);
    /* NOTE: the original node at the pos will be freed by free_node */
    struct single_list *(*replace)(struct single_list *list, int pos, struct slist_node *node);
    struct slist_node *(*find_by_key)(struct single_list *, void * key);
    struct slist_node *(*first)(struct single_list* list);
    struct slist_node *(*last)(struct single_list* list);
    struct slist_node *(*at)(struct single_list * list, int pos);
    struct slist_node *(*take_at)(struct single_list * list, int pos);
    struct slist_node *(*take_by_key)(struct single_list * list, void *key);
    struct single_list *(*remove)(struct single_list * list, struct slist_node * node);
    struct single_list *(*remove_at)(struct single_list *list, int pos);
    struct single_list *(*remove_by_key)(struct single_list *list, void *key);
    int (*length)(struct single_list * list);
    void (*clear)(struct single_list * list);
    void (*deletor)(struct single_list *list);
};

struct single_list * new_single_list(list_op_free_node op_free, list_op_key_hit_test op_cmp);

#ifdef __cplusplus
}
#endif

#endif // SLIST_H