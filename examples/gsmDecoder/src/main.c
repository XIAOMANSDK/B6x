/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief GSM 6.10 decoder test - decode GSM frames and play via PWM speaker
 *
 * @details
 * Test flow:
 * 1. Initialize system clock (64 MHz), flash, debug, GPIO
 * 2. Load GSM audio data from external flash or built-in array
 * 3. Decode each 33-byte GSM frame to 160 PCM samples
 * 4. Play PCM output via DMA-driven PWM speaker
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

/// GSM frame size in bytes (compressed)
#define GSM_FRAME_SIZE       (33)

/// PCM samples per decoded GSM frame
#define GSM_PCM_SAMPLES      (160)

/// External flash data region base address
#define EXT_FLASH_DATA_ADDR  ((const uint8_t *)0x18010000UL)

/// Sentinel values indicating empty external flash
#define FLASH_EMPTY_WORD     (0xFFFFFFFFUL)
#define FLASH_ZERO_WORD      (0x00000000UL)

/*
 * VARIABLES
 ****************************************************************************************
 */

static const uint8_t *gsm_data;        ///< Pointer to GSM compressed data
static uint32_t gsm_data_len = 0;      ///< GSM data length in bytes

/*
 * FUNCTIONS
 ****************************************************************************************
 */

extern void pwmInit(void);
extern void speakerPlay(int16_t *buff, uint16_t samples);

/**
 ****************************************************************************************
 * @brief System initialization - clock, flash, peripherals
 ****************************************************************************************
 */
static void sysInit(void)
{
    iwdt_disable();

    rcc_adc_en();
    SYS_CLK_ALTER();

    boya_flash_quad_mode();
    boya_enter_hpm();

    rcc_fshclk_set(FSH_CLK_DPSC64);
}

/**
 ****************************************************************************************
 * @brief Device initialization - debug, GPIO
 ****************************************************************************************
 */
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

/**
 ****************************************************************************************
 * @brief Decode all GSM frames and play through speaker
 *
 * @details
 * Single-shot decode: initializes GSM state, decodes all frames with
 * double-buffered PCM output, and plays each frame via DMA speaker.
 ****************************************************************************************
 */
static void userProc(void)
{
    static gsm_state_t decoder;
    static int16_t pcm_output[2][GSM_PCM_SAMPLES];
    static bool decode_done = false;

    if (decode_done)
    {
        while (1)
        {
        }
    }

    /* Initialize GSM decoder */
    memset(&decoder, 0, sizeof(decoder));
    decoder.nrp = 40;

    debug("\r\n");
    debug("========================================\r\n");
    debug("GSM 6.10 Decoder Test\r\n");
    debug("========================================\r\n");
    debug("Audio data size: %d bytes\r\n", gsm_data_len);
    debug("Total frames: %d\r\n", gsm_data_len / GSM_FRAME_SIZE);
    debug("Frame size: %d bytes\r\n", GSM_FRAME_SIZE);
    debug("PCM samples per frame: %d\r\n", GSM_PCM_SAMPLES);
    debug("========================================\r\n");
    debug("\r\n");

    /* Decode all frames */
    uint32_t total_frames = gsm_data_len / GSM_FRAME_SIZE;

    for (uint32_t i = 0; i < total_frames; i++)
    {
        const uint8_t *frame_data = &gsm_data[i * GSM_FRAME_SIZE];

        GPIO_DAT_SET(GPIO14);
        gsm_decode_frame(&decoder, (uint8_t *)frame_data, pcm_output[i % 2]);
        GPIO_DAT_CLR(GPIO14);

        GPIO_DAT_SET(GPIO15);
        speakerPlay(pcm_output[i % 2], GSM_PCM_SAMPLES);
        GPIO_DAT_CLR(GPIO15);
    }

    debug("\r\n");
    debug("========================================\r\n");
    debug("Decoding complete!\r\n");
    debug("Total frames decoded: %d\r\n", total_frames);
    debug("Total PCM samples: %d\r\n", total_frames * GSM_PCM_SAMPLES);
    debug("========================================\r\n");
    debug("\r\n");

    decode_done = true;
}

/**
 ****************************************************************************************
 * @brief Application entry point
 *
 * @details
 * Load GSM data from external flash if valid, otherwise use built-in audio data.
 ****************************************************************************************
 */
int main(void)
{
    sysInit();
    devInit();
    pwmInit();

    /* Try external flash data region first */
    gsm_data = EXT_FLASH_DATA_ADDR;

    if ((*(const uint32_t *)gsm_data == FLASH_EMPTY_WORD)
        || (*(const uint32_t *)gsm_data == FLASH_ZERO_WORD))
    {
        gsm_data = gsm_audio_data;
        gsm_data_len = sizeof(gsm_audio_data);
    }
    else
    {
        gsm_data_len = *(const uint32_t *)gsm_data;
        gsm_data += sizeof(uint32_t);
    }

    debug("Decoder gsm file size is %d bytes...\r\n", gsm_data_len);

    while (1)
    {
        userProc();
    }
}
