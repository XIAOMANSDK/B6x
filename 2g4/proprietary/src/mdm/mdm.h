/**
 ****************************************************************************************
 *
 * @file mdm.h
 *
 * @brief Header file - MDM Driver
 *
 ****************************************************************************************
 */

#ifndef _MDM_H_
#define _MDM_H_

#include <stdint.h>
//#include "rfctrl.h"
/*
 * DEFINES
 ****************************************************************************************
 */

/// MDM interrupt mask

#define MDM_SLOT_N_MASK        ( 0x0FFFFFFF )

#define MDM_INT_TX_DONE        ( 1UL << 0 )
#define MDM_INT_SYNC_ERR       ( 1UL << 1 )
#define MDM_INT_SLOT_INT       ( 1UL << 2 )
#define MDM_INT_SLOT_OVER      ( 1UL << 3 )
#define MDM_INT_SYNC_FOUND     ( 1UL << 4 )

#define MDM_INT_ENABLED        ( MDM_INT_TX_DONE  \
                                | MDM_INT_SYNC_ERR \
                                | MDM_INT_SLOT_INT )

/// MDM rx/tx buffer length
#define MDM_TX_DATA_LEN        ( 38 )
#define MDM_RX_DATA_LEN        ( 32 )

/// MDM default sync timestamp value (no sync found)
#define MDM_SYNC_NONE          ( 0xFFFF )

/// MDM rf rate, the same as definition in rfctrl
enum mdm_rate
{
    MDM_1Mbps     = 0x0,
    MDM_2Mbps     = 0x1,
};

/// MDM callback
typedef struct
{
    void (*slot_begin)(uint32_t slot_n, uint16_t fine_n, int sync_found);
    void (*sync_error)(void);
    void (*rx_done)(uint8_t *data, uint16_t length, uint32_t slot_n, uint16_t fine_n);
    void (*tx_done)(uint32_t slot_n, uint16_t fine_n);
} mdm_cb_t;

/// MDM configuration
typedef struct
{
    // Slot window
    uint16_t slot_win;
    // Slot offset
    uint16_t slot_off;
    // RX sync window
    uint16_t sync_win;
    // Acccess code
    uint32_t access_code;
    // RF rate
    uint8_t rf_rate;
} mdm_conf_t;

/// MDM state
typedef struct
{
    // Slot value at slot begin interrupt
    uint32_t int_slot_n;
    // Fine value at slot begin interrupt
    uint16_t int_fine_n;
    // Slot value at sync found
    uint32_t sync_slot_n;
    // Fine value at sync found
    uint16_t sync_fine_n;
 } mdm_state_t;

/*
 * FUNCTION DECLARATION
 ****************************************************************************************
 */

void mdm_init(mdm_cb_t *cb);

void mdm_conf(mdm_conf_t *conf);

void mdm_retrieve_slot(uint32_t *slot_n, uint16_t *fine_n);
void mdm_wait_slot(uint32_t slot_n, uint16_t fine_n);
void mdm_update_slot(uint32_t slot_off, uint16_t fine_off);

void mdm_rx_enable(uint8_t chn_idx, uint8_t rf_rate);

void mdm_rtx_disable(void);

void mdm_tx_send(uint8_t chn_idx, uint8_t rf_rate,uint32_t access_code,
                 uint8_t *data, uint16_t length);

#endif
