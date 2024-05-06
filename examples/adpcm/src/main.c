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
#include "adpcm.h"
#include "dbg.h"


/*
 * DEFINES
 ****************************************************************************************
 */



/*
 * FUNCTIONS
 ****************************************************************************************
 */

static void sysInit(void)
{    
    // Todo config, if need
    
}

static void devInit(void)
{
    iwdt_disable();
    
    dbgInit();

    debug("ADPCM Test...\r");
}

static void adpcmTest(void)
{
    struct adpcm_state state;
    int iRead = 128;
    uint16_t rawBuff[128] = {0};
    uint8_t  wavBuff[64] = {0};
    
    GPIO_DIR_SET_LO(GPIO08);
    for (uint16_t i = 0; i < 128; i++)
    {
        rawBuff[i] = 0x08*i;
    }
    debug("RAW0:\r\n");
    debugHex(rawBuff, 128);
    
    state.index = 0;
    state.valprev = 0;
    GPIO_DAT_SET(GPIO08);
    adpcm_coder((short*)rawBuff, (char*)wavBuff, (iRead), &state);
    GPIO_DAT_CLR(GPIO08);
    debug("WAV0(%d,%d):\r\n", state.index, state.valprev); 
    debugHex(wavBuff, 64);
    
    state.index = 0;
    state.valprev = 0;
    memset(rawBuff, 0, sizeof(rawBuff));
    GPIO_DAT_SET(GPIO08);
    adpcm_decoder((char*)wavBuff, (short*)rawBuff, (iRead), &state);
    GPIO_DAT_CLR(GPIO08);
    debug("RAW1(%d,%d):\r\n", state.index, state.valprev); 
    debugHex(rawBuff, 128);

    state.index = 0;
    state.valprev = 0;
    GPIO_DAT_SET(GPIO08);
    adpcm_coder((short*)rawBuff, (char*)wavBuff, (iRead), &state);
    GPIO_DAT_CLR(GPIO08);
    debug("WAV1(%d,%d):\r\n", state.index, state.valprev); 
    debugHex(wavBuff, 64);
}

int main(void)
{
    sysInit();
    devInit();

    adpcmTest();
    
    while(1)
    {

    }
}
