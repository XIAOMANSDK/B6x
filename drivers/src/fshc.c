/**
 ****************************************************************************************
 *
 * @file flash.c
 *
 * @brief flash Driver
 *
 ****************************************************************************************
 */
#include "drvs.h"
#include "regs.h"

/*
 * DEFINES
 ****************************************************************************************
 */
#ifdef __SRAMFN
#undef __SRAMFN
#endif
#define __SRAMFN(line) __attribute__((section("ram_func.fshc." #line)))

#define FLASH_PAGE_SIZE 256
/// PUYA wait complete
#define FSH_CMD_RD_STA0        0x05
#define FSH_FLAG_WEL_WIP       0x03 // bit0: WEL, bit1: WIP

#define FSHC_WAIT_COMPLETE()   while (fshc_rd_sta(FSH_CMD_RD_STA0, 1) & FSH_FLAG_WEL_WIP)

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */
__SRAMFN(33)
void puya_enter_dual_read(void)
{
    if (FSHC->BYPASS_HPM == 0)
        return;
    
    bool cache_en = (CACHE->CCR.Word);
    CACHE->CCR.Word = 0;
    CACHE->CIR.INV_ALL = 1;
    
    GLOBAL_INT_DISABLE();
    
    fshc_xip_conf(FSH_CMD_DLRD_HMP, IBUS_DL_1DUMY, IBUS_DLRD_HPM);
    fshc_hpm_conf(true, 0x20, 0x10);
    
    GLOBAL_INT_RESTORE();
    
    if (cache_en)
    {
        CACHE->CCR.Word = 1;
    }
}

__SRAMFN(56)
void puya_exit_dual_read(void)
{
    if (FSHC->BYPASS_HPM == 1)
        return;
    
    bool cache_en = (CACHE->CCR.Word);
    CACHE->CCR.Word = 0;
    CACHE->CIR.INV_ALL = 1;
    
    GLOBAL_INT_DISABLE();
    fshc_hpm_conf(false, 0x01, 0x10);
    fshc_xip_conf(FSH_CMD_RD, IBUS_SI_0DUMY, IBUS_SIRD_CFG);
    GLOBAL_INT_RESTORE();
    
    if (cache_en)
    {
        CACHE->CCR.Word = 1;
    }
}

__SRAMFN(77)
void boya_flash_quad_mode(void)
{
    uint8_t sta_reg1 = fshc_rd_sta(FSH_CMD_RD_STA1, 1);
    
    if ((sta_reg1 & 0x02) != 0x02)
    {
        uint8_t sys_clk = (rcc_sysclk_get() + 1) << 4;
        
        sta_reg1 |= 0x02;
        
        // write en singal
        fshc_en_cmd(FSH_CMD_WR_EN);

        // send write sta cmd
        fshc_wr_sta(0x31/*FSH_CMD_WR_STA*/, 1, sta_reg1);
        
        // Write Status Register Cycle Time, Max=12ms
        btmr_delay(sys_clk, 12000);
    }
}

__SRAMFN(99)
void flash_write(uint32_t offset, uint32_t *data, uint32_t wlen)
{
    GLOBAL_INT_DISABLE();
    
    while (SYSCFG->ACC_CCR_BUSY);
    
    fshc_write(offset, data, wlen, FSH_CMD_WR);
    
    GLOBAL_INT_RESTORE();
}

__SRAMFN(111)
void flash_page_erase(uint32_t offset)
{
    GLOBAL_INT_DISABLE();
    
    while (SYSCFG->ACC_CCR_BUSY);
    
    fshc_erase(offset, FSH_CMD_ER_PAGE);
    
    GLOBAL_INT_RESTORE();
}

