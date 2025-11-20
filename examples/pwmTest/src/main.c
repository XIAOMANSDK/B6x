/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief 应用程序主入口 - PWM输出示例
 *
 * @details
 * 本示例演示PWM输出功能的基本使用方法：
 * - 配置通用定时器或高级定时器的PWM输出
 * - 设置不同通道的占空比和输出极性
 * - 支持DMA动态更新占空比
 * - 通过GPIO输出PWM波形
 *
 * 工作原理：
 * 1. 定时器初始化：配置预分频和自动重载值，确定PWM频率
 * 2. 通道配置：设置CCMR（比较模式）和CCER（输出极性）
 * 3. 占空比设置：通过CCR寄存器控制高电平时间
 * 4. 定时器启动：开始计数并输出PWM信号
 *
 * 关键寄存器：
 * - TIMx_ARR: 自动重载寄存器（决定PWM周期）
 * - TIMx_CCRx: 捕获/比较寄存器（决定PWM占空比）
 * - TIMx_CCMRx: 捕获/比较模式寄存器（配置PWM模式）
 * - TIMx_CCER: 捕获/比较使能寄存器（配置输出极性）
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "drvs.h"
#include "dbg.h"


/*
 * DEFINES
 ****************************************************************************************
 */

#define PWM_TMR_PSC            (16 - 1) ///< PWM预分频值 - 16MHz系统时钟分频到1MHz
#define PWM_TMR_ARR            (100 - 1) ///< PWM自动重载值 - 决定PWM周期

/**
 * @brief 定时器选择配置
 * 
 * 根据CTMR_USED选择使用通用定时器或高级定时器：
 * - CTMR_USED=1: 使用通用定时器(CTMR)，4个独立通道
 * - CTMR_USED=0: 使用高级定时器(ATMR)，4个主通道+3个互补通道
 */
#if (CTMR_USED)
#define PA_CTMR_CH1            (15) ///< 通用定时器通道1引脚
#define PA_CTMR_CH2            (16) ///< 通用定时器通道2引脚
#define PA_CTMR_CH3            (17) ///< 通用定时器通道3引脚
#define PA_CTMR_CH4            (18) ///< 通用定时器通道4引脚
#else
#define PA_ATMR_CH1P           (7)  ///< 高级定时器通道1正输出引脚
#define PA_ATMR_CH2P           (8)  ///< 高级定时器通道2正输出引脚
#define PA_ATMR_CH3P           (9)  ///< 高级定时器通道3正输出引脚
#define PA_ATMR_CH4P           (10) ///< 高级定时器通道4正输出引脚
#define PA_ATMR_CH1N           (11) ///< 高级定时器通道1负输出引脚
#define PA_ATMR_CH2N           (12) ///< 高级定时器通道2负输出引脚
#define PA_ATMR_CH3N           (13) ///< 高级定时器通道3负输出引脚
#endif

/**
 * @brief DMA支持配置
 * 
 * 可选使用DMA动态更新PWM占空比：
 * - 定义DMA通道和缓冲区
 * - 配置DMA传输参数
 */
#if (DMA_USED)
#if (CTMR_USED)
#define PWM_CTMR_DMA_CHNL     (DMA_CH0) ///< 通用定时器DMA通道
#else
#define PWM_ATMR_DMA_CHNL     (DMA_CH1) ///< 高级定时器DMA通道
#endif
#define PWM_DUTY_CNT          (10)      ///< DMA占空比缓冲区长度

uint32_t pwm_duty_buff0[PWM_DUTY_CNT]; ///< DMA主缓冲区
uint32_t pwm_duty_buff1[PWM_DUTY_CNT]; ///< DMA备用缓冲区

#define PA_DONE_SEE           (2)       ///< DMA完成状态指示引脚
#endif

/**
 * @brief PWM输出极性配置
 * 
 * 根据DMA使用情况配置CCER寄存器：
 * - 不使用DMA：基本PWM输出
 * - 使用DMA：PWM输出+DMA请求使能
 */
#if (DMA_USED)
#define CFG_PWM_CCER_SIPH      (PWM_CCER_SIPH | PWM_CCxDE_BIT) ///< 高电平有效 + DMA使能
#define CFG_PWM_CCER_SIPL      (PWM_CCER_SIPL | PWM_CCxDE_BIT) ///< 低电平有效 + DMA使能
#else
#define CFG_PWM_CCER_SIPH      (PWM_CCER_SIPH) ///< 高电平有效
#define CFG_PWM_CCER_SIPL      (PWM_CCER_SIPL) ///< 低电平有效
#endif


