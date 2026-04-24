/**
 ****************************************************************************************
 *
 * @file link_slave.h
 *
 * @brief Header file - Link Slave
 *
 ****************************************************************************************
 */

#ifndef _LINK_SLAVE_H_
#define _LINK_SLAVE_H_

#include <stdint.h>
#include "link_param.h"
/*
 * DEFINES
 ****************************************************************************************
 */

#define SLAVE_PIGGYBACK_MAX_LENGTH  ( LINK_PACKET_MAX_LENGTH / 2 - 2 - 3 - 2 - 1 )

typedef struct
{
    // Link common configuration
    link_conf_t link;
    // Device address
    uint32_t address;
    // Type of service
    link_tos_t tos;

    // Timing debounce
    uint16_t debounce_window;
    // Tx retry
    uint8_t tx_retry;
    // RTX gap
    uint16_t rtx_gap;
    // Channel connect retry
    uint8_t chn_retry;
    // Connection alive time(in ms)
    uint32_t conn_alive;
    // Connection keep alive(in ms, 0 = disabled)
    uint32_t keep_alive;
    // Log level (NONE = 0, ERROR = 1, WARN = 2, INFO = 3, DEBUG = 4)
    uint8_t log_level;
} link_slave_conf_t;

typedef enum
{
     SLAVE_NERR_SUCCESS = 0,
     SLAVE_NERR_BUFF_FULL,
     SLAVE_NERR_OVERSIZE,
} link_slave_error_t;

typedef struct
{
    void *param;
    uint32_t flag;
    void (*user_proc)(void *, uint32_t);
    void (*log_out)(const char *);
} link_slave_handler_t;

typedef struct
{
    uint8_t data[SLAVE_PIGGYBACK_MAX_LENGTH];
    uint16_t length;
} link_slave_piggy_t;

/*
 * FUNCTION DECLARATION
 ****************************************************************************************
 */

void link_slave_init(void);
void link_slave_conf(link_slave_conf_t *conf, link_slave_handler_t *handler);
void link_slave_schedule(void);
int link_slave_connect(void);
int link_slave_send(uint8_t *payload, uint16_t length);
int link_slave_recv(link_slave_piggy_t *piggy);

#endif // _LINK_SLAVE_H_
