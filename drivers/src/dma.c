/**
 ****************************************************************************************
 *
 * @file dma.c
 *
 * @brief Direct Memory Access(DMA) 直接内存访问驱动
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

/// DMA控制结构体在内存中的分配，大小取决于DMA通道数量
/// 包含：1通道-0x20, 2通道-0x40, 3~4通道-0x80, 5~8通道-0x100, 9~16通道-0x200, 17~32通道-0x400
__attribute__((section("DMA_ALIGN"), aligned(0x100), zero_init))
volatile DMA_CHNL_CTRL_STRUCT_Typedef dma_ctrl_base;

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 * @brief DMA控制器初始化
 * @note 启用DMA时钟，复位DMA控制器，配置控制基地址和全局使能
 */
void dma_init(void)
{
    // 启用DMA时钟和复位请求
    RCC_APBCLK_EN(APB_DMAC_BIT);   // 启用DMA APB时钟
    RCC_APBRST_REQ(APB_DMAC_BIT);  // 请求DMA复位

    // 设置DMA控制结构体基地址
    DMA->CTRLBASE_POINTER.Word = (uint32_t)&dma_ctrl_base;

    // 启用DMA控制器，配置通道保护控制
    DMA->CFG.Word = DMA_CFG_HPROT(7) | DMA_CFG_EN_BIT;
}

/**
 * @brief DMA控制器反初始化
 * @note 禁用DMA时钟，复位DMA控制器
 */
void dma_deinit(void)
{
    RCC_APBCLK_DIS(APB_DMAC_BIT);  // 禁用DMA APB时钟
    RCC_APBRST_REQ(APB_DMAC_BIT);  // 请求DMA复位
}

/**
 * @brief DMA通道初始化
 * @param chidx 通道索引
 * @param chsel 通道功能选择
 * @note 配置指定DMA通道的请求掩码和功能选择
 */
void dma_chnl_init(uint8_t chidx, uint8_t chsel)
{
    // 通道主配置，选择功能
    if (chidx < DMA_CH_MAX)
    {
        // 禁用DMA信号传输（使用突发传输）
        DMA->USEBURST_SET = (1UL << chidx);
        // 清除通道请求掩码，允许DMA请求
        DMA->REQMSK_CLR   = (1UL << chidx);

        // 配置DMA通道复用功能
        uint8_t arr = (chidx / 4);               // 计算寄存器数组索引
        uint8_t lsh = (chidx % 4) * 8;           // 计算位偏移（每个通道占8位中的6位）
        DMACHCFG->CHSEL[arr] = (DMACHCFG->CHSEL[arr] & ~(0x3FUL << lsh)) | ((uint32_t)chsel << lsh);
    }
}

/**
 * @brief DMA通道反初始化
 * @param chidx 通道索引
 * @note 禁用指定DMA通道，配置信号传输和请求掩码
 */
void dma_chnl_deinit(uint8_t chidx)
{
    uint32_t chbit = 1UL << (chidx % DMA_CH_MAX);

    // 禁用DMA通道
    DMA->CHNL_EN_CLR = chbit;
    // 禁用DMA信号传输（使用突发传输）
    DMA->USEBURST_SET = (1UL << chidx);
    // 清除通道请求掩码
    DMA->REQMSK_CLR   = (1UL << chidx);
}

/**
 * @brief DMA通道配置
 * @param chidx 通道索引
 * @param src_ep 源数据结束指针
 * @param dst_ep 目标数据结束指针
 * @param trans 传输配置数据
 * @note 配置DMA通道的源地址、目标地址和传输参数，并启用通道
 */
void dma_chnl_conf(uint8_t chidx, uint32_t src_ep, uint32_t dst_ep, uint32_t trans)
{
    DMA_CHNL_CTRL_Typedef *chnl_cur = DMA_CHNL_CTRL(chidx);

    // 通道主配置，选择功能
    // 在dma_chnl_init(chidx, chsel)中完成

    // 通道控制结构体填充
    chnl_cur->SRC_DATA_END_PTR    = src_ep;      // 设置源数据结束指针
    chnl_cur->DST_DATA_END_PTR    = dst_ep;      // 设置目标数据结束指针
    chnl_cur->TRANS_CFG_DATA.Word = trans;       // 设置传输配置数据
    chnl_cur->TRANS_CFG_RESV.Word = trans;       // 设置传输配置保留值

    // 启用DMA通道
    DMA->CHNL_EN_SET = 1UL << (chidx % DMA_CH_MAX);
}