/*
 * FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief PWM测试主函数
 *
 * @details
 * 配置PWM输出的完整流程：
 * 1. DMA初始化（如使用）：配置占空比缓冲区和DMA通道
 * 2. GPIO配置：设置PWM输出引脚功能
 * 3. 定时器初始化：配置PSC和ARR确定PWM频率
 * 4. 通道配置：设置各通道的占空比和输出极性
 * 5. 定时器启动：开始PWM输出
 * 6. DMA配置（如使用）：启动DMA传输动态更新占空比
 *
 * PWM频率计算：
 * - 系统时钟：16MHz
 * - 预分频：PWM_TMR_PSC = 15 → 分频后时钟 = 16MHz / 16 = 1MHz
 * - 自动重载：PWM_TMR_ARR = 99 → PWM频率 = 1MHz / 100 = 10kHz
 * - 占空比分辨率：1% (100级)
 ****************************************************************************************
 */
static void pwmTest(void)
{
    /**
     * @brief DMA初始化（如启用）
     * 
     * 配置DMA用于动态更新PWM占空比：
     * - 初始化占空比缓冲区
     * - 配置DMA模块和通道
     */
    #if (DMA_USED)
    uint16_t idx = 0;
    
    GPIO_DIR_SET_LO(1 << PA_DONE_SEE); ///< 配置DMA完成指示引脚为输出
    dma_init(); ///< 初始化DMA模块
    
    /**
     * @brief 初始化占空比缓冲区
     * 
     * 生成递增和递减的占空比序列：
     * - pwm_duty_buff0: 10%, 20%, ..., 100%
     * - pwm_duty_buff1: 100%, 90%, ..., 10%
     */
    for (idx = 0; idx < PWM_DUTY_CNT; idx++)
    {
        pwm_duty_buff0[idx] = (idx + 1) * 10 - 1;     ///< 递增占空比序列
        pwm_duty_buff1[idx] = (PWM_DUTY_CNT - idx) * 10 - 1; ///< 递减占空比序列
    }
    #endif
    
    /**
     * @brief GPIO引脚功能配置
     * 
     * 根据使用的定时器类型配置对应的PWM输出引脚：
     * - 通用定时器：配置CTMR_CH1~CH4引脚为TIMER功能
     * - 高级定时器：配置ATMR_CH1P~CH3N引脚为TIMER功能
     */
    #if (CTMR_USED)
    iom_ctrl(PA_CTMR_CH1, IOM_SEL_TIMER); ///< 配置CTMR通道1为TIMER功能
    iom_ctrl(PA_CTMR_CH2, IOM_SEL_TIMER); ///< 配置CTMR通道2为TIMER功能
    iom_ctrl(PA_CTMR_CH3, IOM_SEL_TIMER); ///< 配置CTMR通道3为TIMER功能
    iom_ctrl(PA_CTMR_CH4, IOM_SEL_TIMER); ///< 配置CTMR通道4为TIMER功能
    #else
    iom_ctrl(PA_ATMR_CH1P, IOM_SEL_TIMER); ///< 配置ATMR通道1正输出为TIMER功能
    iom_ctrl(PA_ATMR_CH2P, IOM_SEL_TIMER); ///< 配置ATMR通道2正输出为TIMER功能
    iom_ctrl(PA_ATMR_CH3P, IOM_SEL_TIMER); ///< 配置ATMR通道3正输出为TIMER功能
    iom_ctrl(PA_ATMR_CH4P, IOM_SEL_TIMER); ///< 配置ATMR通道4正输出为TIMER功能
    iom_ctrl(PA_ATMR_CH1N, IOM_SEL_TIMER); ///< 配置ATMR通道1负输出为TIMER功能
    iom_ctrl(PA_ATMR_CH2N, IOM_SEL_TIMER); ///< 配置ATMR通道2负输出为TIMER功能
    iom_ctrl(PA_ATMR_CH3N, IOM_SEL_TIMER); ///< 配置ATMR通道3负输出为TIMER功能
    #endif
    
    pwm_chnl_cfg_t chnl_conf; ///< PWM通道配置结构体
    
    /**
     * @brief 配置PWM通道工作模式
     * 
     * PWM_CCMR_MODE1 = PWM_OCxPE_BIT | PWM_OCxM(6)
     * - PWM_OCxPE_BIT: 输出比较预加载使能
     * - PWM_OCxM(6): PWM模式1 - 在向上计数时，当TIMx_CNT<TIMx_CCR1时输出有效电平
     */
    chnl_conf.ccmr = PWM_CCMR_MODE1; ///< 配置为PWM模式1，预加载使能
    
    /**
     * @brief 通用定时器(CTMR)配置
     * 
     * 配置4个独立通道，分别设置不同的占空比和输出极性：
     * - CH1: 15%占空比，高电平有效
     * - CH2: 25%占空比，低电平有效  
     * - CH3: 35%占空比，高电平有效
     * - CH4: 45%占空比，低电平有效
     */
    #if (CTMR_USED)
    // 通用定时器配置
    pwm_init(PWM_CTMR, PWM_TMR_PSC, PWM_TMR_ARR); ///< 初始化通用定时器

    chnl_conf.duty = 15; ///< 设置占空比为15%
    /**
     * @brief 通道1配置：15%高电平，85%低电平
     * 
     * CFG_PWM_CCER_SIPH = PWM_CCER_SIPH (PWM_CCxE_BIT)
     * - PWM_CCxE_BIT: 通道输出使能
     * - 高电平有效：计数器值小于CCR时输出高电平
     */
    chnl_conf.ccer = CFG_PWM_CCER_SIPH; ///< 高电平有效配置
    pwm_chnl_set(PWM_CTMR_CH1, &chnl_conf); ///< 应用通道1配置

    chnl_conf.duty = 25; ///< 设置占空比为25%
    /**
     * @brief 通道2配置：25%低电平，75%高电平
     * 
     * CFG_PWM_CCER_SIPL = PWM_CCER_SIPL (PWM_CCxE_BIT | PWM_CCxP_BIT)
     * - PWM_CCxE_BIT: 通道输出使能
     * - PWM_CCxP_BIT: 输出极性反转（低电平有效）
     */
    chnl_conf.ccer = CFG_PWM_CCER_SIPL; ///< 低电平有效配置
    pwm_chnl_set(PWM_CTMR_CH2, &chnl_conf); ///< 应用通道2配置
    
    chnl_conf.duty = 35; ///< 设置占空比为35%
    chnl_conf.ccer = CFG_PWM_CCER_SIPH; ///< 高电平有效配置
    pwm_chnl_set(PWM_CTMR_CH3, &chnl_conf); ///< 应用通道3配置

    chnl_conf.duty = 45; ///< 设置占空比为45%
    chnl_conf.ccer = CFG_PWM_CCER_SIPL; ///< 低电平有效配置
    pwm_chnl_set(PWM_CTMR_CH4, &chnl_conf); ///< 应用通道4配置
    
    pwm_start(PWM_CTMR); ///< 启动通用定时器
    #else
    /**
     * @brief 高级定时器(ATMR)配置
     * 
     * 配置互补PWM输出，支持正负输出对：
     * - CH1P/CH1N: 10%占空比，互补输出
     * - CH2P/CH2N: 20%占空比，互补输出
     * - CH3P/CH3N: 30%占空比，互补输出
     * - CH4P: 40%占空比，单端输出
     */
    // 高级定时器配置
    pwm_init(PWM_ATMR, PWM_TMR_PSC, PWM_TMR_ARR); ///< 初始化高级定时器
    
    chnl_conf.duty = 10; ///< 设置占空比为10%
    chnl_conf.ccer = CFG_PWM_CCER_SIPH; ///< 高电平有效配置
    /**
     * @brief 通道1正负输出配置
     * 
     * CH1P: 10%高电平，90%低电平
     * CH1N: 10%低电平，90%高电平（与CH1P互补）
     */
    pwm_chnl_set(PWM_ATMR_CH1P, &chnl_conf); ///< 应用通道1正输出配置
    pwm_chnl_set(PWM_ATMR_CH1N, &chnl_conf); ///< 应用通道1负输出配置
    
    chnl_conf.duty = 20; ///< 设置占空比为20%
    chnl_conf.ccer = CFG_PWM_CCER_SIPL; ///< 低电平有效配置
    /**
     * @brief 通道2正负输出配置
     * 
     * CH2P: 20%低电平，80%高电平
     * CH2N: 20%高电平，80%低电平（与CH2P互补）
     */
    pwm_chnl_set(PWM_ATMR_CH2P, &chnl_conf); ///< 应用通道2正输出配置
    pwm_chnl_set(PWM_ATMR_CH2N, &chnl_conf); ///< 应用通道2负输出配置
    
    chnl_conf.duty = 30; ///< 设置占空比为30%
    chnl_conf.ccer = CFG_PWM_CCER_SIPH; ///< 高电平有效配置
    pwm_chnl_set(PWM_ATMR_CH3P, &chnl_conf); ///< 应用通道3正输出配置
    pwm_chnl_set(PWM_ATMR_CH3N, &chnl_conf); ///< 应用通道3负输出配置
    
    chnl_conf.duty = 40; ///< 设置占空比为40%
    chnl_conf.ccer = CFG_PWM_CCER_SIPL; ///< 低电平有效配置
    pwm_chnl_set(PWM_ATMR_CH4P, &chnl_conf); ///< 应用通道4正输出配置
    
    pwm_start(PWM_ATMR); ///< 启动高级定时器
    #endif
    
    /**
     * @brief DMA配置（如启用）
     * 
     * 配置DMA自动更新PWM占空比：
     * - 初始化DMA通道
     * - 配置传输参数和模式
     * - 主循环中监控DMA完成状态
     */
    #if (DMA_USED)
    #if (CTMR_USED)
    /**
     * @brief 通用定时器DMA配置
     * 
     * DMA_CTMR_CHx_INIT(PWM_CTMR_DMA_CHNL, 4):
     * - 初始化CTMR DMA通道，配置为通道4更新
     * - 使用基本传输模式(CCM_BASIC)
     */
    DMA_CTMR_CHx_INIT(PWM_CTMR_DMA_CHNL, 4); ///< 初始化CTMR DMA通道
    DMA_CTMR_CHx_CONF(PWM_CTMR_DMA_CHNL, 4, pwm_duty_buff0, PWM_DUTY_CNT, CCM_BASIC); ///< 配置DMA传输
    //DMA_CTMR_CHx_CONF(PWM_CTMR_DMA_CHNL | DMA_CH_ALT, 4, pwm_duty_buff1, PWM_DUTY_CNT, CCM_PING_PONG); ///< 备用配置（注释状态）
    #else
    /**
     * @brief 高级定时器DMA配置
     * 
     * DMA_ATMR_CHx_INIT(PWM_ATMR_DMA_CHNL, 1):
     * - 初始化ATMR DMA通道，配置为通道1更新
     * - 使用乒乓传输模式(CCM_PING_PONG)
     */
    DMA_ATMR_CHx_INIT(PWM_ATMR_DMA_CHNL, 1); ///< 初始化ATMR DMA通道
    DMA_ATMR_CHx_CONF(PWM_ATMR_DMA_CHNL, 1, pwm_duty_buff0, PWM_DUTY_CNT, CCM_PING_PONG); ///< 配置主缓冲区传输
    DMA_ATMR_CHx_CONF(PWM_ATMR_DMA_CHNL | DMA_CH_ALT, 1, pwm_duty_buff1, PWM_DUTY_CNT, CCM_PING_PONG); ///< 配置备用缓冲区传输
    #endif

    /**
     * @brief DMA监控主循环
     * 
     * 持续检查DMA传输状态：
     * - 当DMA传输完成时，通过GPIO指示
     * - 重新配置DMA进行下一次传输
     */
    while (1)
    {
        #if (CTMR_USED)
        if (dma_chnl_done(PWM_CTMR_DMA_CHNL)) ///< 检查CTMR DMA传输是否完成
        {
            GPIO_DAT_SET(1 << PA_DONE_SEE); ///< 设置完成指示
            //dma_chnl_reload(PWM_CTMR_DMA_CHNL); ///< 重新加载DMA（注释状态）
            DMA_CTMR_CHx_CONF(PWM_CTMR_DMA_CHNL, 4, pwm_duty_buff0, PWM_DUTY_CNT, CCM_BASIC); ///< 重新配置DMA传输
            GPIO_DAT_CLR(1 << PA_DONE_SEE); ///< 清除完成指示
        }
        #else
        if (dma_chnl_done(PWM_ATMR_DMA_CHNL)) ///< 检查ATMR DMA传输是否完成
        {
            GPIO_DAT_SET(1 << PA_DONE_SEE); ///< 设置完成指示
            dma_chnl_reload(PWM_ATMR_DMA_CHNL); ///< 重新加载DMA
            GPIO_DAT_CLR(1 << PA_DONE_SEE); ///< 清除完成指示
        }
        #endif

        //idx = (1 + idx) % PWM_DUTY_CNT; ///< 占空比索引更新（注释状态）
        //pwm_duty_upd(PWM_CTMR_CH1, pwm_duty_buff0[idx]); ///< 手动更新占空比（注释状态）
    }
    #endif
}

/**
 ****************************************************************************************
 * @brief 系统初始化函数
 *
 * @details
 * 配置系统基础设置
 ****************************************************************************************
 */
static void sysInit(void)
{
    // Todo config, if need - 可在此添加系统时钟等配置
}

/**
 ****************************************************************************************
 * @brief 设备初始化函数
 *
 * @details
 * 初始化外设模块：
 * - 禁用看门狗
 * - 初始化调试接口
 * - 输出测试开始信息
 ****************************************************************************************
 */
static void devInit(void)
{
    iwdt_disable();  ///< 禁用独立看门狗
    
    dbgInit();                ///< 初始化调试接口
    debug("PWM Test...\r\n"); ///< 输出测试开始信息
}

/**
 ****************************************************************************************
 * @brief 主函数
 *
 * @return int 程序退出状态
 *
 * @details
 * 程序执行主流程：
 * 系统初始化 → 设备初始化 → PWM测试
 ****************************************************************************************
 */
int main(void)
{
    sysInit();    ///< 系统初始化
    devInit();    ///< 设备初始化
    
    pwmTest();    ///< 执行PWM测试
    while (1);    ///< 主循环保持程序运行
}
