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
#include "app_user.h"

#if (DBG_PROC)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif

#define BLE_SLP_MS(ms)           ((ms) << 5)

// 主机回应语音是否可以开始发送的flag
uint8_t voice_start_flag = 0;

void ble_hid_voice_start(uint8_t streamId)
{
    voice_start_flag = 1;
    uint8_t temp[4] = {0x04,0x03,0x01,0x00}; //启动,HTT,8kHz/16bit,streamId
    temp[3] = streamId;
    sess_txd_send2(0, 4, temp);
}

void ble_hid_voice_stop(uint8_t reason)
{
    uint8_t temp[2] = {0x00,0x00};
    temp[1] = reason;
    sess_txd_send2(0, 2, temp);
}

void sess_cb_rxd(uint8_t conidx, uint16_t len, const uint8_t *data)
{
    debugHex(data, len);
    
    switch(data[0])
    {
        case 0x0A: //GET_CAPS
        {
            //                 响应,版本1.0  ,8kHz/16bit, HTT,      帧大小120
            uint8_t temp[9] = {0x0B,0x01,0x00,0x01       ,0x03,0x00,0x78,0x00,0x00};
            sess_txd_send2(0, 9, temp);  // 按键松开,停止语音发送
        } break;
        
        case 0x0D: //MIC_CLOSE
        {
            // data[1] = streamId
            voice_start_flag = 0;
            ble_hid_voice_stop(0);  // 按键松开,停止语音发送
        } break;
    }
}

static void sleep_proc(void)
{
    uint32_t slpdur = ble_slpdur_get();
    // DEBUG("slpdur=%d\n", slpdur);

    if (keys_active())
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
        if (ble_sleep(640, slpdur) == BLE_IN_SLEEP)
        {
            GPIO_DIR_SET_LO(KEY_COL_MSK);
            wakeup_io_sw(KEY_ROW_MSK, KEY_ROW_MSK);

            // uart_send(0, 1, "c");
            core_pwroff(CFG_WKUP_BLE_EN | CFG_WKUP_IO_EN | WKUP_IO_LATCH_N_BIT);
        }
    }
    else if (slpdur > 100)
    {
        // Core enter deepsleep mode
        if (ble_sleep(64, slpdur) == BLE_IN_SLEEP)
        {
            core_sleep(CFG_WKUP_BLE_EN);
            puya_enter_dual_read();
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
