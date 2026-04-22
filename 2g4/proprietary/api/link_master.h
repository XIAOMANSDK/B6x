/**
 ****************************************************************************************
 *
 * @file link_master.h
 *
 * @brief Header file - Link Master
 *
 ****************************************************************************************
 */

#ifndef _LINK_MASTER_H_
#define _LINK_MASTER_H_

#include <stdint.h>
#include "link_param.h"

/*
 * DEFINES
 ****************************************************************************************
 */

#define MASTER_RECV_MAX_LENGTH       ( LINK_PACKET_MAX_LENGTH - 2 - 3 )
#define MASTER_PIGGYBACK_MAX_LENGTH  ( LINK_PACKET_MAX_LENGTH / 2 - 2 - 3 - 2 - 1 )

typedef struct
{
    // Link phy common configration
    link_conf_t link;
    // Channel switch thresh (rssi median)
    uint16_t rssi_median_thresh;
    // Channel switch thresh (rssi peak)
    uint16_t rssi_peak_thresh;
    // Conflict detection period (in ms)
    uint16_t conflict_detect_period;
    // Conflict detection thresh (percentage of errors. max = 100)
    uint16_t conflict_detect_thresh;
    // Additional time ahead of RX start point
    uint16_t rx_leading_time;
    // Ack delay
    uint16_t ack_delay;
    // Connection alive time(in ms)
    uint32_t conn_alive;
    // Log level (NONE = 0, ERROR = 1, WARN = 2, INFO = 3, DEBUG = 4)
    uint8_t log_level;
} link_master_conf_t;

typedef struct
{
    void *param;
    uint32_t flag;
    void (*user_proc)(void *, uint32_t);
    void (*log_out)(const char *);
} link_master_handler_t;

typedef enum
{
    MASTER_MESSAGE_CONNECTION_CREATED    = 1,
    MASTER_MESSAGE_CONNECTION_CLOSED     = 2,
    MASTER_MESSAGE_DATA_RECEIVED         = 3,
    MASTER_MESSAGE_DATA_SENT             = 4,
} link_master_message_type_t;

typedef struct
{
    uint8_t type;
    uint8_t cid;
    uint32_t address;
    union {

        struct
        {
            uint8_t data[MASTER_RECV_MAX_LENGTH];
            uint16_t length;
        } buff;

        struct
        {
            link_tos_t tos;
        } conn;
    } body;
} link_master_message_t;

/*
 * FUNCTION DECLARATION
 ****************************************************************************************
 */

void link_master_init(void);
void link_master_conf(link_master_conf_t *conf, link_master_handler_t *handler);
void link_master_schedule(void);
int link_master_fetch_message(link_master_message_t *msg);
int link_master_piggyback(uint8_t cid, uint8_t *data, uint16_t length);

#endif
