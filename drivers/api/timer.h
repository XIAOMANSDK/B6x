/**
 ****************************************************************************************
 *
 * @file timer.h
 *
 * @brief Header file - Timers Driver
 *
 ****************************************************************************************
 */

#ifndef _TIMER_H_
#define _TIMER_H_

#include <stdint.h>
#include "reg_timer.h"


/*
 * DEFINES
 ****************************************************************************************
 */

enum tmr_ctrl_bfs
{
    TMR_CR_CEN_BIT         = (1 << 0),  // Counter Enable
    TMR_CR_UDIS_BIT        = (1 << 1),  // Update Disable
    TMR_CR_URS_BIT         = (1 << 2),  // Update Request Source
    TMR_CR_OPM_BIT         = (1 << 3),  // One Pluse Mode
    TMR_CR_DIR_BIT         = (1 << 4),  // Direction
    TMR_CR_CMS_LSB         = 5,         // Center-aligned Mode Select - bit[6:5]
    TMR_CR_CMS_MSK         = (0x3 << TMR_CR_CMS_LSB),
    TMR_CR_ARPE_BIT        = (1 << 7),  // Auto-Reload Preload Enable
    TMR_CR_CKD_LSB         = 8,         // Clock Division - bit[9:8]
    TMR_CR_CKD_MSK         = (0x3 << TMR_CR_CKD_LSB),
    TMR_CR_CMPSEL_LSB      = 10,        // Compare input Select (Only ADTMR) - bit[11:10]
    TMR_CR_CMPSEL_MSK      = (0x3 << TMR_CR_CMPSEL_LSB),
};

enum tmr_intr_bfs
{
    TMR_IR_NONE            = 0,
    TMR_IR_UI_BIT          = (1 << 0),  // Update Interrupt
    TMR_IR_CC1_BIT         = (1 << 1),  // Capture/Compare 1 Interrupt
    TMR_IR_CC2_BIT         = (1 << 2),  // Capture/Compare 2 Interrupt
    TMR_IR_CC3_BIT         = (1 << 3),  // Capture/Compare 3 Interrupt
    TMR_IR_CC4_BIT         = (1 << 4),  // Capture/Compare 4 Interrupt
    TMR_IR_COM_BIT         = (1 << 5),  // COM Interrupt
    TMR_IR_TI_BIT          = (1 << 6),  // Trigger Interrupt
    TMR_IR_BI_BIT          = (1 << 7),  // Break Interrupt
    
    TMR_IR_CC1O_BIT        = (1 << 9),  // Capture/Compare 1 Overcapture
    TMR_IR_CC2O_BIT        = (1 << 10), // Capture/Compare 2 Overcapture
    TMR_IR_CC3O_BIT        = (1 << 11), // Capture/Compare 3 Overcapture
    TMR_IR_CC4O_BIT        = (1 << 12), // Capture/Compare 4 Overcapture
};

enum tmr_mode_def
{
    TMR_DISABLE           = 0,
    TMR_1PULSE_MODE       = (TMR_CR_CEN_BIT | TMR_CR_URS_BIT | TMR_CR_OPM_BIT),
    TMR_PERIOD_MODE       = (TMR_CR_CEN_BIT | TMR_CR_ARPE_BIT),
};

/*
 * FUNCTION DECLARATION
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Init Common Timer.
 *
 * @param[in] psc  configure prescaler
 * @param[in] arr  configure Auto-reload
 * 
 ****************************************************************************************
 */
void ctmr_init(uint16_t psc, uint16_t arr);

/**
 ****************************************************************************************
 * @brief Deinit Common Timer.
 * 
 ****************************************************************************************
 */
void ctmr_deinit(void);

/**
 ****************************************************************************************
 * @brief Common Timer control.
 *
 * @param[in] mode  Work Mode configure @see enum tmr_mode_def
 * @param[in] intr  Interrupt configure @see enum tmr_intr_bfs
 * 
 ****************************************************************************************
 */
void ctmr_ctrl(uint16_t mode, uint16_t intr);

/**
 ****************************************************************************************
 * @brief Init Advance Timer.
 *
 * @param[in] psc  configure prescaler
 * @param[in] arr  configure Auto-reload
 * 
 ****************************************************************************************
 */
void atmr_init(uint16_t psc, uint16_t arr);

/**
 ****************************************************************************************
 * @brief Deinit Advance Timer.
 *
 ****************************************************************************************
 */
void atmr_deinit(void);

/**
 ****************************************************************************************
 * @brief Advance Timer control.
 *
 * @param[in] mode  Work Mode configure @see enum tmr_mode_def
 * @param[in] intr  Interrupt configure @see enum tmr_intr_bfs
 * 
 ****************************************************************************************
 */
void atmr_ctrl(uint16_t mode, uint16_t intr);

#endif // _TIMER_H_
