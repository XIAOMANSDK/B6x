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
#include "pt.h"

#if (VOICE)

#undef __ATTR_SRAM
#define __ATTR_SRAM         __attribute__((section("ram_func"), zero_init))

#define SAMPLE_BUF_LEN_16BIT    (60) // MSBC_NB_PCM8K

#define MSBC_OUTPUT_LEN         (57) /*!< 240b msbc压缩后一帧的大小 */
#define BLE_FRAME_SIZE          (60) /*!< 加上小米协议后的总大小 */

#define MIC_IB_PAD 2
#define MIC_IN_PAD 3

#define DMA_PCM_CHAN   DMA_CH0

typedef struct {
    uint8_t send_idx; /*!< 蓝牙发送的index */
    uint8_t seq_idx;
    uint8_t header_seq[4]; /*!< 08 38 C8 F8 */
    uint8_t header_cnt;
} pt_audio_data_t;

extern uint8_t voice_start_flag;

__ATTR_SRAM int16_t pcm_buff0[SAMPLE_BUF_LEN_16BIT];
__ATTR_SRAM int16_t pcm_buff1[SAMPLE_BUF_LEN_16BIT];
// 打包的msbc的buffer
__ATTR_SRAM uint8_t voice_send_buff[BLE_FRAME_SIZE];
//__ATTR_SRAM sbc_t audio_env_sbc;

__RETENTION pt_audio_data_t audio_env;

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

static void pt_voice_init(void)
{
    msbc_estart();

    audio_env.header_seq[0] = 0x08;
    audio_env.header_seq[1] = 0x38;
    audio_env.header_seq[2] = 0xC8;
    audio_env.header_seq[3] = 0xF8;
    audio_env.header_cnt = 0;

    audio_env.send_idx = 0;
    audio_env.seq_idx = 0;
}

void pt_voice_deinit(void)
{
    //sbc_finish(&audio_env_sbc);
    PT_LOGI("[%s] %d %d\n", __FUNCTION__, voiceSendNB, voiceSendOK);
}


/**
 * @brief pcm压缩、打包、发送
 * 
 * @param[in] sample_buf pcm数据
 * @param[in] len 长度，等于MSBC_NB_PCM8K
 */
__SRAMFN static void pt_msbc_encode_package_send(int16_t *sample_buf, int len)
{
    uint8_t *msbc_buff = &voice_send_buff[2];
    msbc_encode_8k(sample_buf, msbc_buff);

    voice_send_buff[0] = 0x01;
    voice_send_buff[1] = audio_env.header_seq[audio_env.seq_idx];
    // voice_send_buff[1] = audio_env.header_cnt++; /*!< for test */
    voice_send_buff[BLE_FRAME_SIZE - 1] = 0x00;

    voiceSendNB++;
    if (pt_hid_voice_report_send(audio_env.send_idx, voice_send_buff, BLE_FRAME_SIZE) == 0)
    {
        voiceSendOK++;
    }

    audio_env.send_idx = (audio_env.send_idx + 1) % 3;
    audio_env.seq_idx = (audio_env.seq_idx + 1) % sizeof(audio_env.header_seq);
}

void micInit(void)
{
    pt_voice_init();

    GPIO_DIR_CLR(GPIO02 | GPIO03);
    iom_ctrl(MIC_IB_PAD, IOM_HIZ);
    iom_ctrl(MIC_IN_PAD, IOM_ANALOG);   // 5 

    // sadc init
    sadc_init(SADC_ANA_DFLT | SADC_INBUF_BYPSS_BIT);

    dma_init();

    // DMA Conf: direct Init
//    SADC->CTRL.SADC_DMAC_EN = 0;      
    
    DMA_SADC_INIT(DMA_PCM_CHAN);
    DMA_SADC_PCM_CONF(DMA_PCM_CHAN, pcm_buff0, SAMPLE_BUF_LEN_16BIT, CCM_PING_PONG);
    DMA_SADC_PCM_CONF(DMA_PCM_CHAN | DMA_CH_ALT, pcm_buff1, SAMPLE_BUF_LEN_16BIT, CCM_PING_PONG);
    sadc_conf((SADC_CR_DFLT & ~(SADC_CR_HPF_COEF_MSK|SADC_CR_CLK_DIV_MSK)) | SADC_CR_HPF(3) | SADC_CR_CLK(SYS_CLK));
    sadc_pcm(SADC_MIC_DFLT & (~SADC_PGA_VOL_MSK | SADC_PGA_VOL(0)));

    voiceSendNB = 0; 
    voiceSendOK = 0;
    voiceSendFt = 4;
}

void micDeinit(void)
{
    pt_voice_deinit();

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
        pt_msbc_encode_package_send(pbuff, MSBC_NB_PCM8K);
        // GPIO_DAT_CLR(1UL << 7);
    } 
}
#endif
