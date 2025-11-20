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
#define DBG_MODE      (1)

// ====== 用户可修改的硬件映射 ======
#define PA_MOTOR_A        (PA09)
#define PA_MOTOR_B        (PA10)
#define PA_MOTOR_C        (PA11)
#define PA_MOTOR_D        (PA12)

#endif  //_APP_CFG_H_
