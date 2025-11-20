#include "app_user.h"
#include "drvs.h"
#include "regs.h"
#include "string.h"
#include "sadc.h"
#include "b6x.h"
#include "bledef.h"
#include "prf_hids.h"
#include "app.h"
#include "app_user.h"
#include "msbc.h"
#include "adpcm.h"

#if (VOICE)

#undef __ATTR_SRAM
#define __ATTR_SRAM         __attribute__((section("ram_func"), zero_init))

#define MIC_IB_PAD 2
#define MIC_IN_PAD 3

#define DMA_PCM_CHAN   DMA_CH0

#ifndef ADPCM_BLOCK_SIZE
#define ADPCM_BLOCK_SIZE        (128)
#endif

#define SAMPLE_DATA_SIZE        (ADPCM_BLOCK_SIZE - 4)
#define READ_UNIT_SIZE          (SAMPLE_DATA_SIZE * 4 + 2)

struct ADPCMBlock
{
    short sample0;
    char index;
    char RESERVED;
    char sampledata[SAMPLE_DATA_SIZE];
};

#if (XIAOMI)
#define PCM_SAMPLE_NB   (216)     // 256: 32ms  8000Hz
#else
#define PCM_SAMPLE_NB   (READ_UNIT_SIZE/2)     // 256: 32ms  8000Hz
#endif

extern uint8_t voice_start_flag;

__ATTR_SRAM int16_t pcm_buff0[PCM_SAMPLE_NB];
__ATTR_SRAM int16_t pcm_buff1[PCM_SAMPLE_NB];

__ATTR_SRAM struct ADPCMBlock adpcm_buff;
struct adpcm_state state;

void uartTxSend(const uint8_t *data, uint16_t len)
{
    while(len--)
    {
        uart_putc(0, *data++);
    }
}

uint16_t voiceSendNB; 
uint16_t voiceSendOK;
uint8_t voiceSendFt;

static void rc_voice_init(void)
{
    msbc_estart();

    state.index = 0;
    state.valprev = 0;
}

/**
 * @brief pcm压缩、打包、发送
 * 
 * @param[in] sample_buf pcm数据
 * @param[in] len 长度，等于MSBC_NB_PCM8K
 */
__SRAMFN static void ble_adpcm_encode_package_send(int16_t *sample_buf, int len)
{
    adpcm_buff.sample0 = sample_buf[0];
    state.valprev = sample_buf[0];
    adpcm_coder((short*)&sample_buf[1], (char *)adpcm_buff.sampledata, (len - 1), &state);        

    voiceSendNB++;
    if (sess_txd_send(0, ADPCM_BLOCK_SIZE, (uint8_t *)&adpcm_buff) == 0)
    {
        voiceSendOK++;
    }
}

void micInit(void)
{
    rc_voice_init();

    GPIO_DIR_CLR(GPIO02 | GPIO03);
    iom_ctrl(MIC_IB_PAD, IOM_HIZ);
    iom_ctrl(MIC_IN_PAD, IOM_ANALOG);   // 5 

    // sadc init
    sadc_init(SADC_ANA_DFLT | SADC_INBUF_BYPSS_BIT);

    dma_init();

    // DMA Conf: direct Init
//    SADC->CTRL.SADC_DMAC_EN = 0;      
    
    DMA_SADC_INIT(DMA_PCM_CHAN);
    DMA_SADC_PCM_CONF(DMA_PCM_CHAN, pcm_buff0, PCM_SAMPLE_NB, CCM_PING_PONG);
    DMA_SADC_PCM_CONF(DMA_PCM_CHAN | DMA_CH_ALT, pcm_buff1, PCM_SAMPLE_NB, CCM_PING_PONG);
    sadc_conf((SADC_CR_DFLT & ~(SADC_CR_HPF_COEF_MSK|SADC_CR_CLK_DIV_MSK)) | SADC_CR_HPF(3) | SADC_CR_CLK(SYS_CLK));
    sadc_pcm(SADC_MIC_DFLT & (~SADC_PGA_VOL_MSK | SADC_PGA_VOL(0)));

    voiceSendNB = 0; 
    voiceSendOK = 0;
    voiceSendFt = 4;
}

void micDeinit(void)
{
    GPIO_DIR_CLR(GPIO02 | GPIO03);
    iom_ctrl(MIC_IB_PAD, IOM_HIZ);
    iom_ctrl(MIC_IN_PAD, IOM_HIZ);

    // disable SADC, reduce power
    SADC->CTRL.Word          = 0;
    SADC->MIC_CTRL.Word      = SADC_MIC_PWD_BIT;
    SADC->SADC_ANA_CTRL.Word = 0;
    dma_chnl_ctrl(DMA_PCM_CHAN, CHNL_DIS);
    dma_chnl_deinit(DMA_PCM_CHAN);
    dma_deinit();

    voiceSendNB = 0; 
    voiceSendOK = 0;
    voiceSendFt = 4;
}

void micPut(void)
{
    // primary or alternate transfer done
    if (dma_chnl_done(DMA_PCM_CHAN))
    {   
        if (voiceSendFt)
        {
            // 过滤
            dma_chnl_reload(DMA_PCM_CHAN);
            voiceSendFt--;
            return;
        }
        
        int16_t *pbuff = NULL;
        if (dma_chnl_reload(DMA_PCM_CHAN))
            pbuff = pcm_buff1;
        else
            pbuff = pcm_buff0;

        if (voice_start_flag != 0x01)
            return;

        // GPIO_DAT_TOG(1ULL << 7);

        // GPIO_DAT_SET(1UL << 7);
        ble_adpcm_encode_package_send(pbuff, PCM_SAMPLE_NB);
        // GPIO_DAT_CLR(1UL << 7);
    } 
}
#endif
