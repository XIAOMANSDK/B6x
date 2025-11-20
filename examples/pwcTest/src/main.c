/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief 应用程序主入口 - 定时器捕获功能示例
 *
 * @details
 * 本示例演示定时器捕获功能的基本使用方法：
 * - 配置定时器通道为输入捕获模式
 * - 同时捕获上升沿和下降沿事件
 * - 计算脉冲宽度、周期、频率和占空比
 * - 通过中断处理实现精确时间测量
 *
 * 工作原理：
 * 1. 初始化：配置定时器、GPIO和捕获通道参数
 * 2. 捕获触发：当检测到边沿事件时，硬件自动保存计数器值到CCR寄存器
 * 3. 中断处理：读取CCR值，计算时间间隔，更新测量结果
 * 4. 结果显示：在主循环中输出测量参数
 *
 * 关键寄存器：
 * - TIMx_CCMR1: 捕获/比较模式寄存器1（配置输入滤波、预分频、通道选择）
 * - TIMx_CCER: 捕获/比较使能寄存器（配置输出极性、使能）
 * - TIMx_CCR1: 捕获/比较寄存器1（存储捕获的时间戳）
 * - TIMx_SR: 状态寄存器（捕获中断标志）
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "drvs.h"
#include "regs.h"
#include "dbg.h"


/*
 * DEFINES
 ****************************************************************************************
 */

#define PA_RET_SEE1           (9)   ///< 状态指示引脚1 - 用于观察中断处理时间
#define PA_CAP_CH1            (16)  ///< 捕获通道1输入引脚

#define POS_NEG_EDGE          (3)   ///< 边沿触发模式选择

/**
 * @brief 边沿触发模式配置
 * 
 * 根据POS_NEG_EDGE的值选择不同的捕获模式：
 * - 1: 仅上升沿触发
 * - 2: 仅下降沿触发  
 * - 3: 双边沿触发（支持高电平和低电平宽度测量）
 */
#if (POS_NEG_EDGE == 1)
#define PA_TMR_EDGE           (PWC_CCER_POSEDGE)  ///< 上升沿触发
#define PWC_CC1S              (1)                 ///< CC1通道配置为输入，TI1作为IC1输入
#define PWC_SMCR_TS_SMS       (0x54)              ///< SMCR.TS=5(TI1FP1), SMCR.SMS=4(复位模式)
#elif (POS_NEG_EDGE == 2)
#define PA_TMR_EDGE           (PWC_CCER_NEGEDGE)  ///< 下降沿触发
#define PWC_CC1S              (1)                 ///< CC1通道配置为输入，TI1作为IC1输入
#define PWC_SMCR_TS_SMS       (0x54)              ///< SMCR.TS=5(TI1FP1), SMCR.SMS=4(复位模式)
#elif (POS_NEG_EDGE == 3)
#define PA_TMR_EDGE           (PWC_CCER_POSEDGE | PWC_CCER_NEGEDGE)  ///< 双边沿触发
#define PWC_CC1S              (3)                 ///< CC1通道配置为输入，TRC作为IC1输入
#define PWC_SMCR_TS_SMS       (0x44)              ///< SMCR.TS=4(TI1F_ED), SMCR.SMS=4(复位模式)
#endif

/**
 * @brief 定时器选择配置
 * 
 * 根据CTMR_USED选择使用通用定时器或高级定时器：
 * - CTMR_USED=1: 使用通用定时器(CTMR)
 * - CTMR_USED=0: 使用高级定时器(ATMR)
 */
#if (CTMR_USED)
#define PWC_TMR               (PWM_CTMR)         ///< 通用定时器
#define PWC_TMR_CH(n)         (PWM_CTMR_CH##n)   ///< 通用定时器通道
#define PWC_IRQc              (CTMR_IRQn)        ///< 通用定时器中断
#define PA_PWC_CH1            (16)               ///< 通用定时器通道1引脚
#else
#define PWC_TMR               (PWM_ATMR)         ///< 高级定时器
#define PWC_TMR_CH(n)         (PWM_ATMR_CH##n##P) ///< 高级定时器通道
#define PWC_IRQc              (ATMR_IRQn)        ///< 高级定时器中断
#define PA_PWC_CH1            (7)                ///< 高级定时器通道1引脚
#endif