/**
 * @brief DMA通道重载
 * @param chidx 通道索引
 * @return 当前是否使用备用通道
 * @note 重新加载DMA通道配置，支持乒乓模式
 */
bool dma_chnl_reload(uint8_t chidx)
{
    DMA_CHNL_CTRL_Typedef *chnl_cur;

    chidx = chidx % DMA_CH_MAX; // 主通道索引

    // 检查是否使用备用通道
    bool alter = (DMA->PRIALT_SET & (1UL << chidx)) ? true : false;

    chnl_cur = DMA_CHNL_CTRL(chidx);

    // 根据循环控制模式决定重载策略
    if (chnl_cur->TRANS_CFG_RESV.CYCLE_CTRL != CCM_PING_PONG)
    {
        // 非乒乓模式：直接重载配置
        chnl_cur->TRANS_CFG_DATA = chnl_cur->TRANS_CFG_RESV;
    }
    else
    {
        #if (1)
        // 乒乓模式：检查当前通道状态并重载
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
        // 备用实现：根据PRIALT_SET位决定重载哪个通道
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

    // 启用DMA通道
    DMA->CHNL_EN_SET = (1UL << chidx);
    return alter;
}

/**
 * @brief 获取DMA通道剩余传输数量
 * @param chidx 通道索引
 * @return 剩余传输数量，0表示传输完成
 * @note 读取当前通道的剩余传输计数
 */
uint16_t dma_chnl_remain(uint8_t chidx)
{
    DMA_CHNL_CTRL_Typedef *chnl_cur = DMA_CHNL_CTRL(chidx);

    // 检查循环控制是否启用，计算剩余传输数量
    if (chnl_cur->TRANS_CFG_DATA.CYCLE_CTRL)
        return (chnl_cur->TRANS_CFG_DATA.N_MINUS_1 + 1);
    else
        return 0;
}

/**
 * @brief 获取乒乓模式DMA通道剩余传输数量
 * @param chidx 通道索引
 * @param len 输出参数，存储剩余传输数量
 * @return 当前是否使用乒乓通道（pong通道）
 * @note 专门用于乒乓模式的剩余传输计数查询
 */
bool dma_chnl_remain_pingpong(uint8_t chidx, uint16_t *len)
{
    bool pong = false;
    DMA_CHNL_CTRL_Typedef *chnl_cur;

    // 确定当前使用的通道（主通道或备用通道）
    if (DMA->PRIALT_SET & (1UL << chidx))
    {
        pong = true;
        chnl_cur = DMA_CHNL_CTRL(chidx | DMA_CH_ALT);
    }
    else
    {
        chnl_cur = DMA_CHNL_CTRL(chidx);
    }

    // 计算剩余传输数量
    *len = (chnl_cur->TRANS_CFG_DATA.CYCLE_CTRL) ? (chnl_cur->TRANS_CFG_DATA.N_MINUS_1 + 1) : 0;

    return pong;
}

/**
 * @brief DMA通道控制
 * @param chidx 通道索引
 * @param ctrl 控制命令（0-禁用，1-启用，CHNL_DONE-等待完成）
 * @note 控制DMA通道的启用/禁用状态，支持等待传输完成
 */
void dma_chnl_ctrl(uint8_t chidx, uint8_t ctrl)
{
    uint32_t chbit = 1UL << (chidx % DMA_CH_MAX);

    if (ctrl)
    {
        // 启用DMA通道
        DMA->CHNL_EN_SET = chbit;

        if (ctrl == CHNL_DONE)
        {
            // 等待通道传输完成
            while ((DMACHCFG->IFLAG0 & chbit) == 0);
            // 清除完成中断标志
            DMACHCFG->ICFR0 = chbit;
        }
    }
    else
    {
        // 禁用DMA通道
        DMA->CHNL_EN_CLR = chbit;
    }
}

/**
 * @brief 检查DMA通道是否完成传输
 * @param chidx 通道索引
 * @return true-传输完成，false-传输中
 * @note 检查并清除DMA通道的完成标志
 */
bool dma_chnl_done(uint8_t chidx)
{
    uint32_t chbit = 1UL << (chidx % DMA_CH_MAX);

    // 检查通道完成中断标志
    if (DMACHCFG->IFLAG0 & chbit)
    {
        // 清除完成中断标志，DMA->CHNL_EN会自动清除
        DMACHCFG->ICFR0 = chbit;
        return true;
    }
    else
    {
        return false;
    }
}
