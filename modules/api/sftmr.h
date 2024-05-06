/**
 ****************************************************************************************
 *
 * @file sftmr.h
 *
 * @brief Header file - Soft Timer Module
 *
 ****************************************************************************************
 */

#ifndef _SFTMR_H_
#define _SFTMR_H_

#include <stdint.h>


/*
 * DEFINES
 ****************************************************************************************
 */

/// Types of timer id, valid range 1 ~ num.
typedef uint8_t             tmr_id_t;

/// Types of timer-tick, 16bits enough.
typedef uint16_t            tmr_tk_t;

/// Callback function, continue mode(>0) or single mode(0).
typedef tmr_tk_t (*tmr_cb_t)(tmr_id_t tmid);

/// timer ID: 0 - Invalid, 1 ~ num Valid
#define TMR_ID_NONE         (0x00)

/// Maximum timer value (Not exceed tmr_tk_t range)
#if !defined(SFTMR_TICKS_MSK)
#define SFTMR_TICKS_MSK     (0xFFFF)
#endif

#define SFTMR_DELAY_MAX     (SFTMR_TICKS_MSK >> 1)

/// Convert to timer accuracy (in uint of 10ms)
#if !defined(TMR_UINT)
#define TMR_UINT            (10)
#endif

#if !defined(_MS)
#define _MS(n)              ((n) / TMR_UINT)
#endif

/*
 * FUNCTION DECLARATION
 ****************************************************************************************
 */

/// Init timer source
void sftmr_init(void);

/// Schedule timer event of callback.
void sftmr_schedule(void);

/// Start timer, callback 'func' after 'delay' ticks post.
tmr_id_t sftmr_start(tmr_tk_t delay, tmr_cb_t func);

/// Clear/Free 'tmr_id' timer instance
void sftmr_clear(tmr_id_t tmr_id);

/// Get current ticks
tmr_tk_t sftmr_tick(void);

/// Blocking to wait 'delay' ticks arrived
void sftmr_wait(tmr_tk_t delay);


#endif // _SFTMR_H_
