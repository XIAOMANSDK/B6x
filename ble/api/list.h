/**
 ****************************************************************************************
 *
 * @file list.h
 *
 * @brief List definitions
 *
 ****************************************************************************************
 */

#ifndef _LIST_H_
#define _LIST_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifndef __INLINE__
#define __INLINE__ __forceinline static
#endif


/*
 * DEFINES
 ****************************************************************************************
 */

/// structure of a list element header
typedef struct list_hdr
{
    /// Pointer to next list_hdr
    struct list_hdr *next;
} list_hdr_t;

/// structure of a list
typedef struct list
{
    /// pointer to first element of the list
    struct list_hdr *first;
    /// pointer to the last element
    struct list_hdr *last;
} list_t;


/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief Initialize a list to defaults values.
 *
 * @param list           Pointer to the list structure.
 ****************************************************************************************
 */
void list_init(list_t *list);

/**
 ****************************************************************************************
 * @brief Construct a list of free elements representing a pool
 *
 * @param list           Pointer to the list structure
 * @param pool           Pointer to the pool to be initialized
 * @param elmt_size      Size of one element of the pool
 * @param elmt_cnt       Nb of elements available in the pool
 ****************************************************************************************
 */
void list_pool_init(list_t *list, void *pool, size_t elmt_size, uint32_t elmt_cnt);

/**
 ****************************************************************************************
 * @brief Add an element as last on the list.
 *
 * @param list           Pointer to the list structure
 * @param list_hdr       Pointer to the header to add at the end of the list
 *
 ****************************************************************************************
 */
void list_push_back(list_t *list, list_hdr_t *list_hdr);

/**
 ****************************************************************************************
 * @brief Append a sequence of elements at the end of a list.
 *
 * Note: the elements to append shall be linked together
 *
 * @param list           Pointer to the list structure
 * @param first_hdr      Pointer to the first element to append
 * @param last_hdr       Pointer to the last element to append
 ****************************************************************************************
 */
void list_push_back_sublist(list_t *list, list_hdr_t *first_hdr, list_hdr_t *last_hdr);

/**
 ****************************************************************************************
 * @brief Add an element as first on the list.
 *
 * @param list           Pointer to the list structure
 * @param list_hdr       Pointer to the header to add at the beginning of the list
 ****************************************************************************************
 */
void list_push_front(list_t *list, list_hdr_t *list_hdr);

/**
 ****************************************************************************************
 * @brief Extract the first element of the list.
 * @param list           Pointer to the list structure
 * @return The pointer to the element extracted, and NULL if the list is empty.
 ****************************************************************************************
 */
list_hdr_t *list_pop_front(list_t *list);

/**
 ****************************************************************************************
 * @brief Search for a given element in the list, and extract it if found.
 *
 * @param list           Pointer to the list structure
 * @param list_hdr       Element to extract
 *
 * @return true if the element is found in the list, false otherwise
 ****************************************************************************************
 */
bool list_extract(list_t *list, list_hdr_t *list_hdr);

/**
 ****************************************************************************************
 * @brief Extract an element when the previous element is known
 *
 * Note: the element to remove shall follow immediately the reference within the list
 *
 * @param list           Pointer to the list structure
 * @param elt_ref_hdr    Pointer to the referenced element (NULL if element to extract is the first in the list)
 * @param elt_to_rem_hdr Pointer to the element to be extracted
 ****************************************************************************************
 */
void list_extract_after(list_t *list, list_hdr_t *elt_ref_hdr, list_hdr_t *elt_to_rem_hdr);

/**
 ****************************************************************************************
 * @brief Extract a sub-list when the previous element is known
 *
 * Note: the elements to remove shall be linked together and  follow immediately the reference element
 *
 * @param[in] list       Pointer to the list structure
 * @param[in] ref_hdr    Pointer to the referenced element (NULL if first element to extract is first in the list)
 * @param[in] last_hdr   Pointer to the last element to extract ()
 ****************************************************************************************
 */
void list_extract_sublist(list_t *list, list_hdr_t *ref_hdr, list_hdr_t *last_hdr);

/**
 ****************************************************************************************
 * @brief Searched a given element in the list.
 *
 * @param list           Pointer to the list structure
 * @param list_hdr       Pointer to the searched element
 *
 * @return true if the element is found in the list, false otherwise
 ****************************************************************************************
 */
bool list_find(list_t *list, list_hdr_t *list_hdr);

/**
 ****************************************************************************************
 * @brief Merge two lists in a single one.
 *
 * This function appends the list pointed by list2 to the list pointed by list1. Once the
 * merge is done, it empties list2.
 *
 * @param list1          Pointer to the destination list
 * @param list2          Pointer to the list to append to list1
 ****************************************************************************************
 */
void list_merge(list_t *list1, list_t *list2);

/**
 ****************************************************************************************
 * @brief Insert a given element in the list before the referenced element.
 *
 * @param list           Pointer to the list structure
 * @param elt_ref_hdr    Pointer to the referenced element
 * @param elt_to_add_hdr Pointer to the element to be inserted
 *
 * @return true if the element is found in the list, false otherwise
 ****************************************************************************************
 */
void list_insert_before(list_t *list, list_hdr_t *elt_ref_hdr, list_hdr_t *elt_to_add_hdr);

/**
 ****************************************************************************************
 * @brief Insert a given element in the list after the referenced element.
 *
 * @param list           Pointer to the list structure
 * @param elt_ref_hdr    Pointer to the referenced element
 * @param elt_to_add_hdr Pointer to the element to be inserted
 *
 * @return true if the element is found in the list, false otherwise
 ****************************************************************************************
 */
void list_insert_after(list_t *list, list_hdr_t *elt_ref_hdr, list_hdr_t *elt_to_add_hdr);

/**
 ****************************************************************************************
 * @brief Count number of elements present in the list
 *
 * @param list           Pointer to the list structure
 *
 * @return Number of elements present in the list
 ****************************************************************************************
 */
uint16_t list_size(list_t *list);

/**
 ****************************************************************************************
 * @brief Test if the list is empty.
 * @param list           Pointer to the list structure.
 * @return true if the list is empty, false else otherwise.
 ****************************************************************************************
 */
__INLINE__ bool list_is_empty(const list_t *const list)
{
    return (list->first == NULL);
}

/**
 ****************************************************************************************
 * @brief Pick the first element from the list without removing it.
 *
 * @param list           Pointer to the list structure.
 *
 * @return First element address. Returns NULL pointer if the list is empty.
 ****************************************************************************************
 */
__INLINE__ list_hdr_t *list_pick(const list_t *const list)
{
    return(list->first);
}

/**
 ****************************************************************************************
 * @brief Return following element of a list element.
 *
 * @param list_hdr       Pointer to the list element.
 *
 * @return The pointer to the next element.
 ****************************************************************************************
 */
__INLINE__ list_hdr_t *list_next(const list_hdr_t *const list_hdr)
{
    return(list_hdr->next);
}

#endif // _LIST_H_
