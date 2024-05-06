/**
 ****************************************************************************************
 *
 * @file proc.c
 *
 * @brief user procedure.
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "bledef.h"
#include "drvs.h"

#include "app.h"
#include "keys.h"
#include "uartRb.h"
#include "prf_bass.h"
#include "hid_desc.h"
#include "app_user.h"

#if (DBG_PROC)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif

#define BLE_SLP_MS(ms)           ((ms) << 5)

extern void kb_scan(void);
/*
 * DEFINES
 ****************************************************************************************
 */
uint8_t adv_dir_flag = 0;  // 定向广播

static void sleep_proc(void)
{
    uint32_t slpdur = ble_slpdur_get();
      
    if (key_press)
    {
        #if (VOICE)
        if (SADC->CTRL.SADC_DMAC_EN)
        {
            // 语音不睡眠
            keys_proc();
            return;
        }
        #endif
        
        if (slpdur > 625)
        {
            slpdur = 625;
        }
    }

    // > 20ms
    if (slpdur > BLE_SLP_MS(40)/*640*/)
    {
        // Core enter poweroff mode
        if (ble_sleep(200, slpdur) == BLE_IN_SLEEP)
        {
            GPIO_DIR_SET_LO(KEY_COL_MSK);
            wakeup_io_sw(KEY_ROW_MSK, KEY_ROW_MSK);

            core_pwroff(CFG_WKUP_BLE_EN | CFG_WKUP_IO_EN | WKUP_IO_LATCH_N_BIT);
        }
    }
    else if (slpdur > 100)
    {
        // Core enter deepsleep mode
        if (ble_sleep(64, slpdur) == BLE_IN_SLEEP)
        {
            core_sleep(CFG_WKUP_BLE_EN);
        }
        
        keys_proc();
    }
}

void user_procedure(void)
{
    sleep_proc();

    #if (VOICE)
    micPut();
    #endif
}
