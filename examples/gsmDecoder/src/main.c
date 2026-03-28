/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Main Entry of the application - GSM Decoder Test
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "drvs.h"
#include "dbg.h"
#include "gsm.h"
#include "gsm_audio_data.h"
#include <string.h>


/*
 * DEFINES
 ****************************************************************************************
 */
const uint8_t* gsm_data;
uint32_t gsm_data_len = 5000;

#define GSM_FRAME_SIZE     33
#define GSM_PCM_SAMPLES    160
#define GSM_TOTAL_FRAMES   (sizeof(gsm_audio_data) / GSM_FRAME_SIZE)

/*
 * FUNCTIONS
 ****************************************************************************************
 */

static void sysInit(void)
{
    iwdt_disable();

    //rcc_ble_en();
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

    dbgInit();
    debug("Start(rsn:0x%X)...\r\n", rsn);
    GPIO_DIR_SET(GPIO14);
    iom_ctrl(PA14, IOM_SEL_GPIO);
    GPIO_DIR_SET(GPIO15);
    iom_ctrl(PA15, IOM_SEL_GPIO);
}

extern void speakerPlay(int16_t *buff, uint16_t samples);
extern const uint16_t audio_data_500hz[16000];
static void userProc(void)
{
    static gsm_state_t decoder;
    static int16_t pcm_output[2][GSM_PCM_SAMPLES];
    static uint8_t decode_done = 0;
    
//    while(1)
//    {
//        static int16_t frame_cnt = 0;
//        speakerPlay(&audio_data_500hz[frame_cnt * GSM_PCM_SAMPLES], GSM_PCM_SAMPLES);
//        frame_cnt++;
//        if(frame_cnt > 100) 
//            while(1); 
//        
//    }
    /* Decode only once */
    if (!decode_done)
    {
        /* Initialize GSM decoder */
        memset(&decoder, 0, sizeof(decoder));
        decoder.nrp = 40;

        debug("\r\n");
        debug("========================================\r\n");
        debug("GSM 6.10 Decoder Test\r\n");
        debug("========================================\r\n");
        debug("Audio data size: %d bytes\r\n", gsm_data_len);
        debug("Total frames: %d\r\n", gsm_data_len/GSM_FRAME_SIZE);
        debug("Frame size: %d bytes\r\n", GSM_FRAME_SIZE);
        debug("PCM samples per frame: %d\r\n", GSM_PCM_SAMPLES);
        debug("========================================\r\n");
        debug("\r\n");

        /* Decode all frames */
        for (uint32_t frame_count = 0; frame_count < gsm_data_len/GSM_FRAME_SIZE; frame_count++)
        {
            const uint8_t *frame_data = &gsm_data[frame_count * GSM_FRAME_SIZE];

            /* Decode the GSM frame */
            GPIO_DAT_SET(GPIO14);
            gsm_decode_frame(&decoder, (uint8_t *)frame_data, pcm_output[frame_count%2]);
            GPIO_DAT_CLR(GPIO14);
            /* Print frame information */
//            debug("Frame %4d: ", frame_count + 1);

//            /* Print all 160 PCM samples */
//            for (int i = 0; i < GSM_PCM_SAMPLES; i++)
//            {
//                debug("0x%04x ", (uint16_t)pcm_output[frame_count%2][i]);
//            }
//            debug("\r\n");
            
            GPIO_DAT_SET(GPIO15);
            speakerPlay(pcm_output[frame_count%2], GSM_PCM_SAMPLES);
            GPIO_DAT_CLR(GPIO15);
        }

        /* All frames decoded */
        debug("\r\n");
        debug("========================================\r\n");
        debug("Decoding complete!\r\n");
        debug("Total frames decoded: %d\r\n", gsm_data_len/GSM_FRAME_SIZE);
        debug("Total PCM samples: %d\r\n", gsm_data_len/GSM_FRAME_SIZE * GSM_PCM_SAMPLES);
        debug("========================================\r\n");
        debug("\r\n");

        decode_done = 1;
    }

    /* Enter infinite loop after decoding */
    while (1)
    {
        /* Stay here */
    }
}
extern void pwmInit(void);

int main(void)
{
    sysInit();
    devInit();
    pwmInit();
    
    gsm_data = (const uint8_t*)0x18010000;// 预留前面64KB空间给代码
    if((*(uint32_t *)gsm_data ==  0xffffffff) || (*(uint32_t *)gsm_data ==  0)){
        gsm_data = gsm_audio_data;
        gsm_data_len = sizeof(gsm_audio_data);
    }else{
        gsm_data_len = *(uint32_t *)gsm_data;
        gsm_data += sizeof(uint32_t);
    }
    debug("Decoder gsm file size is %d bytes...\r\n", gsm_data_len);
    
    while (1)
    {
        userProc();
    }
}
