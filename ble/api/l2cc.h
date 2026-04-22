/**
 ****************************************************************************************
 *
 * @file l2cc.h
 *
 * @brief L2CAP Controller Messages.
 *
 ****************************************************************************************
 */

#ifndef L2CC_H_
#define L2CC_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "task.h"
#include <stdbool.h>


/*
 * MESSAGES
 ****************************************************************************************
 */

/// Message of the L2CC task
/*@TRACE*/
enum l2cc_msg_id
{
    /// L2CAP Operation completed event
    L2CC_CMP_EVT                       = MSG_ID(L2CC, 0x04),

    /* ************* LE Credit Based Connection ************* */
    /// LE credit based connection request
    L2CC_LECB_CONNECT_CMD              = MSG_ID(L2CC, 0x05),
    /// LE credit based connection request indication
    L2CC_LECB_CONNECT_REQ_IND          = MSG_ID(L2CC, 0x06),
    /// LE credit based connection request confirmation
    L2CC_LECB_CONNECT_CFM              = MSG_ID(L2CC, 0x07),
    /// LE credit based connection indication
    L2CC_LECB_CONNECT_IND              = MSG_ID(L2CC, 0x08),
    /// LE credit based disconnect request
    L2CC_LECB_DISCONNECT_CMD           = MSG_ID(L2CC, 0x09),
    /// LE credit based disconnect indication
    L2CC_LECB_DISCONNECT_IND           = MSG_ID(L2CC, 0x0A),
    /// LE credit based credit addition
    L2CC_LECB_ADD_CMD                  = MSG_ID(L2CC, 0x0B),
    /// LE credit based credit addition indication
    L2CC_LECB_ADD_IND                  = MSG_ID(L2CC, 0x0C),

    /// Send data over LE Credit Based Connection
    L2CC_LECB_SDU_SEND_CMD             = MSG_ID(L2CC, 0x0D),
    /// Received data from LE Credit Based connection.
    L2CC_LECB_SDU_RECV_IND             = MSG_ID(L2CC, 0x0E),
    /// Indication to the task that sends the unknown message
    L2CC_UNKNOWN_MSG_IND               = MSG_ID(L2CC, 0x0F),
};

/// request operation type - application interface
enum l2cc_op
{
    /*                 Operation Flags                  */
    /* No Operation (if nothing has been requested)     */
    /* ************************************************ */
    /// No operation
    L2CC_NO_OP                                    = 0x00,

    /* LE Credit Based                                  */
    /* ************************************************ */
    /// LE credit based connection request
    L2CC_LECB_CONNECT,
    /// LE credit based disconnection request
    L2CC_LECB_DISCONNECT,
    /// LE credit addition request
    L2CC_LECB_CREDIT_ADD,
    /// Send SDU
    L2CC_LECB_SDU_SEND,
};

/// Default L2Cap SDU definition
/*@TRACE*/
struct l2cc_sdu
{
    /// Channel Identifier
    uint16_t cid;
    /// Number of credit used
    uint16_t credit;
    /// SDU Data length
    uint16_t length;
    /// data
    uint8_t data[];
};

/// Operation completed event
/*@TRACE*/
struct l2cc_cmp_evt
{
    /// L2CC request type (@see enum l2cc_op)
    uint8_t operation;
    /// Status of request.
    uint8_t status;
    /// Channel ID
    uint16_t cid;
    /// Number of peer credit used - only relevant for LECB
    uint16_t credit;
};

/// LE credit based connection request
/*@TRACE*/
struct l2cc_lecb_connect_cmd
{
    /// L2CC request type:
    /// - L2CC_LECB_CONNECT: LE credit connection
    uint8_t operation;
    /// parameter used for internal usage
    uint8_t  pkt_id;
    /// LE Protocol/Service Multiplexer
    uint16_t le_psm;
    /// Local Channel identifier (0: automatically allocate a free channel)
    uint16_t local_cid;
    /// Credit allocated for the LE Credit Based Connection
    /// Shall be at least: floor(((SDU + 2) + (MPS - 1)) / MPS) + 1
    /// To be sure that 1 SDU can be fully received without requesting credits to application
    uint16_t local_credit;
    /// Maximum SDU size - Shall not exceed device MTU
    uint16_t local_mtu;
    /// Maximum Packet size - Shall not exceed device MPS
    uint16_t local_mps;
};

