/**
 ****************************************************************************************
 *
 * @file flash.c
 *
 * @brief Flash存储器驱动，支持读写、擦除和特殊模式配置
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
#define __SRAMFN(line) __attribute__((section("ram_func.fshc." #line)))  // 定义RAM函数段

#define FLASH_PAGE_SIZE 256                    // Flash页大小256字节
/// PUYA Flash等待完成相关定义
#define FSH_CMD_RD_STA0        0x05            // 读取状态寄存器0命令
#define FSH_FLAG_WEL_WIP       0x03            // 状态标志：bit0-WEL(写使能锁存), bit1-WIP(写进行中)

#define FSHC_WAIT_COMPLETE()   while (fshc_rd_sta(FSH_CMD_RD_STA0, 1) & FSH_FLAG_WEL_WIP)  // 等待Flash操作完成

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 * @brief 进入PUYA Flash双线读取模式
 * @note 此函数运行在RAM中，配置Flash控制器支持高性能双线读取模式
 */
__SRAMFN(33)
void puya_enter_dual_read(void)
{
    // 检查是否已处于高性能模式
    if (FSHC->BYPASS_HPM == 0)
        return;

    // 保存缓存配置并禁用缓存
    uint32_t reg_val = (CACHE->CCR.Word);
    CACHE->CCR.Word  = 0;
    CACHE->CIR.Word  = (0x01 << CACHE_INV_ALL_POS);  // 使缓存无效

    // 禁用全局中断，确保Flash配置不被中断
    GLOBAL_INT_DISABLE();

    // 配置Flash控制器为双线读取模式
    fshc_xip_conf(FSH_CMD_DLRD_HMP, IBUS_DL_1DUMY, IBUS_DLRD_HPM);
    fshc_hpm_conf(true, 0x20, 0x10);

    // 恢复全局中断
    GLOBAL_INT_RESTORE();

    // 恢复缓存配置
    CACHE->CCR.Word = reg_val;
}

/**
 * @brief 退出PUYA Flash双线读取模式
 * @note 此函数运行在RAM中，恢复Flash控制器到单线读取模式
 */
__SRAMFN(53)
void puya_exit_dual_read(void)
{
    // 检查是否处于高性能模式
    if (FSHC->BYPASS_HPM == 1)
        return;

    // 保存缓存配置并禁用缓存
    uint32_t reg_val = (CACHE->CCR.Word);
    CACHE->CCR.Word  = 0;
    CACHE->CIR.Word  = (0x01 << CACHE_INV_ALL_POS);  // 使缓存无效

    // 禁用全局中断
    GLOBAL_INT_DISABLE();

    // 恢复Flash控制器到单线读取模式
    fshc_hpm_conf(false, 0x01, 0x10);
    fshc_xip_conf(FSH_CMD_RD, IBUS_SI_0DUMY, IBUS_SIRD_CFG);

    // 恢复全局中断
    GLOBAL_INT_RESTORE();

    // 恢复缓存配置
    CACHE->CCR.Word = reg_val;
}

/**
 * @brief 配置BOYA Flash四线模式
 * @note 此函数运行在RAM中，启用BOYA Flash的四线SPI模式以提高读取性能
 */
__SRAMFN(71)
void boya_flash_quad_mode(void)
{
    // 禁用全局中断
    GLOBAL_INT_DISABLE();

    // 等待系统配置寄存器空闲
    while (SYSCFG->ACC_CCR_BUSY);

    // 读取Flash状态寄存器1
    uint8_t sta_reg1 = fshc_rd_sta(FSH_CMD_RD_STA1, 1);

    // 检查是否已启用四线模式
    if ((sta_reg1 & 0x02) != 0x02)
    {
        // 计算系统时钟相关的延时参数
        uint8_t sys_clk = (rcc_sysclk_get() + 1) << 4;

        // 设置四线模式标志位
        sta_reg1 |= 0x02;

        // 保存缓存配置并禁用缓存
        uint32_t reg_val = (CACHE->CCR.Word);
        CACHE->CCR.Word  = 0;
        CACHE->CIR.Word  = (0x01 << CACHE_INV_ALL_POS);

        // 发送写使能命令
        fshc_en_cmd(FSH_CMD_WR_EN);

        // 发送写状态寄存器命令，启用四线模式
        fshc_wr_sta(0x31/*FSH_CMD_WR_STA*/, 1, sta_reg1);

        // 恢复缓存配置
        CACHE->CCR.Word = reg_val;

        // 等待状态寄存器写入完成（最大12ms）
        btmr_delay(sys_clk, 12000);
    }

    // 恢复全局中断
    GLOBAL_INT_RESTORE();
}

