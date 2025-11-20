/**
 ****************************************************************************************
 *
 * @file sadc_test.c
 *
 * @brief SADC API usage demo.
 *
 * @details 本示例代码演示了SADC(ADC)模块的使用方法，包括基本ADC读取和DMA传输两种模式。
 *          代码运行过程：
 *          1. 初始化GPIO引脚为模拟输入模式
 *          2. 配置SADC模块的模拟参数
 *          3. 可选择使用DMA进行数据传输或直接读取ADC值
 *          4. 在DMA模式下，数据通过UART发送输出
 *          5. 在非DMA模式下，直接读取并打印ADC值
 *
 * @note 工作原理：
 *        - SADC模块是10位精度的模数转换器，支持10个输入通道
 *        - 通过配置不同的采样模式可以实现单次采样、连续采样、PCM语音采样等
 *        - DMA模式可以实现高效的数据传输，减少CPU开销
 *        - 模拟前端支持可编程增益放大器(PGA)和麦克风偏置电路
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

#if (DMA_USED)
    #define DMA_CH_SADC            (0)                    ///< DMA通道号
    #define ADC_SW_AUTO            (0)                    ///< ADC自动通道切换控制: 0-单通道, ((2 << 1) | 0x01) - 3通道切换
    #define ADC_CH_CTRL            ((SADC_CH_AIN7<<0) | (SADC_CH_AIN8<<4) | (SADC_CH_AIN9<<8))  ///< ADC通道控制字

    #define GPIO_RX_PING           GPIO12                 ///< Ping缓冲区指示GPIO
    #define GPIO_RX_PONG           GPIO13                 ///< Pong缓冲区指示GPIO
    
    #if (SAMP_ADTMR)
    #define PWM_TMR_PSC            (0)                    ///< PWM定时器预分频
    #define PWM_TMR_ARR            (250 - 1)              ///< PWM定时器自动重载值: 249对应64K, 999对应16K
    #define PA_ADTMR_CH0           (PA14)                 ///< ADTMR通道0引脚
    #define PA_ADTMR_CH1           (PA15)                 ///< ADTMR通道1引脚
    #endif
    
    #if (SAMP_PCM)
    #define SAMP_NUM                (128)                 ///< PCM采样点数
    int16_t pcm_buff[SAMP_NUM*2];                         ///< PCM双缓冲区
    #else
    #define SAMP_NUM                (64)                  ///< ADC采样点数
    uint16_t adc_buff[SAMP_NUM];                          ///< ADC数据缓冲区
    #endif
#endif //(DMA_USED)

/**
 ****************************************************************************************
 * @brief UART发送数据函数
 *
 * @param[in] data 待发送数据指针
 * @param[in] len  数据长度
 ****************************************************************************************
 */
void uartTxSend(const uint8_t *data, uint16_t len)
{
    while(len--)
    {
        uart_putc(0, *data++);  // UART发送单个字节
    }
}

/*
 * FUNCTIONS
 ****************************************************************************************
 */

#if (SAMP_ADTMR)
/**
 ****************************************************************************************
 * @brief ADTMR PWM配置函数
 *
 * @details 配置高级定时器产生PWM信号，用于触发ADC采样
 ****************************************************************************************
 */
static void adtmr_pwm(void)
{
    RCC_APBCLK_EN(APB_ATMR_BIT);  // 使能ATMR时钟
    iom_ctrl(PA_ADTMR_CH0, IOM_SEL_TIMER);  // 配置引脚为定时器功能

    pwm_init(PWM_ATMR, PWM_TMR_PSC, PWM_TMR_ARR);  // 初始化PWM
    
    pwm_chnl_cfg_t chnl_conf;
    chnl_conf.duty = 50;                                // 占空比50%
    chnl_conf.ccer = PWM_CCER_SIPH | PWM_CCxDE_BIT;     // PWM输出极性配置
    chnl_conf.ccmr = PWM_CCMR_MODE1;                    // PWM模式1
    pwm_chnl_set(PWM_ATMR_CH0, &chnl_conf);             // 配置PWM通道
    
    pwm_start(PWM_ATMR);  // 启动PWM
}
#endif

/**
 ****************************************************************************************
 * @brief SADC测试主函数
 *
 * @details 演示SADC模块的基本使用方法和DMA传输功能
 ****************************************************************************************
 */
