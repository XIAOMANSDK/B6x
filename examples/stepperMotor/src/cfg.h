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

/// System Clock (0=16MHz, 1=32MHz, 2=48MHz, 3=64MHz)
#define SYS_CLK             (1)

/// Debug Mode: 0=Disable, 1=via UART, 2=RTT Viewer
#define DBG_MODE            (1)

// Hardware pin mapping
#define PA_MOTOR_A          (PA09)
#define PA_MOTOR_B          (PA10)
#define PA_MOTOR_C          (PA11)
#define PA_MOTOR_D          (PA12)

#endif  //_APP_CFG_H_
