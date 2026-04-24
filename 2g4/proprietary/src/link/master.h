/**
 ****************************************************************************************
 *
 * @file master.h
 *
 * @brief Header file - Link Master
 *
 ****************************************************************************************
 */

#ifndef _MASTER_H_
#define _MASTER_H_

#include <stdint.h>
#include "link.h"
#include "link_master.h"
#include "median.h"
#include "rssi.h"

/*
 * DEFINES
 ****************************************************************************************
 */

#define MASTER_EVENT_SLOT_BEGIN (0x00000001)
#define MASTER_EVENT_SYNC_FOUND (0x00000002)
#define MASTER_EVENT_RX_DONE    (0x00000100)

#define MASTER_MESSAGE_COUNT_POWER (6)
#define MASTER_MESSAGE_MAX_COUNT   (1UL << MASTER_MESSAGE_COUNT_POWER)
#define MASTER_PIGGYBACK_LIMIT     (LINK_PACKET_MAX_LENGTH / 2 - 2 - 3 - 2 - 1)

typedef struct
{
    uint16_t prefix_time;
    uint8_t  chn_n;
    uint16_t max_detect;
} link_master_param_t;

typedef enum
{
    MASTER_CHN_NONE = 0,
    MASTER_CHN_SCANNING,
    MASTER_CHN_SELECTED
} link_master_chn_state_t;

typedef enum
{
    MASTER_IDLE = 0,
    MASTER_RECV_DATA,
    MASTER_HANDLE_PACKET,
    MASTER_SEND_ACK
} link_master_rtx_state_t;

typedef struct
{
    uint32_t          address;
    link_tos_t        tos;
    link_assignment_t assign;
    uint8_t           seq;
    uint32_t          last_n;
    uint8_t           piggyback[MASTER_PIGGYBACK_LIMIT + 1];
} link_master_conn_t;

typedef struct
{
    // Slot state
    uint32_t slot_n;
    // Channel state
    volatile link_master_chn_state_t chn;
    uint8_t                          chn_sel;
    uint8_t                          mdm_err_cnt;
    uint16_t                         detect_errors;
    uint16_t                         detect_count;
    median_res_t                     chn_ref;
    median_res_t                     chn_rssi[LINK_MAX_CHN];

    rssi_table_t rssi_tab;

    // RTX state
    volatile link_master_rtx_state_t rtx;
    // Recv context
    uint8_t *recv_buff;
    uint16_t recv_length;

    uint32_t recv_slot_n;
    uint16_t recv_fine_n;

    link_master_conn_t conn[LINK_MAX_CONN];

} link_master_state_t;

#endif
