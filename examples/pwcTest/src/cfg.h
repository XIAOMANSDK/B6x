/**
 ****************************************************************************************
 *
 * @file cfg.h
 *
 * @brief Application configuration macros (--preinclude)
 *
 ****************************************************************************************
 */

#ifndef _APP_CFG_H_
#define _APP_CFG_H_

/// Debug Mode: 0=Disable, 1=via UART, 2=RTT Viewer
#define DBG_MODE            (1)

/// Timer selection: 1=CTMR (general purpose), 0=ATMR (advanced)
#define CTMR_USED           (1)

#endif /* _APP_CFG_H_ */