#define TIMER_INT_CH1_BIT     (0x02U)            ///< 通道1中断标志位

#define TIMER_INT_CH1_BIT     (0x02U)
#define TIMER_INT_CH2_BIT     (0x04U)
#define TIMER_INT_CH3_BIT     (0x08U)
#define TIMER_INT_CH4_BIT     (0x10U)
/*
 * GLOBAL VARIABLES
 ****************************************************************************************
 */

/**
 * @brief 捕获测量结果结构体
 * 
 * 存储脉冲测量的各项参数：
 * - 高电平宽度、低电平宽度、周期、频率、占空比
 * - 最近一次捕获值和时间戳
 */
typedef struct {
    uint32_t high_width;    ///< 高电平宽度（定时器计数）
    uint32_t low_width;     ///< 低电平宽度（定时器计数）  
    uint32_t period;        ///< 信号周期（定时器计数）
    float frequency;        ///< 信号频率（Hz）
    float duty_cycle;       ///< 占空比（百分比）
    uint32_t last_capture;  ///< 上一次捕获值
    uint32_t last_time;     ///< 上一次时间戳
} capture_result_t;

volatile capture_result_t g_cap_result = {0};  ///< 全局捕获结果


/*
 * FUNCTIONS
 ****************************************************************************************
 */

#if (CTMR_USED)
/**
 ****************************************************************************************
 * @brief 通用定时器中断服务函数
 *
 * @details
 * 处理通用定时器的捕获中断：
 * - 检查通道1捕获中断标志
 * - 读取捕获寄存器获取精确时间戳
 * - 计算脉冲宽度和周期参数
 * - 清除中断标志
 *
 * 寄存器操作说明：
 * - CTMR->CCR1: 捕获/比较寄存器1（存储捕获时的计数器值）
 * - CTMR->ICR.CC1I: 中断清除寄存器CC1I位（清除通道1中断标志）
 ****************************************************************************************
 */
void CTMR_IRQHandler(void)
{
    uint32_t iflg = CTMR->IFM.Word;  ///< 读取中断标志寄存器
    
    if (iflg & TIMER_INT_CH1_BIT)  ///< 检查通道1捕获中断
    {
        GPIO_DAT_SET(1 << PA_RET_SEE1);  ///< 状态指示 - 开始处理中断
        
        /**
         * @brief 读取当前捕获值并计算时间间隔
         * 
         * CTMR->CCR1: 捕获/比较寄存器1，存储捕获事件发生时的计数器值
         * 通过连续两次捕获值的时间差计算脉冲参数
         */
        uint32_t current_capture = CTMR->CCR1 + 1;  ///< 读取当前捕获值（补偿+1）
        uint32_t time_diff = current_capture - g_cap_result.last_capture;  ///< 计算时间间隔
        
        /**
         * @brief 根据边沿类型更新测量结果
         * 
         * 通过检查输入引脚电平判断当前边沿类型：
         * - 高电平：刚发生上升沿，前一个间隔为低电平宽度
         * - 低电平：刚发生下降沿，前一个间隔为高电平宽度
         */
        if (gpio_get(PA_PWC_CH1) ^ 0x01)  ///< 检查当前输入电平（异或处理电平反转）
        {
            // 当前为高电平，刚发生上升沿
            g_cap_result.low_width = time_diff;  ///< 前一个间隔为低电平宽度
        }
        else
        {
            // 当前为低电平，刚发生下降沿  
            g_cap_result.high_width = time_diff;  ///< 前一个间隔为高电平宽度
        }
        
        /**
         * @brief 计算周期和频率
         * 
         * 周期 = 高电平宽度 + 低电平宽度
         * 频率 = 定时器频率 / 周期
         * 占空比 = 高电平宽度 / 周期 × 100%
         */
        g_cap_result.period = g_cap_result.high_width + g_cap_result.low_width;
        if (g_cap_result.period > 0)
        {
            // 定时器时钟为16MHz，预分频15999，实际计数频率为1kHz
            g_cap_result.frequency = 1000.0f / g_cap_result.period;  ///< 计算频率（Hz）
            g_cap_result.duty_cycle = (float)g_cap_result.high_width / g_cap_result.period * 100.0f;  ///< 计算占空比
        }
        
        g_cap_result.last_capture = current_capture;  ///< 更新上一次捕获值
        
        debug("TIME:[%d ms] LEVEL:[%d] H:%d L:%d P:%d F:%.2fHz D:%.1f%%\r\n", 
              current_capture, 
              gpio_get(PA_PWC_CH1) ^ 0x01,
              g_cap_result.high_width,
              g_cap_result.low_width, 
              g_cap_result.period,
              g_cap_result.frequency,
              g_cap_result.duty_cycle);  ///< 输出测量结果
        
        CTMR->ICR.CC1I = 1;  ///< 清除通道1捕获中断标志
        
        GPIO_DAT_CLR(1 << PA_RET_SEE1);  ///< 状态指示 - 中断处理完成
    }
}
#else
/**
 ****************************************************************************************
 * @brief 高级定时器中断服务函数
 *
 * @details
 * 处理高级定时器的捕获中断，功能与通用定时器类似
 ****************************************************************************************
 */
