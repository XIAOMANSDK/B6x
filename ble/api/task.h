/**
 ****************************************************************************************
 *
 * @file task.h
 *
 * @brief Task Definitions
 *
 ****************************************************************************************
 */

#ifndef _TASK_H_
#define _TASK_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include <stdint.h>
#include <stdbool.h>
#include "list.h"
#include "utils.h"
#include "blelib.h"


/*
 * DEFINITIONS
 ****************************************************************************************
 */

/// Type ID: 0~255(Group)
typedef uint8_t  tid_t;

/// State
typedef uint8_t  state_t;


/**
 * Task Identifier bit field
 *
 *   15   14   13   12   11   10    9    8    7    6    5    4    3    2    1    0
 * +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
 * |          Instance Index          |          Task Type(Group ID)          |
 * +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
 *
 * Bit [0-7] : task type     (Group ID, @see enum TASK_TYPE)
 * Bit [8-15]: instance index(single or multi-instantiated task)
 */
typedef uint16_t task_id_t;

/// Builds task identifier from 'task' and 'index'.
#define TASK_BUILD(task, index)   ((uint16_t)(((index) << 8) | (task)))

/// Builds task identifier from 'type'(TASK_xxxx) and 'index'.
#define TASK_ID(type, index)      ((uint16_t)(((index) << 8) | (TASK_ ## type)))

/// Retrieves task type from 'task_id'.
#define TASK_TYPE(task_id)        ((task_id) & 0xFF)

/// Retrieves task index number from 'task_id'.
#define TASK_IDX(task_id)         (((task_id) >> 8) & 0xFF)

/// Invalid task
#define TASK_INVALID              (0xFFFF)


/**
 * Message Identifier bit field
 *
 *   15   14   13   12   11   10    9    8    7    6    5    4    3    2    1    0
 * +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
 * |          Msg Type(Task Base)          |          Msg Index(Offset)            |
 * +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
 *
 * Bit [0-7] : message index(no more than 255 messages per task)
 * Bit [8-15]: message type (no more than 255 tasks support)
 */
typedef uint16_t msg_id_t;

/// Build message identifier from 'type_id' and 'index'.
#define MSG_BUILD(tid, index)     ((uint16_t)((tid) << 8) | (index))

/// Build message identifier from 'type'(TID_xxxx) and 'index'.
#define MSG_ID(type, index)       ((uint16_t)((TID_ ## type) << 8) | (index))

/// Retrieves message type from 'msg_id'.
#define MSG_TYPE(msg_id)          (((uint16_t)msg_id) >> 8)

/// Retrieves message index from 'msg_id'.
#define MSG_IDX(msg_id)           ((msg_id) & 0xFF)

/// Default Message(to handle several message type in same handler).
#define MSG_DEFAULT               (0xFFFF)

/// Message to get Task State Pointer
#define MSG_TASK_STATE            (0x0000)

/// Format of task dispatch function type
typedef void* (*task_func_t)(msg_id_t msgid, uint8_t task_idx);

/// Format of message handler function type
typedef int (*msg_func_t)(msg_id_t msgid, const void *param,                  \
                             task_id_t dest_id, task_id_t src_id);

/// Macro for message handler function definition
#define MSG_HANDLER(msg_name, param_struct)                                   \
    static int msg_name##_handler(msg_id_t msgid, param_struct const *param,  \
                             task_id_t dest_id, task_id_t src_id)

/// Macro for message handlers table definition
#define MSG_HANDLER_TAB(task) const struct msg_handler task##_msg_handler_tab[] =


/// Status returned by Message Handler
enum msg_status
{
    MSG_STATUS_FREE    = 0,  ///< consumed, msg and ext are freed by the kernel
    MSG_STATUS_NO_FREE = 1,  ///< consumed, nothing is freed by the kernel
    MSG_STATUS_SAVED   = 2,  ///< not consumed, will be pushed in the saved queue
};

/// Status returned by task API functions
enum task_status
{
    TASK_OK            = 0,
    TASK_FAIL          = 1,
    TASK_ERR_UNKNOWN   = 2,
    TASK_ERR_EXCEEDED  = 3,
    TASK_ERR_EXISTS    = 4,
};

/// Element of a message handler table.
struct msg_handler
{
    msg_id_t id;             ///< Id of the handled message.
    msg_func_t func;         ///< Pointer to the handler function.
};

/// Message Element.
struct msg_elem
{
    list_hdr_t   hdr;        ///< List header for chaining
    msg_id_t     msgid;      ///< Message id.
    task_id_t    dest_id;    ///< Destination kernel identifier.
    task_id_t    src_id;     ///< Source kernel identifier.
    uint16_t     param_len;  ///< Parameter embedded struct length.
    uint32_t     param[];    ///< Parameter embedded struct. Must be word-aligned.
};

/// Tasks types definition
enum task_type
{
    /******** Low Layer Tasks  ********/
    // Link Layer Controller Task
    TASK_LLC         = 0x01,

    // HCI Test & Debug Task
    TASK_HCI         = 0x02,

    /******** App Layer Tasks  ********/
    // Application Main Task
    TASK_APP         = 0x03,

    /******** Host Layer Tasks ********/
    // L2CAP Controller Task
    // - handles L2CAP attribute and security block.
    TASK_L2CC        = 0x04,

    // Generic Attribute Profile Task
    // - includes services and characteristic discovery, configuration exchanges
    // - and attribute value access operations(reading, writing, notification and indication).
    TASK_GATT        = 0x05,

    // Generic Access Profile Manager Task
    // - manage all non connected stuff, configuring device mode(discoverable, connectable, etc.)
    // - and perform required actions(scanning, connection, etc.)
    // - and manage GAP Controller state according to corresponding BLE connection states.
    TASK_GAPM        = 0x06,

    // Generic Access Profile Controller Task
    // - perform GAP action related to a BLE connection(pairing, update parameters, disconnect, etc.)
    // - GAP controller is multi-instantiated, one task instance per BLE connection.
    TASK_GAPC        = 0x07,

    // Maximum number of tasks
    TASK_MAX,

    // MACRO of Invalid Task
    TASK_NONE        = 0xFF,
};

/// Type ID for identifying Group(Messages and Profiles)
enum type_id
{
    /******** Low Layer Identifiers  ********/
    TID_LLC          = 1,    // LL Controller
    TID_LLD          = 2,    // LL Driver
    TID_HCI          = 3,    // HCI

    /******** Host Layer Identifiers ********/
    TID_L2CC         = 4,    // L2CAP Controller
    TID_GATT         = 5,    // Generic Attribute Profile
    TID_GAPM         = 6,    // Generic Access Profile Manager
    TID_GAPC         = 7,    // Generic Access Profile Controller

    /******** App Layer Identifiers  ********/
    TID_APP          = 9,    // Application Task
    TID_MESH         = 10,   // Mesh Service
    TID_FMNA         = 11,   // FindMy Network Accessory

    /******** PRF Layer Identifiers  ********/
    /* User customize */

    /* End customize */

    // MACRO of Invalid Identifier
    TID_INVALID      = 0xFF,
};


#endif //_TASK_H_
