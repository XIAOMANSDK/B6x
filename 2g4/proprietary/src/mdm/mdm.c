
#include <stdint.h>
#include "drvs.h"
#include "reg_mdm.h"
#include "rfctrl.h"
#include "mdm.h"
#include "crc.h"

#define DMA_CH_MDM_TX         0
#define DMA_CH_MDM_RX         1

#define MDM_SLOT_GET_TWICE    1

#define MDM_SLOT_GET(_slot, _fine)     \
        do {                           \
            _slot = MDM->SLOTCNT;      \
            _fine = MDM->FINECNT;      \
            if (_slot != MDM->SLOTCNT) \
            {                          \
                _slot = MDM->SLOTCNT;  \
                _fine = MDM->FINECNT;  \
            }                          \
        } while(0)

static mdm_cb_t mdm_notifier = {0,};

static volatile mdm_conf_t mdm_config;
static volatile mdm_state_t mdm_state;

static uint8_t mdm_rx_buff[MDM_RX_DATA_LEN] = {0};
static uint8_t mdm_tx_buff[MDM_TX_DATA_LEN] = {0};

static void mdm_load(void)
{
    mdm_config.slot_win = MDM->SLOT_SET.MDM_SLOT_WIN;
    mdm_config.slot_off = MDM->SLOT_SET.MDM_SLOT_OFF;
    mdm_config.sync_win = MDM->RXSYNC_WIN;
    mdm_config.access_code = MDM->ACCESS_REG;
}

void mdm_init(mdm_cb_t *cb)
{
    RCC_APBCLK_EN(APB_APBMISC_BIT | APB_AON_BIT | APB_RF_BIT | APB_MDM_BIT);
    RCC_AHBCLK_EN(AHB_SYSCFG_BIT);
    rcc_adc_en();

    rf_ctrl_init();

    MDM->REG0.ACC_REG_EN = 0;
    MDM->EXT_CTRL.MDM_INT_EN = MDM_INT_ENABLED;
    MDM->EXT_CTRL.MDM_EXT_EN = 1;

    mdm_load();

    mdm_notifier = *cb;

    mdm_state.int_slot_n = 0;
    mdm_state.int_fine_n = 0;
    mdm_state.sync_slot_n = 0;
    mdm_state.sync_fine_n = MDM_SYNC_NONE;

    NVIC_EnableIRQ(MDM_IRQn);

    dma_init();
    DMA_MDM_TX_INIT(DMA_CH_MDM_TX);
    DMA_MDM_RX_INIT(DMA_CH_MDM_RX);

    DMACHCFG->IEFR0 = (1UL << DMA_CH_MDM_RX);

    NVIC_EnableIRQ(DMAC_IRQn);
}

void mdm_conf(mdm_conf_t *conf)
{
    MDM->SLOT_SET.MDM_SLOT_OFF = conf->slot_off;
    MDM->SLOT_SET.MDM_SLOT_WIN = conf->slot_win;

    MDM->RXSYNC_WIN            = conf->sync_win;
    MDM->ACCESS_REG            = conf->access_code;
    MDM->REG0.ACC_REG_EN       = 1;

    mdm_load();

    #if (MDM_SLOT_GET_TWICE)
    MDM_SLOT_GET(mdm_state.int_slot_n, mdm_state.int_fine_n);
    #else
    mdm_state.int_slot_n = MDM->SLOTCNT;
    mdm_state.int_fine_n = MDM->FINECNT;
    #endif

    mdm_state.sync_slot_n = 0;
    mdm_state.sync_fine_n = MDM_SYNC_NONE;
}

void mdm_retrieve_slot(uint32_t *slot_n, uint16_t *fine_n)
{
    *fine_n = MDM->FINECNT;
    *slot_n = (*fine_n < mdm_state.int_fine_n ? mdm_state.int_slot_n : mdm_state.int_slot_n + 1)
              & MDM_SLOT_N_MASK;
}

void mdm_wait_slot(uint32_t slot_n, uint16_t fine_n)
{
    uint32_t slot_c;
    do
    {
        slot_c = MDM->SLOTCNT;
        if (slot_c != MDM->SLOTCNT)
        {
            slot_c = MDM->SLOTCNT;
        }
    } while (slot_c < slot_n || (slot_n == 0 && slot_c > 0x0FFFFFF0));

    if (slot_c == slot_n)
    {
        while (MDM->FINECNT > fine_n);
    }
}

void mdm_update_slot(uint32_t slot_off, uint16_t fine_off)
{
    MDM->EXT_CTRL.MDM_INT_EN = 0;// ( MDM_INT_TX_DONE | MDM_INT_SYNC_ERR );
    MDM->EXT_CTRL.MDM_SLOT_CNT_HOLD = 1;

    #if (MDM_SLOT_GET_TWICE)
    uint32_t slot_n;
    uint16_t fine_n;
    MDM_SLOT_GET(slot_n, fine_n);
    #else
    uint32_t slot_n = MDM->SLOTCNT;
    uint16_t fine_n = MDM->FINECNT;
    #endif
    slot_n += slot_off;

    if (fine_n >= fine_off)
    {
        fine_n -= fine_off;
    }
    else
    {
        slot_n++;
        fine_n += mdm_config.slot_win - fine_off;
    }

    slot_n &= MDM_SLOT_N_MASK;

    MDM->SLOTCNT_SET = slot_n;
    MDM->FINECNT_SET = fine_n;

    mdm_state.int_slot_n = slot_n;
    mdm_state.int_fine_n = fine_n;

    mdm_state.sync_slot_n = 0;
    mdm_state.sync_fine_n = MDM_SYNC_NONE;

    MDM->EXT_CTRL.MDM_SLOT_UPLOAD = 1;
    MDM->EXT_CTRL.MDM_SLOT_CNT_HOLD = 0;
    MDM->EXT_CTRL.MDM_INT_EN = MDM_INT_ENABLED;
}