/**
 * @brief Flash页写入函数
 * @param offset Flash偏移地址
 * @param data 待写入数据指针
 * @param wlen 写入数据长度（以字为单位）
 * @note 此函数运行在RAM中，执行Flash页写入操作
 */
__SRAMFN(103)
void flash_write(uint32_t offset, uint32_t *data, uint32_t wlen)
{
    // 禁用全局中断
    GLOBAL_INT_DISABLE();

    // 等待系统配置寄存器空闲
    while (SYSCFG->ACC_CCR_BUSY);

    // 调用底层Flash写入函数
    fshc_write(offset, data, wlen, FSH_CMD_WR);

    // 恢复全局中断
    GLOBAL_INT_RESTORE();
}

/**
 * @brief Flash页擦除函数
 * @param offset Flash偏移地址（页对齐）
 * @note 此函数运行在RAM中，执行Flash页擦除操作
 */
__SRAMFN(115)
void flash_page_erase(uint32_t offset)
{
    // 禁用全局中断
    GLOBAL_INT_DISABLE();

    // 等待系统配置寄存器空闲
    while (SYSCFG->ACC_CCR_BUSY);

    // 保存缓存配置并禁用缓存
    uint32_t reg_val = (CACHE->CCR.Word);
    CACHE->CCR.Word  = 0;
    CACHE->CIR.Word  = (0x01 << CACHE_INV_ALL_POS);

    // 调用底层Flash擦除函数
    fshc_erase(offset, FSH_CMD_ER_PAGE);

    // 恢复缓存配置
    CACHE->CCR.Word = reg_val;

    // 恢复全局中断
    GLOBAL_INT_RESTORE();
}

/**
 * @brief Flash字节写入函数
 * @param offset Flash偏移地址
 * @param data 待写入字节数据指针
 * @param blen 写入数据长度（字节数）
 * @note 此函数运行在RAM中，执行字节级Flash写入操作
 */
__SRAMFN(133)
void flash_byte_write(uint32_t offset, uint8_t *data, uint32_t blen)
{
    flen_t wrcnt = 0;
    uint32_t wr_val = 0;

    // 禁用全局中断
    GLOBAL_INT_DISABLE();

    // 等待系统配置寄存器空闲
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
    // 发送写使能命令
    fshc_en_cmd(FSH_CMD_WR_EN);

    // 配置Flash写入参数：命令、偏移地址、页大小、控制字、访问位
    fshc_wr_cfg(FSH_CMD_WR, offset, FLASH_PAGE_SIZE, 0x0354, ACBIT_SI_0DUMY);
#endif

    // 填充第一个字数据并启动发送
    while (FSHC->FIFO_STATUS.TXFIFO_FULL);  // 等待TX FIFO非满
    xmemcpy((uint8_t *)&wr_val, data + wrcnt, 4);
    FSHC->SPDR_WR = wr_val;                // 写入数据到Flash数据寄存器
    FSHC->SEND_EN = 1;                     // 使能发送
    wrcnt += 4;

    // 继续填充剩余数据
    while (wrcnt < blen)
    {
        if (!FSHC->FIFO_STATUS.TXFIFO_FULL)
        {
            xmemcpy((uint8_t *)&wr_val, data + wrcnt, 4);
            FSHC->SPDR_WR = wr_val;
            wrcnt += 4;
        }
    }

    // 填充页剩余部分为0xFFFFFFFF（Flash擦除状态）
    while (wrcnt < FLASH_PAGE_SIZE)
    {
        if (!FSHC->FIFO_STATUS.TXFIFO_FULL)
        {
            FSHC->SPDR_WR = 0xFFFFFFFF;
            wrcnt += 4;
        }
    }

    // 等待Flash操作完成
    FSHC_WAIT_COMPLETE();

    // 恢复全局中断
    GLOBAL_INT_RESTORE();
}

