/**
 ****************************************************************************************
 *
 * @file ke_api.h
 *
 * @brief kernel management API functions.
 *
 ****************************************************************************************
 */

#ifndef KE_API_H_
#define KE_API_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include <stdint.h>
#include <stdbool.h>
#include "task.h"
#include "list.h"


/*
 * DEFINITIONS
 ****************************************************************************************
 */

/// heap memory types.
enum mem_type
{
    /// Heap for environment allocated
    MEM_ENV,
    /// Heap for messages allocated
    MEM_MSG,

    MEM_TYPE_MAX
};

typedef struct ke_heap_cfg
{
    uint32_t base[MEM_TYPE_MAX];
    uint16_t size[MEM_TYPE_MAX];
} heap_cfg_t;


/*
 * FUNCTION DECLARATIONS: Task API
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Create a task.
 *
 * @param[in] task_type  Task type.
 * @param[in] p_desc     Pointer to task descriptor.
 *
 * @return Task status
 ****************************************************************************************
 */
uint8_t ke_task_create(uint8_t task_type, task_func_t p_desc);

/**
 ****************************************************************************************
 * @brief Retrieve the state of a task.
 *
 * @param[in] task  Task id.
 *
 * @return Current state of the task
 ****************************************************************************************
 */
state_t ke_state_get(task_id_t task);

/**
 ****************************************************************************************
 * @brief Set the state of a task(try to activate saved messages queue).
 *
 * @param[in] task   Task id
 * @param[in] state  New State
 ****************************************************************************************
 */
void ke_state_set(task_id_t task, state_t state);


/*
 * FUNCTION DECLARATIONS: Message API
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Convert a parameter pointer to a message pointer
 *
 * @param[in] param_ptr  Pointer to the parameter member of a message
 *
 * @return The pointer to the message
 ****************************************************************************************
 */
__INLINE__ struct msg_elem * ke_param2msg(const void *param_ptr)
{
    return (struct msg_elem*) (((uint8_t*)param_ptr) - offsetof(struct msg_elem, param));
}

/**
 ****************************************************************************************
 * @brief Convert a message pointer to a parameter pointer
 *
 * @param[in] msg Pointer to the message.
 *
 * @return The pointer to the param member
 ****************************************************************************************
 */
__INLINE__ void * ke_msg2param(struct msg_elem const *msg)
{
    return (void*) (((uint8_t*) msg) + offsetof(struct msg_elem, param));
}

/**
 ****************************************************************************************
 * @brief Macro wrapper to ke_msg_alloc() and cast the returned pointer to struct.
 *
 * @param[in] msgid      Message id
 * @param[in] dest_id    Destination Task id
 * @param[in] src_id     Source Task id
 * @param[in] param_str  parameter structure
 *
 * @return Pointer to the parameter member.
 ****************************************************************************************
 */
#define KE_MSG_ALLOC(msgid, dest_id, src_id, param_str) \
    (struct param_str*) ke_msg_alloc(msgid, dest_id, src_id, sizeof(struct param_str))

/**
 ****************************************************************************************
 * @brief Macro wrapper to ke_msg_alloc() with a variable length data[] located at the end.
 *
 * @param[in] msgid      Message id
 * @param[in] dest_id    Destination Task id
 * @param[in] src_id     Source Task id
 * @param[in] param_str  parameter structure
 * @param[in] length     length for the data
 *
 * @return Pointer to the parameter member.
 ****************************************************************************************
 */
#define KE_MSG_ALLOC_DYN(msgid, dest_id, src_id, param_str, length) \
    (struct param_str*)ke_msg_alloc(msgid, dest_id, src_id, (sizeof(struct param_str) + (length)));