void ATMR_IRQHandler(void)
{
    uint32_t iflg = ATMR->IFM.Word;  ///< 读取中断标志寄存器
    
    if (iflg & TIMER_INT_CH1_BIT)  ///< 检查通道1捕获中断
    {
        GPIO_DAT_SET(1 << PA_RET_SEE1);  ///< 状态指示 - 开始处理中断
        
        ATMR->ICR.CC1I = 1;  ///< 清除通道1捕获中断标志
        
        GPIO_DAT_CLR(1 << PA_RET_SEE1);  ///< 状态指示 - 中断处理完成
    }
}
#endif

/**
 ****************************************************************************************
 * @brief 捕获功能测试函数
 *
 * @details
 * 配置定时器捕获功能的完整流程：
 * 1. GPIO引脚配置：状态指示和输入捕获引脚
 * 2. 定时器初始化：预分频和自动重载值设置
 * 3. 捕获通道配置：输入模式、边沿触发、滤波参数
 * 4. 从模式控制：配置触发源和从模式
 * 5. 中断使能：NVIC和全局中断使能
 *
 * 关键寄存器配置：
 * - CCMR1: 配置通道为输入模式、滤波器、预分频器
 * - CCER: 配置输出极性、捕获使能
 * - SMCR: 从模式控制，配置触发源和复位模式
 ****************************************************************************************
 */
