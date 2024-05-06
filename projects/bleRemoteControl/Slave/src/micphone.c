#include "app_user.h"
#include "drvs.h"
#include "regs.h"
#include "adpcm.h"
#include "string.h"
#include "sadc.h"
#include "b6x.h"
#include "bledef.h"
#include "hid_desc.h"
#include "app.h"
#include "prf_sess.h"
#include "app_user.h"

#if (VOICE)
                               
#define ADPCM_BLOCK_SIZE        (128)
#define SAMPLE_DATA_SIZE        (ADPCM_BLOCK_SIZE - 4)
#define READ_UNIT_SIZE          (SAMPLE_DATA_SIZE * 4 + 2)

struct ADPCMBlock
{
    short sample0;
    char index;
    char RESERVED;
    char sampledata[SAMPLE_DATA_SIZE];
};

#if (0)
#define PCM_SAMPLE_NB   (216)     // 256: 32ms  8000Hz
#else
#define PCM_SAMPLE_NB   (READ_UNIT_SIZE/2)     // 256: 32ms  8000Hz
#endif


#define MIC_IB_PAD 2
#define MIC_IN_PAD 3

#define DMA_PCM_CHAN   DMA_CH0

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

void micInit(void)
{
    GPIO_DIR_CLR(GPIO02 | GPIO03);
    iom_ctrl(MIC_IB_PAD, IOM_HIZ);
    iom_ctrl(MIC_IN_PAD, IOM_ANALOG);   // 5 
    
    // sadc init
    sadc_init(SADC_ANA_DFLT | SADC_INBUF_BYPSS_BIT);

    dma_init();       
    
    // DMA Conf: direct Init
    SADC->CTRL.SADC_DMAC_EN = 0;
    
    DMA_SADC_INIT(DMA_PCM_CHAN);
    DMA_SADC_PCM_CONF(DMA_PCM_CHAN, pcm_buff0, PCM_SAMPLE_NB, CCM_PING_PONG);
    DMA_SADC_PCM_CONF(DMA_PCM_CHAN | DMA_CH_ALT, pcm_buff1, PCM_SAMPLE_NB, CCM_PING_PONG);
    sadc_conf((SADC_CR_DFLT & ~(SADC_CR_HPF_COEF_MSK|SADC_CR_CLK_DIV_MSK)) | SADC_CR_HPF(3) | SADC_CR_CLK(0));
    sadc_pcm(SADC_MIC_DFLT & (~SADC_PGA_VOL_MSK | SADC_PGA_VOL(0)));

    state.index = 0;
    state.valprev = 0;

    voiceSendNB = 0; 
    voiceSendOK = 0;
    voiceSendFt = 4;
}

void voice_send(uint16_t len, const uint8_t *data)
{
    voiceSendNB++;
    if (hids_report_send(app_env.curidx, RPT_IDX_MIC, len, data) == LE_SUCCESS)
    {
        voiceSendOK++;
    }
}

#if  (MODE_SELECT)
void micPut(void)
{
    // primary or alternate transfer done
    if (dma_chnl_done(DMA_PCM_CHAN))
    {   
        if (dma_chnl_reload(DMA_PCM_CHAN))  // 0x100
        {
            uartTxSend((uint8_t *)&pcm_buff1, PCM_SAMPLE_NB*2); 
        }       
        else
        {
            uartTxSend((uint8_t *)&pcm_buff0, PCM_SAMPLE_NB*2);
        }
    }
}
#else
void micPut(void)
{
    // primary or alternate transfer done
    if (dma_chnl_done(DMA_PCM_CHAN))
    {   
        if (voiceSendFt)
        {
            // ¹ýÂË
            dma_chnl_reload(DMA_PCM_CHAN);
            voiceSendFt--;
            return;
        }
        
        adpcm_buff.index = state.index;
        
        if (dma_chnl_reload(DMA_PCM_CHAN))  // 0x100
        {
            adpcm_buff.sample0 = pcm_buff1[0];
            state.valprev = pcm_buff1[0];
            adpcm_coder((short*)&pcm_buff1[1], (char *)adpcm_buff.sampledata, (PCM_SAMPLE_NB - 1), &state); 
        }       
        else
        {
            adpcm_buff.sample0 = pcm_buff0[0];
            state.valprev = pcm_buff0[0];
            adpcm_coder((short*)&pcm_buff0[1], (char *)adpcm_buff.sampledata, (PCM_SAMPLE_NB - 1), &state);        
        }
        
//        uartTxSend((uint8_t *)&adpcm_buff, ADPCM_BLOCK_SIZE);  
//        sess_txd_send(app_env.curidx, ADPCM_BLOCK_SIZE, (uint8_t *)&adpcm_buff);
        voice_send(ADPCM_BLOCK_SIZE, (uint8_t *)&adpcm_buff);
    } 
}
#endif
#endif