/**
 ****************************************************************************************
 * @brief Allocate memory for a message
 *
 * This primitive allocates memory for a message that has to be sent. The memory
 * is allocated dynamically on the heap and the length of the variable parameter
 * structure has to be provided in order to allocate the correct size.
 *
 * The memory allocated will be automatically freed by the kernel, after the
 * pointer has been sent to ke_msg_send(). If the message is not sent, it must
 * be freed explicitly with ke_msg_free().
 *
 * Allocation failure is considered critical and should not happen.
 *
 * @param[in] msgid      Message id
 * @param[in] dest_id    Destination Task id
 * @param[in] src_id     Source Task id
 * @param[in] param_len  Size of the message parameters to be allocated
 *
 * @return Pointer to the parameter member of the message. If the parameter
 *         structure is empty, the pointer will point to the end of the message.
 ****************************************************************************************
 */
void *ke_msg_alloc(msg_id_t msgid, task_id_t dest_id, task_id_t src_id, uint16_t param_len);

/**
 ****************************************************************************************
 * @brief Send a message previously allocated with ke_msg_alloc() functions.
 *
 * The kernel will take care of freeing the message memory.
 * Once the function have been called, it is not possible to access its data
 * anymore as the kernel may have copied the message and freed the original
 * memory.
 *
 * @param[in] param_ptr  Pointer to the parameter member of the message
 ****************************************************************************************
 */
void ke_msg_send(const void *param_ptr);

/**
 ****************************************************************************************
 * @brief Send a message that has a zero length parameter member.
 *
 * @param[in] msgid      Message id
 * @param[in] dest_id    Destination Task id
 * @param[in] src_id     Source Task id
 ****************************************************************************************
 */
void ke_msg_send_basic(msg_id_t msgid, task_id_t dest_id, task_id_t src_id);

/**
 ****************************************************************************************
 * @brief Forward a message to another task by changing its tasks IDs.
 *
 * @param[in] param_ptr  Pointer to the parameter member of the message
 * @param[in] dest_id    New destination task of the message.
 * @param[in] src_id     New source task of the message.
 ****************************************************************************************
 */
void ke_msg_forward(const void *param_ptr, task_id_t dest_id, task_id_t src_id);

/*
 * FUNCTION DECLARATIONS: Heap API
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Allocation of a size block of memory.
 *
 * @param[in] size  Size of the memory area that need to be allocated.
 * @param[in] type  Type of memory block(@see mem_type)
 *
 * @return A pointer to the allocated memory area, or NULL if no memory is available.
 ****************************************************************************************
 */
void *ke_malloc(uint32_t size, uint8_t type);

/**
 ****************************************************************************************
 * @brief Freeing of a block of memory.
 *
 * @param[in] mem_ptr  Pointer to the memory area that need to be freed.
 ****************************************************************************************
 */
void ke_free(void *mem_ptr);


/*
 * FUNCTION DECLARATIONS: Timer API
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Set a timer.
 *
 * The function first cancel the timer if it is already existing, then
 * it creates a new one. The timer can be one-shot or periodic, i.e. it
 * will be automatically set again after each trigger.
 *
 * When the timer expires, a message is sent to the task provided as
 * argument, with the timer id as message id.
 *
 * @param[in] timer_id  Timer id (message id type).
 * @param[in] task      Task id which will be notified
 * @param[in] delay     Delay in time units(1ms).
 ****************************************************************************************
 */
void ke_timer_set(msg_id_t timer_id, task_id_t task, uint32_t delay);

/**
 ****************************************************************************************
 * @brief Remove an registered timer.
 *
 * This function search for the timer identified by its id and its task id.
 * If found it is stopped and freed, otherwise an error message is returned.
 *
 * @param[in] timer_id  Timer id.
 * @param[in] task      Task id.
 ****************************************************************************************
 */
void ke_timer_clear(msg_id_t timer_id, task_id_t task);

/**
 ****************************************************************************************
 * @brief Checks if a requested timer is active(in the timer queue).
 *
 * @param[in] timer_id  Timer id.
 * @param[in] task      Task id.
 ****************************************************************************************
 */
bool ke_timer_active(msg_id_t timer_id, task_id_t task);

#endif // KE_API_H_
