/**
 ****************************************************************************************
 *
 * @file dma.c
 *
 * @brief Direct Memory Access(DMA) Driver
 *
 ****************************************************************************************
 */

#include "dma.h"
#include "rcc.h"
#include "reg_dma.h"
#include "reg_dmachcfg.h"


/*
 * DEFINES
 ****************************************************************************************
 */

#define DMA_CFG_EN_BIT             (1 << 0)      // bit0 master_enable
#define DMA_CFG_HPROT(bits)        ((bits) << 5) // bit[7:5] chnl_prot_ctrl
    
#define DMA_CHNL_CTRL(chidx)       (DMA_CHNL_CTRL_Typedef *)(DMA->CTRLBASE_POINTER.Word + ((chidx) * sizeof(DMA_CHNL_CTRL_Typedef)))


/// The amount of system memory that you must assign to the controller depends on the
/// number of DMA channels and whether you configure it to use the alternate data structure.
/// Contains: 1ch-0x20, 2chs-0x40, 3~4chs-0x80, 5~8chs-0x100, 9~16chs-0x200, 17~32chs-0x400
__attribute__((section("DMA_ALIGN"), aligned(0x100), zero_init))
volatile DMA_CHNL_CTRL_STRUCT_Typedef dma_ctrl_base;


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void dma_init(void)
{
    // clk_en rst_req
    RCC_APBCLK_EN(APB_DMAC_BIT);
    RCC_APBRST_REQ(APB_DMAC_BIT);
    
    // dma base addr
    DMA->CTRLBASE_POINTER.Word = (uint32_t)&dma_ctrl_base;
    
    // dma enable
    DMA->CFG.Word = DMA_CFG_HPROT(7) | DMA_CFG_EN_BIT;
}

void dma_deinit(void)
{
    // dma disable
    DMA->CFG.Word = 0;
    DMA->CTRLBASE_POINTER.Word = 0;
}

void dma_chnl_init(uint8_t chidx, uint8_t chsel)
{
    // Channel primary set, select function
    if (chidx < DMA_CH_MAX)
    {
        // dma disable signal translation
        DMA->USEBURST_SET = (1UL << chidx);
        // close the mask of request for channel
        DMA->REQMSK_CLR   = (1UL << chidx);
        
        // config dma reuse function
        uint8_t arr = (chidx / 4);
        uint8_t lsh = (chidx % 4) * 8; // 6 of 8bit
        DMACHCFG->CHSEL[arr] = (DMACHCFG->CHSEL[arr] & ~(0x3FUL << lsh)) | ((uint32_t)chsel << lsh);
    }
}

void dma_chnl_deinit(uint8_t chidx)
{
    uint32_t chbit = 1UL << (chidx % DMA_CH_MAX);
    
    DMA->CHNL_EN_CLR = chbit;
    // dma disable signal translation
    DMA->USEBURST_SET = (1UL << chidx);
    // close the mask of request for channel
    DMA->REQMSK_CLR   = (1UL << chidx);
}

void dma_chnl_conf(uint8_t chidx, uint32_t src_ep, uint32_t dst_ep, uint32_t trans)
{
    DMA_CHNL_CTRL_Typedef *chnl_cur = DMA_CHNL_CTRL(chidx);
    
    // Channel primary set, select function
    // done in dma_chnl_init(chidx, chsel);
    
    // Channel control fill
    chnl_cur->SRC_DATA_END_PTR    = src_ep;
    chnl_cur->DST_DATA_END_PTR    = dst_ep;
    chnl_cur->TRANS_CFG_DATA.Word = trans;
    chnl_cur->TRANS_CFG_RESV.Word = trans;
    
    DMA->CHNL_EN_SET = 1UL << (chidx % DMA_CH_MAX);
}

bool dma_chnl_reload(uint8_t chidx)
{    
    DMA_CHNL_CTRL_Typedef *chnl_cur;
    
    chidx = chidx % DMA_CH_MAX; // Pri Channel
    
    bool alter = (DMA->PRIALT_SET & (1UL << chidx)) ? true : false;
    
    chnl_cur = DMA_CHNL_CTRL(chidx);
    
    if (chnl_cur->TRANS_CFG_RESV.CYCLE_CTRL != CCM_PING_PONG)
    {
        chnl_cur->TRANS_CFG_DATA = chnl_cur->TRANS_CFG_RESV;
    }
    else
    {
        #if (1)
        if (chnl_cur->TRANS_CFG_DATA.CYCLE_CTRL == CCM_STOP)
        {
            chnl_cur->TRANS_CFG_DATA = chnl_cur->TRANS_CFG_RESV;
        }
        
        chnl_cur = DMA_CHNL_CTRL(chidx | DMA_CH_ALT);
        if (chnl_cur->TRANS_CFG_DATA.CYCLE_CTRL == CCM_STOP)
        {
            chnl_cur->TRANS_CFG_DATA = chnl_cur->TRANS_CFG_RESV;
        }
        #else
        if (DMA->PRIALT_SET & (1UL << chidx))
        {
            chnl_cur->TRANS_CFG_DATA = chnl_cur->TRANS_CFG_RESV;
        }
        else
        {
            DMA_CHNL_CTRL_Typedef *chnl_alt = DMA_CHNL_CTRL(chidx | DMA_CH_ALT);
            chnl_alt->TRANS_CFG_DATA = chnl_alt->TRANS_CFG_RESV;
        }
        #endif
    }

    DMA->CHNL_EN_SET = (1UL << chidx);
    return alter;
}

uint16_t dma_chnl_remain(uint8_t chidx)
{
    DMA_CHNL_CTRL_Typedef *chnl_cur = DMA_CHNL_CTRL(chidx);
    
    if (chnl_cur->TRANS_CFG_DATA.CYCLE_CTRL)
        return (chnl_cur->TRANS_CFG_DATA.N_MINUS_1 + 1);
    else
        return 0;
}

void dma_chnl_ctrl(uint8_t chidx, uint8_t ctrl)
{
    uint32_t chbit = 1UL << (chidx % DMA_CH_MAX);
    
    if (ctrl)
    {
        DMA->CHNL_EN_SET = chbit;
        
        if (ctrl == CHNL_DONE)
        {
            while ((DMACHCFG->IFLAG0 & chbit) == 0);
            DMACHCFG->ICFR0 = chbit;
        }
    }
    else
    {
        DMA->CHNL_EN_CLR = chbit;
    }
}

bool dma_chnl_done(uint8_t chidx)
{
    uint32_t chbit = 1UL << (chidx % DMA_CH_MAX);
    
    if (DMACHCFG->IFLAG0 & chbit)
    {
        // clear done, *DMA->CHNL_EN auto be cleared*
        DMACHCFG->ICFR0 = chbit;
        return true;
    }
    else
    {
        return false;
    }
}
