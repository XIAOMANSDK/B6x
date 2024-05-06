/**
 ****************************************************************************************
 *
 * @file btns.h
 *
 * @brief Header file - Separate Button Module
 *
 ****************************************************************************************
 */

#ifndef _BTNS_H_
#define _BTNS_H_

#include <stdint.h>


/*
 * DEFINES
 ****************************************************************************************
 */

enum btn_evt {
    BTN_IDLE,

    BTN_PRESS,
    BTN_RELEASE,

    BTN_CLICK,
    BTN_DCLICK, // double click
    BTN_LONG,
    BTN_LLONG,  // long long press
    
    BTN_EVT_MAX
};

typedef void(*btn_func_t)(uint8_t id, uint8_t evt);


/*
 * FUNCTION DECLARATION
 ****************************************************************************************
 */

/// Init iopad and env
void btns_init(void);

/// Conf event handler, disabled when 'hdl' is NULL
void btns_conf(btn_func_t hdl);


#endif  // _BTNS_H_