void sadcTest(void)
{
    // 配置GPIO为输入模式
    GPIO_DIR_CLR(GPIO08 | GPIO09 | GPIO10);  // 清除方向寄存器，设为输入

    // 配置ADC输入引脚为模拟模式
    iom_ctrl(PA08, IOM_ANALOG);  // AIN7通道
    iom_ctrl(PA09, IOM_ANALOG);  // AIN8通道  
    iom_ctrl(PA10, IOM_ANALOG);  // AIN9通道
    
    // SADC初始化
    #if (SAMP_PCM)
    sadc_init(SADC_ANA_DFLT | SADC_INBUF_BYPSS_BIT);  // PCM模式，旁路输入缓冲器
    #else
    sadc_init(SADC_ANA_DFLT);  // 默认模拟配置
    #endif
    
#if (DMA_USED) 
    // 配置指示GPIO为输出
    GPIO_DIR_SET_LO(GPIO_RX_PING | GPIO_RX_PONG);  // 设置GPIO方向为输出
    
    dma_init();  // 初始化DMA控制器
    
    DMA_SADC_INIT(DMA_CH_SADC);  // 初始化SADC DMA通道
    
    #if (SAMP_PCM)
    // PCM模式配置
    DMA_SADC_PCM_CONF(DMA_CH_SADC, pcm_buff, SAMP_NUM, CCM_PING_PONG);           // 主缓冲区配置
    DMA_SADC_PCM_CONF(DMA_CH_SADC | DMA_CH_ALT, pcm_buff+SAMP_NUM, SAMP_NUM, CCM_PING_PONG);  // 备用缓冲区配置
    // SADC配置: 时钟上升沿采样，HPF系数3，时钟分频0，单次转换模式
    sadc_conf((SADC_CR_DFLT & ~(SADC_CR_HPF_COEF_MSK|SADC_CR_CLK_DIV_MSK)) | SADC_CR_HPF(3) | SADC_CR_CLK(0));
    sadc_pcm(SADC_MIC_DFLT & (~SADC_PGA_VOL_MSK | SADC_PGA_VOL(0)));  // PCM麦克风配置
    #elif (SAMP_ADTMR)
    // ADTMR触发模式配置
    DMA_SADC_AUX_CONF(DMA_CH_SADC, adc_buff, SAMP_NUM, CCM_BASIC);  // 基本DMA模式
    sadc_atmr(ADC_SW_AUTO, ADC_CH_CTRL);  // 配置ATMR触发
    #else
    // 普通DMA模式配置
    DMA_SADC_AUX_CONF(DMA_CH_SADC, adc_buff, SAMP_NUM, CCM_BASIC);  // 基本DMA模式
    sadc_dma(ADC_SW_AUTO, ADC_CH_CTRL);  // 配置SADC DMA
    #endif
    
    // 主循环 - 处理DMA传输完成事件
    while (1)
    {
        if (dma_chnl_done(DMA_CH_SADC))  // 检查DMA通道是否完成传输
        {
            #if (SAMP_PCM)
            if (dma_chnl_reload(DMA_CH_SADC))  // 检查是否为Ping缓冲区
            {
                // Ping缓冲区数据处理
                GPIO_DAT_SET(GPIO_RX_PING);  // 设置Ping指示
                uartTxSend((uint8_t *)&pcm_buff, SAMP_NUM*2);  // 通过UART发送PCM数据
                GPIO_DAT_CLR(GPIO_RX_PING);  // 清除Ping指示
            }
            else
            {
                // Pong缓冲区数据处理  
                GPIO_DAT_SET(GPIO_RX_PONG);  // 设置Pong指示
                uartTxSend((uint8_t *)&pcm_buff[SAMP_NUM], SAMP_NUM*2);  // 通过UART发送PCM数据
                GPIO_DAT_CLR(GPIO_RX_PONG);  // 清除Pong指示
            }
            #else
            debug("DMA-Read:\r\n");  // 调试信息
            debugHex(adc_buff, SAMP_NUM);  // 十六进制格式输出ADC数据
            #endif
        }
    }
#else // !(DMA_USED)
    // 非DMA模式 - 直接读取ADC值
    for (uint8_t i = 0; i < 10; i++)
    {
        uint16_t adc_data = sadc_read(SADC_CH_AIN8, 0);  // 读取AIN8通道ADC值
        debug("adc_data:0x%x\r\n", adc_data);  // 输出ADC值
    }
    
    while (1);  // 主循环
#endif
}
