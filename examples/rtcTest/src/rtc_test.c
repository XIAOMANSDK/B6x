/**
 ****************************************************************************************
 *
 * @file rtc_test.c
 *
 * @brief  RTC时间闹钟与ADTMR对比示例
 *
 * @details
 * 本示例演示RTC（实时时钟）的基本使用方法：
 * - 配置RTC模块并设置初始时间
 * - 设置周期闹钟，在中断中重新加载
 * - 使用高级定时器(ATMR)记录闹钟触发时间
 * - 通过GPIO输出状态指示，验证时间精度
 *
 * 工作原理：
 * 1. RTC配置：使能RTC模块，设置秒和毫秒计数器
 * 2. 闹钟设置：配置闹钟时间，支持单次或周期模式
 * 3. 中断处理：闹钟触发时重新设置并记录ATMR时间
 * 4. 时间对比：通过ATMR的微秒级精度验证RTC闹钟
 *
 * 关键寄存器：
 * - RTC_SEC: RTC秒计数器寄存器
 * - RTC_MS: RTC毫秒计数器寄存器  
 * - RTC_ALARM_SEC: 闹钟秒设置寄存器
 * - RTC_ALARM_MS: 闹钟毫秒设置寄存器
 * - RTCINT_CTRL: RTC中断控制寄存器
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

/// GPIO引脚定义 - 用于状态指示
#define GPIO_RTC            GPIO11  ///< RTC中断处理状态指示引脚
#define GPIO_TMR            GPIO12  ///< ATMR中断处理状态指示引脚  
#define GPIO_RUN            GPIO13  ///< 主循环运行状态指示引脚

/// RTC闹钟参数配置
#define RTC_ALARM_TIME      1000    ///< 闹钟时间：1000ms（1秒周期）
#define RC32K_CAL_NB        8       ///< RC32K校准次数：8次取稳定值

/// 高级定时器参数配置
#define TMR_PSC             (16 - 1) // 1us sysclk=(n)MHz
#define TMR_ARR             0xFFFF   ///< ATMR自动重载值：16位最大值

/*
 * GLOBAL VARIABLES
 ****************************************************************************************
 */

volatile bool rtcFlg;        ///< RTC中断事件标志 - 主循环处理使用
volatile uint16_t tmrCnt;    ///< ATMR中断计数器 - 记录10ms中断次数
volatile uint32_t tmrVal;    ///< ATMR计数值 - 记录闹钟触发时的精确时间


/*
 * FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief RTC中断服务函数
 *
 * @details
 * 处理RTC闹钟中断事件：
 * 1. 设置GPIO状态指示开始处理中断
 * 2. 设置中断事件标志供主循环处理
 * 3. 重新设置闹钟时间（周期模式）
 * 4. 记录ATMR当前计数值并复位计数器
 * 5. 清除GPIO状态指示
 *
 * 寄存器操作说明：
 * - RTCINT_CTRL.RTC_INT_CLR: 清除RTC中断标志
 * - RTC_ALARM_SEC/RTC_ALARM_MS: 重新设置闹钟时间
 * - ATMR->CNT: 读取ATMR计数器值
 ****************************************************************************************
 */
void RTC_IRQHandler(void)
{
    GPIO_DAT_SET(GPIO_RTC);  ///< 设置RTC中断处理状态指示
    
    rtcFlg = true;  ///< 设置RTC中断事件标志
    
    /**
     * @brief 闹钟模式选择
     * 
     * 单次模式 - 禁用中断或清除闹钟：
     * //rtc_irq_set(0); 或 rtc_alarm_set(0);
     * 
     * 周期模式 - 重新加载闹钟：
     * 调用rtc_alarm_set()重新设置相同的闹钟时间
     */
    // 单次模式 - 禁用（注释状态）
    //rtc_irq_set(0); or rtc_alarm_set(0);
    
    // 周期模式 - 重新加载闹钟
    rtc_alarm_set(RTC_ALARM_TIME);  ///< 重新设置1秒周期闹钟
    
    /**
     * @brief 记录ATMR计数值用于时间对比
     * 
     * ATMR配置为1us计数，结合中断计数器可记录长时间：
     * - tmrCnt: 记录10ms中断次数（高位）
     * - ATMR->CNT: 当前计数值（低位）
     * - 组合值 = (tmrCnt << 16) + ATMR->CNT，单位微秒
     */
    tmrVal = ATMR->CNT + ((uint32_t)tmrCnt << 16);  ///< 组合ATMR计数值，单位us
    
    /**
     * @brief 复位ATMR计数器
     * 
     * 每次RTC闹钟触发时清零ATMR计数器：
     * - ATMR->CNT = 0: 清零硬件计数器
     * - tmrCnt = 0: 清零软件计数器
     * 用于下一周期的精确时间测量
     */
    ATMR->CNT = 0;  ///< 清零ATMR硬件计数器
    tmrCnt = 0;     ///< 清零ATMR软件计数器

    GPIO_DAT_CLR(GPIO_RTC);  ///< 清除RTC中断处理状态指示
}


