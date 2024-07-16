/**
 * @file pt_list.h
 * @brief 
 * @author lijianpeng (bosco@pthyidh.com)
 * @date 2024-02-29 10:51:04
 * 
 * @copyright Copyright (c) 2024 by 深圳市鹏天微智能科技有限公司, All Rights Reserved.
 */
#ifndef PT_LIST_H
#define PT_LIST_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Double List structure
 */
struct pt_list_node
{
    struct pt_list_node *next;                          /**< point to next node. */
    struct pt_list_node *prev;                          /**< point to prev node. */
};
typedef struct pt_list_node pt_list_t;                  /**< Type for lists. */

/**
 * Single List structure
 */
struct pt_slist_node
{
    struct pt_slist_node *next;                         /**< point to next node. */
};
typedef struct pt_slist_node pt_slist_t;                /**< Type for single list. */


/**
 * pt_container_of - return the member address of ptr, if the type of ptr is the
 * struct type.
 */
#define pt_container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member)))


/**
 * @brief initialize a list object
 */
#define PT_LIST_OBJECT_INIT(object) { &(object), &(object) }

/**
 * @brief initialize a list
 *
 * @param l list to be initialized
 */
pt_inline void pt_list_init(pt_list_t *l)
{
    l->next = l->prev = l;
}

/**
 * @brief insert a node after a list
 *
 * @param l list to insert it
 * @param n new node to be inserted
 */
pt_inline void pt_list_insert_after(pt_list_t *l, pt_list_t *n)
{
    l->next->prev = n;
    n->next = l->next;

    l->next = n;
    n->prev = l;
}

/**
 * @brief insert a node before a list
 *
 * @param n new node to be inserted
 * @param l list to insert it
 */
pt_inline void pt_list_insert_before(pt_list_t *l, pt_list_t *n)
{
    l->prev->next = n;
    n->prev = l->prev;

    l->prev = n;
    n->next = l;
}

/**
 * @brief remove node from list.
 * @param n the node to remove from the list.
 */
pt_inline void pt_list_remove(pt_list_t *n)
{
    n->next->prev = n->prev;
    n->prev->next = n->next;

    n->next = n->prev = n;
}

/**
 * @brief tests whether a list is empty
 * @param l the list to test.
 */
pt_inline int pt_list_isempty(const pt_list_t *l)
{
    return l->next == l;
}

/**
 * @brief get the list length
 * @param l the list to get.
 */
pt_inline unsigned int pt_list_len(const pt_list_t *l)
{
    unsigned int len = 0;
    const pt_list_t *p = l;
    while (p->next != l)
    {
        p = p->next;
        len ++;
    }

    return len;
}

/**
 * @brief get the struct for this entry
 * @param node the entry point
 * @param type the type of structure
 * @param member the name of list in structure
 */
#define pt_list_entry(node, type, member) \
    pt_container_of(node, type, member)

/**
 * pt_list_for_each - iterate over a list
 * @pos:    the pt_list_t * to use as a loop cursor.
 * @head:   the head for your list.
 */
#define pt_list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * pt_list_for_each_safe - iterate over a list safe against removal of list entry
 * @pos:    the pt_list_t * to use as a loop cursor.
 * @n:      another pt_list_t * to use as temporary storage
 * @head:   the head for your list.
 */
#define pt_list_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos != (head); \
        pos = n, n = pos->next)

/**
 * pt_list_for_each_entry  -   iterate over list of given type
 * @pos:    the type * to use as a loop cursor.
 * @head:   the head for your list.
 * @member: the name of the list_struct within the struct.
 */
#define pt_list_for_each_entry(pos, head, member) \
    for (pos = pt_list_entry((head)->next, typeof(*pos), member); \
         &pos->member != (head); \
         pos = pt_list_entry(pos->member.next, typeof(*pos), member))

/**
 * pt_list_for_each_entry_safe - iterate over list of given type safe against removal of list entry
 * @pos:    the type * to use as a loop cursor.
 * @n:      another type * to use as temporary storage
 * @head:   the head for your list.
 * @member: the name of the list_struct within the struct.
 */