/// LE credit based connection request indication
/*@TRACE*/
struct l2cc_lecb_connect_req_ind
{
    /// LE Protocol/Service Multiplexer
    uint16_t le_psm;
    /// Peer Channel identifier
    uint16_t peer_cid;
    /// Maximum SDU size
    uint16_t peer_mtu;
    /// Maximum Packet size
    uint16_t peer_mps;
};

/// LE credit based connection request confirmation
/*@TRACE*/
struct l2cc_lecb_connect_cfm
{
    /// Peer Channel identifier
    uint16_t peer_cid;
    /// True to accept the incoming connection, False else
    bool accept;
    /// Local Channel identifier (0: automatically allocate a free channel)
    uint16_t local_cid;
    /// Credit allocated for the LE Credit Based Connection
    /// Shall be at least: floor(((SDU + 2) + (MPS - 1)) / MPS) + 1
    /// To be sure that 1 SDU can be fully received without requesting credits to application
    uint16_t local_credit;
    /// Maximum SDU size - Shall not exceed device MTU
    uint16_t local_mtu;
    /// Maximum Packet size - Shall not exceed device MPS
    uint16_t local_mps;
};

/// LE credit based connection indication
/*@TRACE*/
struct l2cc_lecb_connect_ind
{
    /// Status
    uint8_t  status;
    /// LE Protocol/Service Multiplexer
    uint16_t le_psm;
    /// Local Channel identifier
    uint16_t local_cid;
    ///  Destination Credit for the LE Credit Based Connection
    uint16_t peer_credit;
    /// Maximum SDU size
    uint16_t peer_mtu;
    /// Maximum Packet size
    uint16_t peer_mps;
};

/// LE credit based disconnect request
/*@TRACE*/
struct l2cc_lecb_disconnect_cmd
{
    /// L2CC request type:
    /// - L2CC_LECB_DISCONNECT: LE credit disconnection
    uint8_t  operation;
    /// parameter used for internal usage
    uint8_t  pkt_id;
    /// Local Channel identifier
    uint16_t local_cid;
};

/// LE credit based disconnect indication
/*@TRACE*/
struct l2cc_lecb_disconnect_ind
{
    /// Local Channel identifier
    uint16_t local_cid;
    /// Reason
    uint8_t reason;
};

/// LE credit based credit addition
/*@TRACE*/
struct l2cc_lecb_add_cmd
{
    /// L2CC request type:
    /// - L2CC_LECB_CREDIT_ADD: LE credit addition
    uint8_t  operation;
    /// parameter used for internal usage
    uint8_t  pkt_id;
    /// Local Channel identifier
    uint16_t local_cid;
    /// Credit added locally for channel identifier
    uint16_t credit;
};

/// LE credit based credit addition indication
/*@TRACE*/
struct l2cc_lecb_add_ind
{
    /// Local Channel identifier
    uint16_t local_cid;
    /// Destination added credit (relative value)
    uint16_t peer_added_credit;
};

/// Send data over an LE Credit Based Connection
/*@TRACE*/
struct l2cc_lecb_sdu_send_cmd
{
    /// L2CC request type (@see enum l2cc_op):
    /// - L2CC_LECB_SDU_SEND: Send a SDU
    uint8_t operation;
    /// offset value information - for internal use only
    uint16_t offset;
    /// SDU information
    struct l2cc_sdu sdu;
};

/// Inform that a data packet has been received from a LE Credit Based connection.
/*@TRACE*/
struct l2cc_lecb_sdu_recv_ind
{
    /// Status information
    uint8_t status;
    /// offset value information
    uint16_t offset;
    /// SDU information
    struct l2cc_sdu sdu;
};

/// Indicate that an unknown message has been received
/*@TRACE*/
struct l2cc_unknown_msg_ind
{
    /// Unknown message id
    msg_id_t unknown_msg_id;
};

#endif // _L2CC_H_