/**
 * @brief Flash字节读取函数
 * @param offset Flash偏移地址
 * @param buff 读取数据存储缓冲区
 * @param blen 读取数据长度（字节数）
 * @note 此函数运行在RAM中，执行字节级Flash读取操作
 */
__SRAMFN(190)
void flash_byte_read(uint32_t offset, uint8_t *buff, uint32_t blen)
{
    flen_t rdcnt = 0;
    uint32_t rd_val = 0, algned4_len = ((blen >> 2) << 2);  // 4字节对齐长度
    uint32_t remain_len = (blen - algned4_len);             // 剩余字节数

    // 禁用全局中断
    GLOBAL_INT_DISABLE();

    // 等待缓存空闲
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

    // 读取4字节对齐的数据
    while (rdcnt < algned4_len)
    {
        if (!FSHC->FIFO_STATUS.RXFIFO_EMPTY)  // 等待RX FIFO非空
        {
            rd_val = FSHC->SPDR_RD;           // 从Flash数据寄存器读取数据

            xmemcpy(buff + rdcnt, &rd_val, 4);

            rdcnt += 4;
        }
    }

    // 读取剩余的非4字节对齐数据
    if ((remain_len) && (!FSHC->FIFO_STATUS.RXFIFO_EMPTY))
    {
        rd_val = FSHC->SPDR_RD;

        xmemcpy(buff + rdcnt, &rd_val, remain_len);

        rdcnt += remain_len;
    }

    // 恢复全局中断
    GLOBAL_INT_RESTORE();
}

/**
 * @brief Flash字读取函数
 * @param offset Flash偏移地址
 * @param buff 读取数据存储缓冲区
 * @param wlen 读取数据长度（以字为单位）
 * @note 此函数运行在RAM中，执行字级Flash读取操作
 */
__SRAMFN(242)
void flash_read(uint32_t offset, uint32_t *buff, uint32_t wlen)
{
    // 禁用全局中断
    GLOBAL_INT_DISABLE();

    // 等待系统配置寄存器空闲
    while (SYSCFG->ACC_CCR_BUSY);

    // 调用底层Flash读取函数
    fshc_read(offset, buff, wlen, FSH_CMD_RD);

    // 恢复全局中断
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

/**
 * @brief 获取Flash容量大小
 * @return Flash容量大小（字节数）
 * @note 通过读取Flash ID中的存储密度信息计算容量
 */
uint32_t flash_size(void)
{
    /****************************************************/
    // Byte0           | Byte1       | Byte2
    // manufacturer ID | memory type | memory density
    // 85              | 44          | 12
    /****************************************************/
    // memory density
    // 10, 64KB
    // 11, 128KB
    // 12, 256KB
    // 13, 512KB
    uint8_t mid;

    // 禁用全局中断
    GLOBAL_INT_DISABLE();
    
    // 等待系统配置寄存器空闲
    while (SYSCFG->ACC_CCR_BUSY);
    
    // 读取Flash ID，获取存储密度信息
    // Byte0: manufacturer ID, Byte1: memory type, Byte2: memory density
    mid = (fshc_rd_sta(FSH_CMD_RD_ID, 3) >> 16) & 0xFF;
    
    // 恢复全局中断
    GLOBAL_INT_RESTORE();

    // 根据存储密度计算容量
    // 10: 64KB, 11: 128KB, 12: 256KB, 13: 512KB
    return (mid > 0x20 ? 0 : (0x01UL << mid));
}
