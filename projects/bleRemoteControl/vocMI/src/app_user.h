#ifndef _APP_USER_H_
#define _APP_USER_H_

#include <stdint.h>
#include <stdbool.h>

#include "regs.h"
#include "micphone.h"
#include "app.h"
#include "keys.h"
#include "sadc.h"
#include "pt.h"
/***************************************/
/*************** KEY CFG ***************/
/***************************************/
/// Number of Key Row and Column
#if (KEY_HY)
#define KEY_C0                 PA04
#define KEY_C1                 PA12
#define KEY_C2                 PA11
#define KEY_C3                 PA10
        
#define KEY_R0                 PA09
#define KEY_R1                 PA05
#define KEY_R2                 PA08
#define KEY_R3                 PA13

/// Macro for Number of keyboard row&column
#define KEY_ROW_NB              (4)
#define KEY_COL_NB              (4)
#define KEY_COL_MSK (BIT(KEY_C0)|BIT(KEY_C1)|BIT(KEY_C2)|BIT(KEY_C3))
#define KEY_ROW_MSK (BIT(KEY_R0)|BIT(KEY_R1)|BIT(KEY_R2)|BIT(KEY_R3))
#else
#define KEY_C0                 PA08
#define KEY_C1                 PA12
#define KEY_C2                 PA05
#define KEY_C3                 PA09

#define KEY_R0                 PA10
#define KEY_R1                 PA13
#define KEY_R2                 PA11

/// Macro for Number of keyboard row&column
#define KEY_ROW_NB              (3)
#define KEY_COL_NB              (4)
#define KEY_COL_MSK (BIT(KEY_C0)|BIT(KEY_C1)|BIT(KEY_C2)|BIT(KEY_C3))
#define KEY_ROW_MSK (BIT(KEY_R0)|BIT(KEY_R1)|BIT(KEY_R2))
#endif
/***************************************/

extern bool last_sta;
extern uint16_t g_no_action_cnt;
extern const struct gapc_conn_param dft_conn_param;
extern bool key_press;
extern uint16_t voiceSendNB; 
extern uint16_t voiceSendOK;

void app_conn_param_update(bool key_change);
void deletePairInfo(void);
#endif // _APP_USER_H_
