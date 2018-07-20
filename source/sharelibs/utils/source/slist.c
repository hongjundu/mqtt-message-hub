#include "slist.h"
#ifdef __APPLE__
#include <stdlib.h>
#else
#include <malloc.h>
#endif

static struct single_list * _add_node(struct single_list *list, struct slist_node *node)
{

    if(list->tail)
    {
        list->tail->next = node;
        node->next = 0;
        list->tail = node;
        list->size++;
    }
    else
    {
        list->head = node;
        list->tail = node;
        node->next = 0;
        list->size = 1;
    }

    return list;
}

static struct single_list * _insert_node(struct single_list * list, int pos, struct slist_node *node)
{
    if(pos < list->size)
    {
        int i = 0;
        struct slist_node * p = list->head;
        struct slist_node * prev = list->head;
        for(; i < pos; i++)
        {
            prev = p;
            p = p->next;
        }
        if(p == list->head)
        {
            /* insert at head */
            node->next = list->head;
            list->head = node;
        }
        else
        {
            prev->next = node;
            node->next = p;
        }

        if(node->next == 0) list->tail = node;
        list->size++;
    }
    else
    {
        list->add(list, node);
    }

    return list;
}

static struct single_list * _replace(struct single_list * list, int pos, struct slist_node *node)
{
    if(pos < list->size)
    {
        int i = 0;
        struct slist_node * p = list->head;
        struct slist_node * prev = list->head;
        for(; i < pos; i++)
        {
            prev = p;
            p = p->next;
        }
        if(p == list->head)
        {
            /* replace at head */
            node->next = list->head->next;
            list->head = node;
        }
        else
        {
            prev->next = node;
            node->next = p->next;
        }

        if(node->next == 0) list->tail = node;

        if(list->free_node) list->free_node(p);
        else if(p) free(p);
    }

    return list;
}

static struct slist_node * _find_by_key(struct single_list *list, void * key)
{
    if(list->key_hit_test)
    {
        struct slist_node * p = list->head;
        while(p)
        {
            if(list->key_hit_test(p, key) == 0) return p;
            p = p->next;
        }
    }
    return 0;
}

static struct slist_node *_first_of(struct single_list* list)
{
    return list->head;
}

static struct slist_node *_last_of(struct single_list* list)
{
    return list->tail;
}

static struct slist_node *_node_at(struct single_list * list, int pos)
{
    if(pos < list->size)
    {
        int i = 0;
        struct slist_node * p = list->head;
        for(; i < pos; i++)
        {
            p = p->next;
        }
        return p;
    }

    return 0;
}

static struct slist_node * _take_at(struct single_list * list, int pos)
{
    if(pos < list->size)
    {
        int i = 0;
        struct slist_node * p = list->head;
        struct slist_node * prev = p;
        for(; i < pos ; i++)
        {
            prev = p;
            p = p->next;
        }
        if(p == list->head)
        {
            list->head = p->next;
            if(list->head == 0) list->tail = 0;
        }
        else if(p == list->tail)
        {
            list->tail = prev;
            prev->next = 0;
        }
        else
        {
            prev->next = p->next;
        }

        list->size--;

        p->next = 0;
        return p;
    }

    return 0;
}

static struct slist_node * _take_by_key(struct single_list * list, void *key)
{
    if(list->key_hit_test)
    {
        struct slist_node * p = list->head;
        struct slist_node * prev = p;
        while(p)
        {
            if(list->key_hit_test(p, key) == 0) break;
            prev = p;
            p = p->next;
        }

        if(p)
        {
            if(p == list->head)
            {
                list->head = p->next;
                if(list->head == 0) list->tail = 0;
            }
            else if(p == list->tail)
            {
                list->tail = prev;
                prev->next = 0;
            }
            else
            {
                prev->next = p->next;
            }

            list->size--;

            p->next = 0;
            return p;
        }
    }
    return 0;
}

static struct single_list *_remove_node(struct single_list * list, struct slist_node * node)
{
    struct slist_node * p = list->head;
    struct slist_node * prev = p;
    while(p)
    {
        if(p == node) break;
        prev = p;
        p = p->next;
    }

    if(p)
    {
        if(p == list->head)
        {
            list->head = list->head->next;
            if(list->head == 0) list->tail = 0;
        }
        else if(p == list->tail)
        {
            prev->next = 0;
            list->tail = prev;
        }
        else
        {
            prev->next = p->next;
        }

        if(list->free_node) list->free_node(p);
        else if(p) free(p);

        list->size--;
    }
    return list;
}

static struct single_list *_remove_at(struct single_list *list, int pos)
{
    if(pos < list->size)
    {
        int i = 0;
        struct slist_node * p = list->head;
        struct slist_node * prev = p;
        for(; i < pos ; i++)
        {
            prev = p;
            p = p->next;
        }
        if(p == list->head)
        {
            list->head = p->next;
            if(list->head == 0) list->tail = 0;
        }
        else if(p == list->tail)
        {
            list->tail = prev;
            prev->next = 0;
        }
        else
        {
            prev->next = p->next;
        }

        if(list->free_node) list->free_node(p);
        else if(p) free(p);

        list->size--;
    }

    return list;
}

static struct single_list *_remove_by_key(struct single_list *list, void *key)
{
    if(list->key_hit_test)
    {
        struct slist_node * p = list->head;
        struct slist_node * prev = p;
        while(p)
        {
            if(list->key_hit_test(p, key) == 0) break;
            prev = p;
            p = p->next;
        }

        if(p)
        {
            if(p == list->head)
            {
                list->head = list->head->next;
                if(list->head == 0) list->tail = 0;
            }
            else if(p == list->tail)
            {
                prev->next = 0;
                list->tail = prev;
            }
            else
            {
                prev->next = p->next;
            }

            if(list->free_node) list->free_node(p);
            else if(p) free(p);

            list->size--;
        }
    }

    return list;
}

static int _length_of(struct single_list * list)
{
    return list->size;
}

static void _clear_list(struct single_list * list)
{
	if(!list)
		return;
    struct slist_node * p = list->head;
    struct slist_node * p2;
    while(p)
    {
        p2 = p;
        p = p->next;

        if(list->free_node) list->free_node(p2);
        else if(p2) free(p2);
    }

    list->head = 0;
    list->tail = 0;
    list->size = 0;
}

static void _delete_single_list(struct single_list *list)
{
	if(!list)
		return;
    list->clear(list);
	free(list);
}

struct single_list * new_single_list(list_op_free_node op_free, list_op_key_hit_test op_cmp)
{
    struct single_list *list = (struct single_list *)malloc(sizeof(struct single_list));
    list->head = 0;
    list->tail = 0;
    list->size = 0;
    list->free_node = op_free;
    list->key_hit_test = op_cmp;

    list->add = _add_node;
    list->insert = _insert_node;
    list->replace = _replace;
    list->find_by_key = _find_by_key;
    list->first = _first_of;
    list->last = _last_of;
    list->at = _node_at;
    list->take_at = _take_at;
    list->take_by_key = _take_by_key;
    list->remove = _remove_node;
    list->remove_at = _remove_at;
    list->remove_by_key = _remove_by_key;
    list->length = _length_of;
    list->clear = _clear_list;
    list->deletor = _delete_single_list;

    return list;
}