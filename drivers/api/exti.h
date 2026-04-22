/**
 ****************************************************************************************
 *
 * @file exti.h
 *
 * @brief Header file - EXTI Driver
 *
 ****************************************************************************************
 */

#ifndef _EXTI_H_
#define _EXTI_H_

#include <stdint.h>
#include "reg_exti.h"


/*
 * DEFINES
 ****************************************************************************************
 */

/// Debounce for EXTI: 'psc'(prescaler 0~255 clk cycles) * 'cnt'(count 0~7)
#define EXTI_DBC(psc, cnt)    (((psc) << 8) | (cnt))

/// SourceBit of EXTI: 'pa'(pad PA00~19)
#define EXTI_SRC(pa)          (1UL << (pa))

/// Type of EXTI local src
enum exti_loca_typ
{
    EXTI_IER            = 0,  // w  interrupt enable
    EXTI_IDR            = 1,  // w  interrupt disable
    EXTI_IVS            = 2,  // r  interrupt valid status
    EXTI_RIF            = 3,  // r  raw interrupt flag status
    EXTI_IFM            = 4,  // r  interrupt flag masked status
    EXTI_ICR            = 5,  // w  interrupt clear
    EXTI_RTS            = 6,  // rw rising edge trigger selection
    EXTI_FTS            = 7,  // rw falling edge trigger selection
    EXTI_SWI            = 8,  // rw software interrupt event
    EXTI_ADTE           = 9,  // rw ad trigger enable
    EXTI_DBE            = 10, // rw input debounce enable
};


/*
 * FUNCTION DECLARATION
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Init EXTI, enable clk and rst, config debounce
 *
 * @param[in] dbc  debounce for IO, @see EXTI_DBC(psc, cnt)
 * 
 ****************************************************************************************
 */
void exti_init(uint16_t dbc);

/**
 ****************************************************************************************
 * @brief Deinit EXTI Module. Diable EXTI Clock.
 *
 ****************************************************************************************
 */
void exti_deinit(void);

/**
 ****************************************************************************************
 * @brief Enable Bits Field of BUS Register 
 *
 * @param[in] typ  Type of local src @see enum exti_loca_typ
 * @param[in] loca Value Bits of local src @see EXTI_SRC(pa)
 *
 ****************************************************************************************
 */
void exti_set(uint8_t typ, uint32_t loca);

/**
 ****************************************************************************************
 * @brief Get EXTI local src value
 *
 * @param[in] typ  Type of local src @see enum exti_loca_typ
 *
 * @return local src value
 ****************************************************************************************
 */
uint32_t exti_get(uint8_t typ);


#endif
