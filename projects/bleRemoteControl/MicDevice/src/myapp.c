/**
 ****************************************************************************************
 *
 * @file myapp.c
 *
 * @brief User Application - Override func
 *
 ****************************************************************************************
 */

#include "app.h"
#include "bledef.h"
#include "drvs.h"
#include "leds.h"

#if (DBG_APP)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat,len)
#endif


/*
 * DEFINES
 ****************************************************************************************
 */
#ifndef CFG_LTK_STORE
    #define CFG_LTK_STORE      (1)
#endif

#ifndef LTK_STORE_OFFSET
    #define LTK_STORE_OFFSET   (0x1100)
#endif

#if (LTK_STORE_OFFSET < 0x1000)
    #error "User Store Data Offset Must Greater Than or Equal 0x1000"
#endif

struct gapc_ltk gLTK;

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

#if (LED_PLAY)
/**
 ****************************************************************************************
 * @brief API to Set State of Application, add leds Indication
 *
 * @param[in] state    new state
 ****************************************************************************************
 */
void app_state_set(uint8_t state)
{
    DEBUG("State(old:%d,new:%d)", app_state_get(), state);

    app_env.state = state;
    
    // Indication, User add more...
    if (state == APP_IDLE)
    {
        leds_play(LED_SLOW_BL);
    }
    else if (state == APP_READY)
    {
        leds_play(LED_FAST_BL);
    }
    else if (state == APP_CONNECTED)
    {
        leds_play(LED_CONT_ON);
    }
}
#endif //(LED_PLAY)

/**
 ****************************************************************************************
 * @brief API to Generate LTK for bonding, maybe User Override! (__weak func)
 *
 * @param[in]     conidx   connection index
 * @param[in|out] ltk      Pointer of ltk buffer
 ****************************************************************************************
 */
void app_ltk_gen(uint8_t conidx, struct gapc_ltk *ltk)
{
    DEBUG("LTK Gen");

    // Generate all the values
    gLTK.key_size = GAP_KEY_LEN;
    gLTK.ext_info = 0;
    
    gLTK.ediv = rand_hword();
    
    for (int i = 0; i < GAP_RAND_NB_LEN; i++)
    {
        gLTK.ltk.key[i]     = (uint8_t)rand_word();
        gLTK.ltk.key[i + 8] = (uint8_t)rand_word();
        gLTK.randnb.nb[i]   = (uint8_t)rand_word();
    }

    memcpy(ltk, &gLTK, sizeof(struct gapc_ltk));
    debugHex((uint8_t *)ltk, sizeof(struct gapc_ltk));
}

/**
 ****************************************************************************************
 * @brief API to Save LTK when bonded, maybe User Override! (__weak func)
 *
 * @param[in] conidx   connection index
 * @param[in] ltk      Pointer of LTK data
 ****************************************************************************************
 */
void app_ltk_save(uint8_t conidx, const struct gapc_ltk *ltk)
{
    DEBUG("LTK Saved Start");
    
    #if (CFG_LTK_STORE)
    flash_page_erase(LTK_STORE_OFFSET);
    flash_byte_write(LTK_STORE_OFFSET, (uint8_t *)&gLTK, sizeof(struct gapc_ltk));
    #endif
    
    DEBUG("LTK Saved Done");
}

/**
 ****************************************************************************************
 * @brief API to Find LTK when re-encryption, maybe User Override! (__weak func)
 *
 * @param[in] ediv     EDIV value for matching
 * @param[in] rand_nb  Rand Nb values for matching
 *
 * @return NULL for not matched, else return Pointer of LTK found.
 ****************************************************************************************
 */
const uint8_t *app_ltk_find(uint16_t ediv, const uint8_t *rand_nb)
{
    DEBUG("Read LTK");
    
#if (CFG_LTK_STORE)
    flash_byte_read(LTK_STORE_OFFSET, (uint8_t *)&gLTK, sizeof(struct gapc_ltk));
    debugHex((uint8_t *)&gLTK, sizeof(struct gapc_ltk));
#endif

    if ((ediv == gLTK.ediv) && (memcmp(rand_nb, gLTK.randnb.nb, 8) == 0))
        return &gLTK.ltk.key[0];
    else
        return NULL;
}

// 删除配对信息
void deletePairInfo(void)
{
    memset(&gLTK, 0xff, sizeof(struct gapc_ltk));
    // 清空LTK的FLASH空间
//    flash_erase_write(LTK_STORE_OFFSET, (uint32_t *)ltk_Data);
    flash_page_erase(LTK_STORE_OFFSET);
}