void mdm_rx_enable(uint8_t chn_idx, uint8_t rf_rate)
{
    DMA_MDM_RX_CONF(DMA_CH_MDM_RX, mdm_rx_buff, MDM_RX_DATA_LEN, CCM_BASIC);

    rf_ctrl_rx_enable(chn_idx, rf_rate);
    mdm_state.sync_slot_n = 0;
    mdm_state.sync_fine_n = MDM_SYNC_NONE;
}

void mdm_rtx_disable(void)
{
    rf_ctrl_rtx_disable();
    dma_chnl_ctrl(DMA_CH_MDM_RX, CHNL_DIS);
}

void mdm_tx_send(uint8_t chn_idx, uint8_t rf_rate, uint32_t access_code,
                 uint8_t *data, uint16_t length)
{
    uint8_t *p = mdm_tx_buff;
    uint8_t preamble = 0xaa ^ ( - (access_code & 1) );
    *p++ = preamble;
    if (rf_rate == RATE_2Mbps)
    {
        *p++ = preamble;
    }
    *p++ = access_code & 0xff;
    *p++ = (access_code >> 8) & 0xff;
    *p++ = (access_code >> 16) & 0xff;
    *p++ = (access_code >> 24) & 0xff;

    while (length--)
    {
        *p++ = *data++;
    }

    DMA_MDM_TX_CONF(DMA_CH_MDM_TX, mdm_tx_buff, p - mdm_tx_buff, CCM_BASIC);
    rf_ctrl_tx_enable(chn_idx, rf_rate);
}

__STATIC_INLINE void mdm_find_sync_time(MDM_EXT_ST_TypeDef ext_st)
{
    if (ext_st.MDM_SYNC_FOUND && mdm_state.sync_fine_n == MDM_SYNC_NONE)
    {
        mdm_state.sync_fine_n = ext_st.MDM_SYNC_TIME;
        mdm_state.sync_slot_n = (mdm_state.sync_fine_n < mdm_state.int_fine_n ?
                                  mdm_state.int_slot_n : ((mdm_state.int_slot_n + 1) & MDM_SLOT_N_MASK));

    }
}

void MDM_IRQHandler(void)
{
    MDM_EXT_ST_TypeDef ext_st = MDM->EXT_ST;

    if (ext_st.MDM_SLOT_INT)
    {
        MDM->EXT_CTRL.MDM_SLOT_INT_CLR = 1;

        mdm_find_sync_time(ext_st);
        #if (MDM_SLOT_GET_TWICE)
        MDM_SLOT_GET(mdm_state.int_slot_n, mdm_state.int_fine_n);
        #else
        mdm_state.int_slot_n = MDM->SLOTCNT;
        mdm_state.int_fine_n = MDM->FINECNT;
        #endif
        //MDM->EXT_CTRL.MDM_SLOT_CNT_HOLD = 0;
        mdm_notifier.slot_begin(mdm_state.int_slot_n, mdm_state.int_fine_n, ext_st.MDM_SYNC_FOUND);
        //while(MDM->EXT_ST.MDM_SLOT_INT);
    }

    if (ext_st.MDM_SYNC_ERR)
    {
        dma_chnl_ctrl(DMA_CH_MDM_RX, CHNL_DIS);
        mdm_notifier.sync_error();
    }

    if (ext_st.MDM_TX_DONE)
    {
        uint32_t slot_n;
        uint16_t fine_n;
        mdm_retrieve_slot(&slot_n, &fine_n);
        dma_chnl_done(DMA_CH_MDM_TX);
        mdm_notifier.tx_done(slot_n, fine_n);
    }
}

void DMAC_IRQHandler(void)
{
    uint32_t iflag = DMACHCFG->IFLAG0;

    // disable intr
    DMACHCFG->IEFR0 &= ~iflag;
    // clear intr flag
    DMACHCFG->ICFR0 = iflag;
    NVIC_ClearPendingIRQ(DMAC_IRQn);

    if (iflag & BIT(DMA_CH_MDM_RX))
    {
        uint16_t length = *(mdm_rx_buff + 1) + 2 + CRC_LEN;
        mdm_find_sync_time(MDM->EXT_ST);
        if (length > MDM_RX_DATA_LEN)
        {
            length = MDM_RX_DATA_LEN;
        }
        dma_chnl_ctrl(DMA_CH_MDM_RX, CHNL_DIS);
        mdm_notifier.rx_done(mdm_rx_buff, length, mdm_state.sync_slot_n, mdm_state.sync_fine_n);
    }

    // re-enable intr
    DMACHCFG->IEFR0 |= iflag;
}