__SRAMFN(123)
void flash_byte_write(uint32_t offset, uint8_t *data, uint32_t blen)
{
    flen_t wrcnt = 0;
    uint32_t wr_val = 0;
    
    GLOBAL_INT_DISABLE();
    
    while (SYSCFG->ACC_CCR_BUSY);
    
#if (0)
/// fshc cmd mode
#define FCM_GET_CMD(fcmd)      (((fcmd) & FCM_CMD_MSK) >> FCM_CMD_LSB)
#define FCM_GET_MODE(fcmd)     (((fcmd) & FCM_MODE_MSK) >> FCM_MODE_LSB)
    uint8_t wr_cmd = FCM_GET_CMD(FSH_CMD_WR);
    uint8_t ln_mod = FCM_GET_MODE(FSH_CMD_WR);
    uint16_t wr_ctrl = SCTRL_WR_DAT(ln_mod);
    
    fshc_en_cmd(FSH_CMD_WR_EN);
    fshc_wr_cfg(wr_cmd, offset, len, wr_ctrl, ACBIT_SI_0DUMY);
#else
    fshc_en_cmd(FSH_CMD_WR_EN);
    fshc_wr_cfg(FSH_CMD_WR, offset, FLASH_PAGE_SIZE, 0x0354, ACBIT_SI_0DUMY);
#endif

    // first fill word, then enable send
    while (FSHC->FIFO_STATUS.TXFIFO_FULL);
    xmemcpy((uint8_t *)&wr_val, data + wrcnt, 4);
    FSHC->SPDR_WR = wr_val;
    FSHC->SEND_EN = 1;
    wrcnt += 4;

    // fill next data
    while (wrcnt < blen)
    {
        if (!FSHC->FIFO_STATUS.TXFIFO_FULL)
        {
            xmemcpy((uint8_t *)&wr_val, data + wrcnt, 4);
            FSHC->SPDR_WR = wr_val;
            wrcnt += 4;
        }
    }
    
    while (wrcnt < FLASH_PAGE_SIZE)
    {
        if (!FSHC->FIFO_STATUS.TXFIFO_FULL)
        {
            FSHC->SPDR_WR = 0xFFFFFFFF;
            wrcnt += 4;
        }
    }
    
    FSHC_WAIT_COMPLETE();
    
    GLOBAL_INT_RESTORE();
}

__SRAMFN(180)
void flash_byte_read(uint32_t offset, uint8_t *buff, uint32_t blen)
{
    flen_t rdcnt = 0;
    uint32_t rd_val = 0, algned4_len = ((blen >> 2) << 2);
    uint32_t remain_len = (blen - algned4_len);
    
    GLOBAL_INT_DISABLE();
    
    // wait cache idle
    while (SYSCFG->ACC_CCR_BUSY);
    
#if (0)
/// fshc cmd mode
#define FCM_GET_CMD(fcmd)      (((fcmd) & FCM_CMD_MSK) >> FCM_CMD_LSB)
#define FCM_GET_MODE(fcmd)     (((fcmd) & FCM_MODE_MSK) >> FCM_MODE_LSB)
    uint8_t rd_cmd = FCM_GET_CMD(FSH_CMD_RD);
    uint8_t ln_mod = FCM_GET_MODE(FSH_CMD_RD);
    
    uint16_t rd_ctrl = SCTRL_RD_DAT(ln_mod);
    // OTP | (Dual or Quad) use 1DUMY
    uint16_t rd_acbit = (FSH_CMD_RD & (FCM_RWOTP_BIT | (2 << FCM_MODE_LSB))) ?  ACBIT_SI_1DUMY : ACBIT_SI_0DUMY;
    
    fshc_rd_cfg(rd_cmd, offset, len, rd_ctrl, rd_acbit);
#else
    fshc_rd_cfg(FSH_CMD_RD, offset, blen, 0x0394, 0x01D7);
#endif
    
    while (rdcnt < algned4_len)
    {
        if (!FSHC->FIFO_STATUS.RXFIFO_EMPTY)
        {
            rd_val = FSHC->SPDR_RD;
            
            xmemcpy(buff + rdcnt, &rd_val, 4);
            
            rdcnt += 4;
        }
    }
    
    if ((remain_len) && (!FSHC->FIFO_STATUS.RXFIFO_EMPTY))
    {
        rd_val = FSHC->SPDR_RD;
        
        xmemcpy(buff + rdcnt, &rd_val, remain_len);
        
        rdcnt += remain_len;
    }
    
    GLOBAL_INT_RESTORE();
}

__SRAMFN(232)
void flash_read(uint32_t offset, uint32_t *buff, uint32_t wlen)
{
    GLOBAL_INT_DISABLE();
    
    while (SYSCFG->ACC_CCR_BUSY);

    fshc_read(offset, buff, wlen, FSH_CMD_RD);

    GLOBAL_INT_RESTORE();
}

#if (0)
#define ERASE_MODE_MAX 4
const uint8_t ERASE_CMD[ERASE_MODE_MAX] =
{
    /*ERASE_PAGE    = */0x81,
    /*ERASE_SECTOR  = */0x20,
    /*ERASE_BLOCK32 = */0x52,
    /*ERASE_BLOCK64 = */0xD8,
};

const uint8_t PER_SIZE[ERASE_MODE_MAX] =
{
    8,   // Page Size 256Bytes
    12,  // Sector Size 4KBytes
    15,  // Block Size 32KBytes
    16,  // Block Size 64KBytes
};

__SRAMFN(262)
void flash_multi_erase(uint8_t erase_mode, uint32_t idx, uint32_t cnt)
{
    while (SYSCFG->ACC_CCR_BUSY);
    
    for (uint16_t i = 0; i < cnt; i++)
    {
        fshc_erase((idx << PER_SIZE[erase_mode]), ERASE_CMD[erase_mode]);
        
        ++idx;
    }
}
#endif
