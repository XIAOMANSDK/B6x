#include "drvs.h"
#include "regs.h"
#include "adpcm.h"
#include "micphone.h"

#if (DBG_MICPHONE)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif

#ifndef ADPCM_SELECT
#define ADPCM_SELECT         (0)  // 1:ADPCM DATA   0:PCM DATA 
#endif

#ifndef MICPHONE_DMA_CHNL
#define MICPHONE_DMA_CHNL    (DMA_CH0)
#endif

#define MIC_IB_PAD 2
#define MIC_IN_PAD 3

#if (ADPCM_SELECT)
#define ADPCM_BLOCK_SIZE        (128)
#define SAMPLE_DATA_SIZE        (ADPCM_BLOCK_SIZE - 4)
#define READ_UNIT_SIZE          (SAMPLE_DATA_SIZE * 4 + 2)
#define PCM_SAMPLE_NB           (READ_UNIT_SIZE/2)     // 256: 32ms  8000Hz

struct ADPCMBlock
{
    short sample0;
    char index;
    char RESERVED;
    char sampledata[SAMPLE_DATA_SIZE];
};

__ATTR_SRAM struct ADPCMBlock adpcm_buff;
struct adpcm_state state;
#endif

#ifndef PCM_SAMPLE_NB
#define PCM_SAMPLE_NB   (216)     // 256: 32ms  8000Hze
#endif

#if (ADPCM_SELECT)
#define MICPHONE_DATA_LEN        (ADPCM_BLOCK_SIZE)
#else
#define MICPHONE_DATA_LEN        (PCM_SAMPLE_NB*2)
#endif

__ATTR_SRAM int16_t pcm_buff0[PCM_SAMPLE_NB];  // Ping
__ATTR_SRAM int16_t pcm_buff1[PCM_SAMPLE_NB];  // Pong
__ATTR_SRAM int16_t pcm_buff2[PCM_SAMPLE_NB];  // Ping
__ATTR_SRAM int16_t pcm_buff3[PCM_SAMPLE_NB];  // Pong

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
    
    DMA_SADC_INIT(MICPHONE_DMA_CHNL);
    DMA_SADC_PCM_CONF(MICPHONE_DMA_CHNL, pcm_buff0, PCM_SAMPLE_NB, CCM_PING_PONG);
    DMA_SADC_PCM_CONF(MICPHONE_DMA_CHNL | DMA_CH_ALT, pcm_buff1, PCM_SAMPLE_NB, CCM_PING_PONG);
    sadc_conf((SADC_CR_DFLT & ~(SADC_CR_HPF_COEF_MSK|SADC_CR_CLK_DIV_MSK)) | SADC_CR_HPF(3) | SADC_CR_CLK(2));
    sadc_pcm(SADC_MIC_DFLT & (~SADC_PGA_VOL_MSK | SADC_PGA_VOL(0)));

    voiceSendFt = 4;
    
    #if (ADPCM_SELECT)
    state.index = 0;
    state.valprev = 0;
    #endif    
}

void micDeinit(void)
{
    GPIO_DIR_CLR(GPIO02 |GPIO03);
    iom_ctrl(MIC_IB_PAD,IOM_HIZ);
    iom_ctrl(MIC_IN_PAD,IOM_HIZ);
    // disable SADC,reduce power
    SADC->CTRL.Word = 0;
    SADC->MIC_CTRL.Word = SADC_MIC_PWD_BIT;
    SADC->SADC_ANA_CTRL.Word = 0;
    dma_chnl_ctrl(MICPHONE_DMA_CHNL,CHNL_DIS);
    dma_chnl_deinit(MICPHONE_DMA_CHNL);
    dma_deinit();
}

uint8_t* micDataGet(void)
{
    // primary or alternate transfer done
    if (dma_chnl_done(MICPHONE_DMA_CHNL))
    {   
        if (voiceSendFt)
        {
            // ¹ýÂË
            dma_chnl_reload(MICPHONE_DMA_CHNL);
            voiceSendFt--;
            return NULL;
        }
        
        #if (ADPCM_SELECT)
        adpcm_buff.index = state.index;
        #endif
        
//        GPIO_DIR_SET_HI(GPIO17);
//        GPIO_DIR_SET_LO(GPIO17);
        
        if (dma_chnl_reload(MICPHONE_DMA_CHNL))  // 0x100
        {
            #if (ADPCM_SELECT)
            adpcm_buff.sample0 = pcm_buff1[0];
            state.valprev = pcm_buff1[0];
            adpcm_coder((short*)&pcm_buff1[1], (char *)adpcm_buff.sampledata, (PCM_SAMPLE_NB - 1), &state); 
            #else
            return (uint8_t *)&pcm_buff1;
            #endif
        }
        else
        {
            #if (ADPCM_SELECT)
            adpcm_buff.sample0 = pcm_buff0[0];
            state.valprev = pcm_buff0[0];
            adpcm_coder((short*)&pcm_buff0[1], (char *)adpcm_buff.sampledata, (PCM_SAMPLE_NB - 1), &state);        
            #else
            return (uint8_t *)&pcm_buff0;
            #endif
        }
        
//        uart_send(UART1_PORT, (uint8_t *)&adpcm_buff, ADPCM_BLOCK_SIZE);  
//        sess_txd_send(app_env.curidx, ADPCM_BLOCK_SIZE, (uint8_t *)&adpcm_buff);
//        voice_send(ADPCM_BLOCK_SIZE, (uint8_t *)&adpcm_buff);
        #if (ADPCM_SELECT)
        return (uint8_t *)&adpcm_buff;
        #endif
    }
    
    return NULL;
}