/**
 ****************************************************************************************
 * @brief 高级定时器中断服务函数
 *
 * @details
 * 处理ATMR更新中断（每10ms触发一次）：
 * 1. 设置GPIO状态指示开始处理中断
 * 2. 读取中断状态寄存器确认中断源
 * 3. 处理更新中断：禁用→清除→计数→使能
 * 4. 清除GPIO状态指示
 *
 * 寄存器操作说明：
 * - ATMR->RIF.Word: 读取中断标志状态
 * - ATMR->IDR.UI: 禁用更新中断
 * - ATMR->ICR.UI: 清除更新中断标志
 * - ATMR->IER.UI: 使能更新中断
 ****************************************************************************************
 */

void ATMR_IRQHandler(void)

{
    GPIO_DAT_SET(GPIO_TMR);  ///< 设置ATMR中断处理状态指示
    
    uint32_t irq_stat = ATMR->RIF.Word;  ///< 读取ATMR中断标志状态寄存器
    
    /**
     * @brief 处理更新中断（每10ms）
     * 
     * 检查UI（Update Interrupt）标志位：
     * - 每次计数器溢出时产生更新中断
     * - 中断周期 = (PSC+1)*(ARR+1)/系统时钟
     */
    if (irq_stat & 0x01/*TIMER_UI_BIT*/)  ///< 检查更新中断标志
    {
        ATMR->IDR.UI = 1;  ///< 禁用更新中断（避免嵌套中断）
        
        tmrCnt++;  ///< 递增ATMR软件计数器（每10ms加1）
        
        ATMR->ICR.UI = 1;  ///< 清除更新中断标志
        ATMR->IER.UI = 1;  ///< 重新使能更新中断
    }
    
    GPIO_DAT_CLR(GPIO_TMR);  ///< 清除ATMR中断处理状态指示
}

/**
 ****************************************************************************************
 * @brief RTC测试主函数
 *
 * @details
 * 执行RTC功能测试的完整流程：
 * 1. GPIO配置：状态指示引脚方向设置
 * 2. 时钟校准：RC32K时钟源校准确保时间精度
 * 3. RTC配置：使能RTC模块并设置闹钟
 * 4. ATMR配置：初始化高级定时器用于时间对比
 * 5. 中断使能：配置NVIC和全局中断
 * 6. 主循环：监控RTC中断事件并输出时间信息
 *
 * 测试目的：
 * - 验证RTC闹钟的周期准确性
 * - 通过ATMR验证RTC时间精度
 * - 观察中断响应和处理时间
 ****************************************************************************************
 */