static void captureTest(void)
{
    // GPIO引脚初始化
    GPIO_DAT_CLR((1 << PA_RET_SEE1) | (1 << PA_PWC_CH1));  ///< 清除状态指示和输入引脚
    GPIO_DIR_SET((1 << PA_RET_SEE1));                      ///< 配置状态指示引脚为输出
    GPIO_DIR_CLR((1 << PA_PWC_CH1));                       ///< 配置捕获输入引脚为输入
    
    /**
     * @brief 配置捕获输入引脚
     * 
     * 根据使用的定时器类型选择不同的引脚复用功能：
     * - 通用定时器：使用CSC（交叉开关）功能
     * - 高级定时器：使用TIMER专用功能
     */
    #if (CTMR_USED)
        // PA0~PA19范围，使用CSC功能
        iom_ctrl(PA_PWC_CH1, IOM_PULLDOWN | IOM_INPUT | IOM_SEL_CSC);  ///< 配置为上拉输入，CSC功能
        csc_input(PA_PWC_CH1, CSC_CTMR_CH1);  ///< 映射到通用定时器通道1输入
    #else    
        // 高级定时器使用专用TIMER功能
        iom_ctrl(PA_PWC_CH1, IOM_PULLDOWN | IOM_INPUT | IOM_SEL_TIMER);  ///< 配置为上拉输入，TIMER功能
    #endif
    
    // 定时器初始化 - 1ms时基
    /**
     * @brief 初始化PWM定时器
     * 
     * pwm_init(PWC_TMR, 15999, UINT16_MAX):
     * - 定时器时钟：16MHz
     * - 预分频值：15999 → 计数频率 = 16MHz / (15999+1) = 1kHz
     * - 自动重载值：UINT16_MAX(65535) → 最大周期65.535秒
     * - 实际时基：1ms
     */
    pwm_init(PWC_TMR, 15999, UINT16_MAX);  ///< 初始化定时器，1ms时基
    
    // 捕获通道配置
    pwm_chnl_cfg_t chnl_cfg;  ///< 通道配置结构体
    
    chnl_cfg.duty = 0;  ///< 占空比设置为0（输入模式时不使用）
    
    /**
     * @brief 配置捕获/比较模式寄存器(CCMR1)
     * 
     * PWC_CCMR_MODE(PWC_CC1S, 3, PWC_PSC0)参数：
     * - PWC_CC1S: 捕获/比较选择（输入模式选择）
     * - 3: IC1F输入滤波器设置（具体滤波参数）
     * - PWC_PSC0: 输入捕获预分频器（无分频）
     * 
     * CCMR1寄存器位说明：
     * - CC1S[1:0]: 通道配置为输入模式
     * - IC1F[3:0]: 输入捕获滤波器
     * - IC1PSC[1:0]: 输入捕获预分频器
     */
    chnl_cfg.ccmr = PWC_CCMR_MODE(PWC_CC1S, 3, PWC_PSC0);  ///< 配置CCMR1寄存器
    
    /**
     * @brief 配置捕获/比较使能寄存器(CCER)
     * 
     * 双边沿触发模式(3)使用特殊配置，其他模式使用PA_TMR_EDGE
     * CCER寄存器位说明：
     * - CC1E: 捕获/比较1输出使能（输入模式下为使能捕获）
     * - CC1P: 捕获/比较1输出极性（输入模式下为边沿极性选择）
     */
    #if (POS_NEG_EDGE != 3)
        chnl_cfg.ccer = PA_TMR_EDGE;  ///< 单边沿触发配置
    #endif

    pwm_chnl_set(PWC_TMR_CH(1), &chnl_cfg);  ///< 应用通道配置
    
    /**
     * @brief 配置从模式控制寄存器(SMCR)
     * 
     * PWC_SMCR_TS_SMS: 从模式选择
     * - TS[2:0]: 触发选择（TI1FP1或TI1F_ED）
     * - SMS[2:0]: 从模式选择（复位模式）
     * 
     * TIMER_INT_CH1_BIT | TIMER_INT_CH2_BIT: 使能通道1和2中断
     */
    pwm_conf(PWC_TMR, PWC_SMCR_TS_SMS, TIMER_INT_CH1_BIT | TIMER_INT_CH2_BIT); 
    
    pwm_start(PWC_TMR);  ///< 启动定时器
    
    // 中断使能配置
    NVIC_EnableIRQ(PWC_IRQc);  ///< 使能定时器中断
    __enable_irq();            ///< 使能全局中断
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
    
    dbgInit();                    ///< 初始化调试接口
    debug("Capture Test...\r\n"); ///< 输出测试开始信息
}

/**
 ****************************************************************************************
 * @brief 主函数
 *
 * @return int 程序退出状态
 *
 * @details
 * 程序执行主流程：
 * 系统初始化 → 设备初始化 → 捕获功能测试
 ****************************************************************************************
 */
int main(void)
{
    sysInit();    ///< 系统初始化
    devInit();    ///< 设备初始化
    
    captureTest();  ///< 执行捕获功能测试
    
    while (1);      ///< 主循环保持程序运行
}
