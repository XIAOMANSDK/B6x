/**
 ****************************************************************************************
 *
 * @file slave.h
 *
 * @brief Header file - Link Slave
 *
 ****************************************************************************************
 */

#ifndef _SLAVE_H_
#define _SLAVE_H_

#include <stdint.h>
#include "link.h"
#include "link_slave.h"
#include "circle.h"

/*
 * DEFINES
 ****************************************************************************************
 */

#define SLAVE_EVENT_SLOT_BEGIN 0x00000001
#define SLAVE_EVENT_SYNC_ERROR 0x00000002
#define SLAVE_EVENT_TX_DONE    0x00000004
#define SLAVE_EVENT_RX_DONE    0x00000100

#define SLAVE_SYSCALL_CONNECT (1)
#define SLAVE_SYSCALL_SEND    (2)

#define SLAVE_CONN_REQ_LENGTH                                                                      \
    (sizeof(link_packet_t) + sizeof(link_packet_payload_req_t) + LINK_PACKET_CRC_LENGTH)
#define SLAVE_SEND_COUNT_POWER (3)
#define SLAVE_SEND_MAX_LENGTH  (LINK_PACKET_MAX_LENGTH)
#define SLAVE_MAX_PAYLOAD_LENGTH                                                                   \
    (SLAVE_SEND_MAX_LENGTH - sizeof(link_packet_t) - LINK_PACKET_CRC_LENGTH)

#define SLAVE_PIGGYBACK_COUNT_POWER (3)

typedef enum
{
    SLAVE_DISCONNECTED = 0,
    SLAVE_CONNECTING,
    SLAVE_CONNECTED
} link_slave_conn_state_t;

typedef enum
{
    SLAVE_IDLE = 0,
    SLAVE_SEND_DATA,
    SLAVE_RECV_ACK,
} link_slave_rtx_state_t;

typedef struct
{
    // Slot state
    uint32_t slot_n;
    // Channel state
    uint8_t chn_sel;

    // Connection state
    volatile link_slave_conn_state_t conn;
    uint8_t                          cid;
    link_assignment_t                assign;

    // Sync state
    uint8_t req_buff[SLAVE_CONN_REQ_LENGTH];

    uint32_t req_slot_n;
    uint16_t req_fine_n;
    uint8_t  req_retry;

    // RX state
    volatile link_slave_rtx_state_t rtx;

    uint8_t *recv_buff;
    uint16_t recv_length;
    uint32_t recv_slot_n;
    uint16_t recv_fine_n;

    // Tx state
    uint32_t sent_slot_n;
    uint16_t sent_fine_n;

    // TX context
    circle_buff_t send_queue;
    uint8_t       send_seq;
    uint8_t       send_retry;

    // Heartbeat
    uint8_t heartbeat;

    // Send context
    uint8_t *send_payload;
    uint16_t send_length;
    int      send_rtcode;

    // Piggy back buff
    circle_buff_t piggy_queue;

    // MDM state error cnt
    uint8_t mdm_err_cnt;
} link_slave_state_t;

#endif