#define pt_list_for_each_entry_safe(pos, n, head, member) \
    for (pos = pt_list_entry((head)->next, typeof(*pos), member), \
         n = pt_list_entry(pos->member.next, typeof(*pos), member); \
         &pos->member != (head); \
         pos = n, n = pt_list_entry(n->member.next, typeof(*n), member))

/**
 * pt_list_first_entry - get the first element from a list
 * @ptr:    the list head to take the element from.
 * @type:   the type of the struct this is embedded in.
 * @member: the name of the list_struct within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define pt_list_first_entry(ptr, type, member) \
    pt_list_entry((ptr)->next, type, member)

#define PT_SLIST_OBJECT_INIT(object) { PT_NULL }

/**
 * @brief initialize a single list
 *
 * @param l the single list to be initialized
 */
pt_inline void pt_slist_init(pt_slist_t *l)
{
    l->next = PT_NULL;
}

pt_inline void pt_slist_append(pt_slist_t *l, pt_slist_t *n)
{
    struct pt_slist_node *node;

    node = l;
    while (node->next) node = node->next;

    /* append the node to the tail */
    node->next = n;
    n->next = PT_NULL;
}

pt_inline void pt_slist_insert(pt_slist_t *l, pt_slist_t *n)
{
    n->next = l->next;
    l->next = n;
}

pt_inline unsigned int pt_slist_len(const pt_slist_t *l)
{
    unsigned int len = 0;
    const pt_slist_t *list = l->next;
    while (list != PT_NULL)
    {
        list = list->next;
        len ++;
    }

    return len;
}

pt_inline pt_slist_t *pt_slist_remove(pt_slist_t *l, pt_slist_t *n)
{
    /* remove slist head */
    struct pt_slist_node *node = l;
    while (node->next && node->next != n) node = node->next;

    /* remove node */
    if (node->next != (pt_slist_t *)0) node->next = node->next->next;

    return l;
}

pt_inline pt_slist_t *pt_slist_first(pt_slist_t *l)
{
    return l->next;
}

pt_inline pt_slist_t *pt_slist_tail(pt_slist_t *l)
{
    while (l->next) l = l->next;

    return l;
}

pt_inline pt_slist_t *pt_slist_next(pt_slist_t *n)
{
    return n->next;
}

pt_inline int pt_slist_isempty(pt_slist_t *l)
{
    return l->next == PT_NULL;
}

/**
 * @brief get the struct for this single list node
 * @param node the entry point
 * @param type the type of structure
 * @param member the name of list in structure
 */
#define pt_slist_entry(node, type, member) \
    pt_container_of(node, type, member)

/**
 * pt_slist_for_each - iterate over a single list
 * @pos:    the pt_slist_t * to use as a loop cursor.
 * @head:   the head for your single list.
 */
#define pt_slist_for_each(pos, head) \
    for (pos = (head)->next; pos != PT_NULL; pos = pos->next)

/**
 * pt_slist_for_each_entry  -   iterate over single list of given type
 * @pos:    the type * to use as a loop cursor.
 * @head:   the head for your single list.
 * @member: the name of the list_struct within the struct.
 */
#define pt_slist_for_each_entry(pos, head, member) \
    for (pos = pt_slist_entry((head)->next, typeof(*pos), member); \
         &pos->member != (PT_NULL); \
         pos = pt_slist_entry(pos->member.next, typeof(*pos), member))

/**
 * pt_slist_first_entry - get the first element from a slist
 * @ptr:    the slist head to take the element from.
 * @type:   the type of the struct this is embedded in.
 * @member: the name of the slist_struct within the struct.
 *
 * Note, that slist is expected to be not empty.
 */
#define pt_slist_first_entry(ptr, type, member) \
    pt_slist_entry((ptr)->next, type, member)

/**
 * pt_slist_tail_entry - get the tail element from a slist
 * @ptr:    the slist head to take the element from.
 * @type:   the type of the struct this is embedded in.
 * @member: the name of the slist_struct within the struct.
 *
 * Note, that slist is expected to be not empty.
 */
#define pt_slist_tail_entry(ptr, type, member) \
    pt_slist_entry(pt_slist_tail(ptr), type, member)


#ifdef __cplusplus
}
#endif

#endif /* PT_LIST_H */

