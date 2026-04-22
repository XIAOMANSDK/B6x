/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Main Entry of the application.
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "drvs.h"
#include "bledef.h"
#include "app.h"
#include "sftmr.h"
#include "leds.h"
#include "uartRb.h"
#include "atcmd.h"
#include "dbg.h"
#include "cfg.h"
#if (GSM_DECODE_EN)
#include "gsm_audio.h"
#include "gsm_audio_data.h"
#endif
#include "speaker.h"

/*
 * DEFINES
 ****************************************************************************************
 */


/*
 * FUNCTIONS
 ****************************************************************************************
 */

extern void user_procedure(void);

static void sysInit(void)
{ 
    iwdt_disable();

    rcc_ble_en();
    rcc_adc_en();
    SYS_CLK_ALTER();
    //puya_enter_dual_read();
    boya_flash_quad_mode();
    boya_enter_hpm();
    
    //rcc_fshclk_set(FSH_CLK_DPSC42);
    rcc_fshclk_set(FSH_CLK_DPSC64);
}

static void devInit(void)
{
    uint16_t rsn = rstrsn();
    
    uart1Rb_Init();
    dbgInit();
    
    debug("Start(rsn:%X, clk:%d)...\r\n", rsn, rcc_sysclk_freq());
    
    //atConfigFlashRead();
    
    #if ((LED_PLAY) || (CFG_SFT_TMR))
    sftmr_init();
    
    #if (LED_PLAY)
    leds_init();
    leds_play(LED_FAST_BL);
    #endif //(LED_PLAY)
    
    #endif

    // Init BLE App
    app_init(rsn);

#if (GSM_DECODE_EN)
    gsm_audio_init();
    debug("GSM Decoder initialized\r\n");
#endif
    //pwmInit();
}

int main(void)
{
    sysInit();
    devInit();

    // Global Interrupt Enable
    GLOBAL_INT_START();

#if (GSM_DECODE_EN)
    // GSM audio playback with Flash data fallback
    extern const uint8_t gsm_audio_data[];
    #define GSM_AUDIO_DATA_SIZE 3696

    // Check if Flash has valid GSM data
    static uint32_t playback_addr;   // Static to persist in loop
    static uint32_t playback_size;   // Static to persist in loop

    //flash_read(GSM_FLASH_ADDR, (uint32_t *)&flash_data_len, 1);
    playback_size = *(uint32_t *)GSM_FLASH_ADDR;
    // Flash data is valid if not 0xFFFFFFFF and not 0x00000000
    if (playback_size != 0xFFFFFFFF && playback_size != 0)
    {
        // Use Flash data (with length prefix)
        debug("Using Flash GSM data at 0x%08X (size=%d)\r\n", GSM_FLASH_ADDR, playback_size);
        playback_addr = GSM_FLASH_ADDR + sizeof(uint32_t);
    }
    else
    {
        // Flash has no data, use embedded gsm_audio_data
        debug("No valid Flash data, using embedded GSM data (%d bytes)\r\n", GSM_AUDIO_DATA_SIZE);
        playback_addr = (uint32_t)gsm_audio_data;
        playback_size = GSM_AUDIO_DATA_SIZE;
    }
    // Use gsm_play_start_memory for embedded data (loop mode enabled)
    debug("GSM: Starting playback from 0x%08X, size=%d frames\r\n",
          playback_addr, playback_size / 33);

    if (gsm_play_start_memory(playback_addr, playback_size, 1) == 0)
    {
        debug("GSM: Playback started successfully\r\n");
    }
    else
    {
        debug("GSM: Failed to start playback\r\n");
    }
#endif

    // main loop
    while (1)
    {
        // Schedule Messages & Events (BLE stack)
        // This continues to work during GSM audio playback
        ble_schedule();

        #if ((LED_PLAY) || (CFG_SFT_TMR))
        // SoftTimer Polling
        sftmr_schedule();
        #endif //(LED_PLAY)

        // User's Procedure
        user_procedure();

#if (GSM_DECODE_EN)
        // Non-blocking GSM playback poll
        // This advances the state machine one step per call
        // Returns 0 if playing, 1 if complete
        // Note: gsm_play_poll() handles auto-restart for loop mode internally
        gsm_play_poll();
#endif

        // GSM audio playback runs in background via DMA
        // BLE AT commands still available:
        // - AT+GSMPLAY       : Play GSM audio from Flash
        // - AT+GSMPLAY=<addr> : Play from specific address
        // - AT+GSMSTOP       : Stop current playback
        // - AT+GSMSTATUS?    : Query playback status
    }
}
