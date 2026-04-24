/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief ADPCM audio encoder/decoder test
 *
 * @details
 * Test flow:
 * 1. Generate a linear ramp as test PCM input
 * 2. Encode PCM -> ADPCM (compress 128 samples to 64 bytes)
 * 3. Decode ADPCM -> PCM (restore 128 samples)
 * 4. Re-encode restored PCM to verify round-trip
 * GPIO08 pulses mark encode/decode duration for scope measurement.
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "drvs.h"
#include "adpcm.h"
#include "dbg.h"


/*
 * DEFINES
 ****************************************************************************************
 */

/// Number of PCM samples for each encode/decode pass
#define ADPCM_SAMPLE_COUNT  (128)

/// ADPCM output size: 4 bits per sample -> SAMPLE_COUNT / 2 bytes
#define ADPCM_ENCODED_SIZE  (ADPCM_SAMPLE_COUNT / 2)

/// Linear ramp step for test input generation
#define RAMP_STEP           (0x08)

/// GPIO pin for encode/decode timing measurement
#define GPIO_ADPCM          GPIO08

/*
 * FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief System initialization
 ****************************************************************************************
 */
static void sysInit(void)
{
    // TODO: Add system clock configuration if needed
}

/**
 ****************************************************************************************
 * @brief Device initialization
 ****************************************************************************************
 */
static void devInit(void)
{
    iwdt_disable();

    dbgInit();
    debug("ADPCM Test...\r\n");
}

/**
 ****************************************************************************************
 * @brief Run ADPCM encode/decode round-trip test
 *
 * @details
 * Generate ramp data -> encode -> decode -> re-encode, printing results
 * at each stage for verification.
 ****************************************************************************************
 */
static void adpcmTest(void)
{
    struct adpcm_state state;
    uint16_t rawBuff[ADPCM_SAMPLE_COUNT] = {0};
    uint8_t  wavBuff[ADPCM_ENCODED_SIZE] = {0};

    GPIO_DIR_SET_LO(GPIO_ADPCM);

    /* Generate linear ramp as test input */
    for (uint16_t i = 0; i < ADPCM_SAMPLE_COUNT; i++)
    {
        rawBuff[i] = RAMP_STEP * i;
    }
    debug("RAW0:\r\n");
    debugHex(rawBuff, ADPCM_SAMPLE_COUNT);

    /* Encode: PCM (16-bit) -> ADPCM (4-bit) */
    state.index = 0;
    state.valprev = 0;
    GPIO_DAT_SET(GPIO_ADPCM);
    adpcm_coder((short *)rawBuff, (char *)wavBuff, ADPCM_SAMPLE_COUNT, &state);
    GPIO_DAT_CLR(GPIO_ADPCM);
    debug("WAV0(%d,%d):\r\n", state.index, state.valprev);
    debugHex(wavBuff, ADPCM_ENCODED_SIZE);

    /* Decode: ADPCM (4-bit) -> PCM (16-bit) */
    state.index = 0;
    state.valprev = 0;
    memset(rawBuff, 0, sizeof(rawBuff));
    GPIO_DAT_SET(GPIO_ADPCM);
    adpcm_decoder((char *)wavBuff, (short *)rawBuff, ADPCM_SAMPLE_COUNT, &state);
    GPIO_DAT_CLR(GPIO_ADPCM);
    debug("RAW1(%d,%d):\r\n", state.index, state.valprev);
    debugHex(rawBuff, ADPCM_SAMPLE_COUNT);

    /* Re-encode decoded output for round-trip verification */
    state.index = 0;
    state.valprev = 0;
    GPIO_DAT_SET(GPIO_ADPCM);
    adpcm_coder((short *)rawBuff, (char *)wavBuff, ADPCM_SAMPLE_COUNT, &state);
    GPIO_DAT_CLR(GPIO_ADPCM);
    debug("WAV1(%d,%d):\r\n", state.index, state.valprev);
    debugHex(wavBuff, ADPCM_ENCODED_SIZE);
}

/**
 ****************************************************************************************
 * @brief Application entry point
 ****************************************************************************************
 */
int main(void)
{
    sysInit();
    devInit();

    adpcmTest();

    while (1)
    {
    }
}
