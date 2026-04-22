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

/// System Clock(0=16MHz, 1=32MHz, 2=48MHz, 3=64MHz)
#define SYS_CLK             (3)


/// Debug Mode: 0=Disable, 1=via UART, 2=RTT Viewer
#define DBG_MODE            (1)

/// LCD Image display, limit by Flash Size 
#define DISP_LOGO           (1)
#define DISP_EYES           (1)
#define DISP_SLIDE          (0)
#define DISP_ROBOT          (0)

#endif  //_APP_CFG_H_
