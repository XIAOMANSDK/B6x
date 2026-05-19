/**
 ****************************************************************************************
 *
 * @file cfg.h
 *
 * @brief App Configure MACRO, --preinclude
 *
 ****************************************************************************************
 */

#ifndef _APP_CFG_H_
#define _APP_CFG_H_

/// Debug Mode: 0=Disable, 1=via UART, 2=RTT Viewer
#define DBG_MODE            (1)

#define LI_BATT_MODE        (0)
#define DCDC_MODE           (0)
#define EXT_ADC_IO          ((LI_BATT_MODE) || (DCDC_MODE))

#define SADC_BATTERY_IO     (PA13)

#endif  //_APP_CFG_H_
