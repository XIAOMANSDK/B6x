/**
 ****************************************************************************************
 *
 * @file iwdt.h
 *
 * @brief Header file - WatchDog Timer Driver
 *
 ****************************************************************************************
 */

#ifndef _IWDT_H_
#define _IWDT_H_

#include <stdint.h>
#include <stdbool.h>


/*
 * DEFINES
 ****************************************************************************************
 */

enum iwdt_ctrl_bfs
{
    IWDT_EN_BIT            = (1 << 0), // enable
    IWDT_INTEN_BIT         = (1 << 1), // interrupt enable
    IWDT_RSTEN_BIT         = (1 << 2), // reset enable
    IWDT_CLKLSI_BIT        = (1 << 3), // clk select lsi
};

#define IWDT_CR_DFLT         (IWDT_EN_BIT | IWDT_RSTEN_BIT | IWDT_CLKLSI_BIT)

#define iwdt_disable()       iwdt_conf(0)

/*
 * FUNCTION DECLARATION
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief Init Config IWDT.
 *
 * @param[in] ctrl  Bits field of value @see enum iwdt_ctrl_bfs.
 ****************************************************************************************
 */
void iwdt_init(uint8_t ctrl);

/**
 ****************************************************************************************
 * @brief Deinit IWDT Module.
 *
 ****************************************************************************************
 */
void iwdt_deinit(void);

/**
 ****************************************************************************************
 * @brief reload IWDT.
 ****************************************************************************************
 */
void iwdt_feed(void);

#if (ROM_UNUSED)
/**
 ****************************************************************************************
 * @brief Config IWDT, feed or disable 
 *
 * @param[in] load  LOAD to feed watchdog, 0 means disable
 *
 * @return Timer value
 ****************************************************************************************
 */
uint32_t iwdt_conf(uint32_t load);
#endif

#endif // _IWDT_H_