void rtcTest(void)
{
    uint16_t rc32cal;        ///< RC32K校准值
    rtc_time_t time, alarm;  ///< RTC时间和闹钟时间结构体
    
    /**
     * @brief GPIO方向配置
     * 
     * 配置所有状态指示引脚为输出模式：
     * - GPIO_RTC: RTC中断处理状态
     * - GPIO_TMR: ATMR中断处理状态
     * - GPIO_RUN: 主循环运行状态
     */
    GPIO_DIR_SET_LO(GPIO_RTC | GPIO_TMR | GPIO_RUN);
    
    /**
     * @brief 时钟输出和RC32K校准
     * 
     * 1. 配置LSI时钟输出便于观察RC32K频率
     * 2. 配置RC32K校准参数（参考时钟和校准周期）
     * 3. 执行多次校准获取稳定值
     */
    iospc_clkout(CLK_OUT_LSI);  ///< 配置LSI时钟输出，便于频率测量
    
    /**
     * @brief RC32K校准配置
     * 
     * rc32k_conf(RCLK_HSE, RCAL_CYCLES(0x1F)):
     * - RCLK_HSE: 使用HSE作为参考时钟
     * - RCAL_CYCLES(0x1F): 设置校准周期数
     */
    rc32k_conf(RCLK_HSE, RCAL_CYCLES(0x1F));
    
    /**
     * @brief 执行多次RC32K校准
     * 
     * 循环执行RC32K_CAL_NB(8)次校准：
     * - 每次校准时设置GPIO_RUN指示
     * - 调用rc32k_calib()获取校准值
     * - 清除GPIO_RUN指示
     * - 输出每次校准结果
     */
    for (uint8_t i = 0; i < RC32K_CAL_NB; i++)
    {
        GPIO_DAT_SET(GPIO_RUN);        ///< 设置校准状态指示
        rc32cal = rc32k_calib();       ///< 执行RC32K校准
        GPIO_DAT_CLR(GPIO_RUN);        ///< 清除校准状态指示
        
        debug("RC32K Cal%d:0x%X\r\n", i, rc32cal);
    }
    
    /**
     * @brief RTC模块配置
     * 
     * 1. 使能RTC模块
     * 2. 设置1秒周期闹钟
     * 3. 使能RTC中断
     */
    rtc_conf(true);                    ///< 配置并使能RTC模块
    rtc_alarm_set(RTC_ALARM_TIME);     ///< 设置1秒周期闹钟
    NVIC_EnableIRQ(RTC_IRQn);          ///< 使能RTC中断
    
    /**
     * @brief 高级定时器配置
     * 
     * ATMR配置参数：
     * - 预分频：TMR_PSC(47) → 48分频，1us计数
     * - 自动重载：TMR_ARR(65535) → 最大计数65.535ms
     * - 工作模式：周期模式，更新中断
     * 
     * 时基计算：
     * - 系统时钟：48MHz
     * - 分频后：48MHz / 48 = 1MHz → 1us计数
     * - 中断周期：(47+1)×(65535+1)/48MHz ≈ 65.536ms
     */
    atmr_init(TMR_PSC, TMR_ARR);      ///< 初始化ATMR，1us时基
    atmr_ctrl(TMR_PERIOD_MODE, TMR_IR_UI_BIT);  ///< 配置为周期模式，使能更新中断
    NVIC_EnableIRQ(ATMR_IRQn);         ///< 使能ATMR中断
    
    __enable_irq();                    ///< 使能全局中断
    
    /**
     * @brief 初始时间信息输出
     * 
     * 获取并输出RTC当前时间和设置的闹钟时间：
     * - rtc_time_get(): 获取当前RTC时间（秒和毫秒）
     * - rtc_alarm_get(): 获取设置的闹钟时间
     */
    time = rtc_time_get();             ///< 获取当前RTC时间
    alarm = rtc_alarm_get();           ///< 获取闹钟设置时间
    debug("0-RTC(Time:%d.%03d,Alarm:%d.%03d)\r\n", time.sec, time.ms, alarm.sec, alarm.ms);
    
    /**
     * @brief 主循环监控
     * 
     * 持续监控RTC中断事件：
     * - 设置运行状态指示
     * - 检查RTC中断标志
     * - 输出ATMR记录的时间和当前RTC时间
     * - 清除运行状态指示
     */
    while (1)
    {
        GPIO_DAT_SET(GPIO_RUN);  ///< 设置主循环运行状态指示
        
        /**
         * @brief RTC中断事件处理
         * 
         * 当RTC闹钟中断发生时：
         * - 清除中断标志
         * - 输出ATMR记录的时间信息
         * - 输出当前RTC时间和闹钟时间
         */
        if (rtcFlg)
        {
            rtcFlg = false;  ///< 清除RTC中断事件标志
            
            /**
             * @brief 输出ATMR时间信息
             * 
             * 显示ATMR记录的精确时间：
             * - tmrCnt: 10ms中断次数
             * - tmrVal: 组合计数值（微秒）
             * 用于验证RTC闹钟的触发精度
             */
            debug("TMR(Cnt:%03d,Val:0x%X)\r\n", tmrCnt, tmrVal);
            
            /**
             * @brief 输出RTC时间信息
             * 
             * 显示当前RTC时间和下一次闹钟时间：
             * - 验证RTC时间是否正确递增
             * - 确认闹钟设置是否生效
             */
            time = rtc_time_get();    ///< 获取当前RTC时间
            alarm = rtc_alarm_get();  ///< 获取闹钟设置时间
            debug("1-RTC(Time:%d.%03d,Alarm:%d.%03d)\r\n", time.sec, time.ms, alarm.sec, alarm.ms);
        }
        
        GPIO_DAT_CLR(GPIO_RUN);  ///< 清除主循环运行状态指示
    }
}
