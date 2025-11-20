/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Main Entry of the application.
 *
 * @details 代码运行过程和原理：
 * 1. 系统初始化：配置系统时钟和外设
 * 2. 设备初始化：关闭看门狗，初始化调试串口
 * 3. GPIO测试：配置输入输出引脚，实现引脚状态监控和响应
 *    - 配置PA02、PA03、PA04、PA05为输出引脚
 *    - 配置PA08、PA09、PA10、PA11为输入引脚（带上下拉）
 *    - 循环检测输入引脚状态变化
 *    - 根据输入变化控制输出引脚电平
 *    - 支持复位引脚功能切换（GPIO/nRESET）
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

/// 测试引脚定义
enum test_pad
{
    // 输出引脚
    PA_OUT00            = PA02,
    PA_OUT01            = PA03,
    PA_OUT10            = PA04,
    PA_OUT11            = PA05,
    // 输入引脚
    PA_IN00             = PA08,
    PA_IN01             = PA09,
    PA_IN10             = PA10,
    PA_IN11             = PA11,
};

/// GPIO输出引脚位掩码
#define GPIO_OUT0        (BIT(PA_OUT00) | BIT(PA_OUT01))
#define GPIO_OUT1        (BIT(PA_OUT10) | BIT(PA_OUT11))
/// GPIO输入引脚位掩码
#define GPIO_IN0         (BIT(PA_IN00)  | BIT(PA_IN01))
#define GPIO_IN1         (BIT(PA_IN10)  | BIT(PA_IN11))


/*
 * FUNCTIONS
 ****************************************************************************************
 */

static uint32_t pin_val; ///< 当前引脚状态值
static uint32_t pin_chg; ///< 引脚状态变化标志
static bool io_rst;      ///< 复位引脚模式标志：0-nReset功能，1-GPIO功能

/**
 * @brief 将输入引脚状态映射到对应的输出引脚
 * @param[in] val 输入引脚状态值
 * @return 对应的输出引脚位掩码
 * @note 实现输入到输出的简单映射关系：
 *        PA_IN00 -> PA_OUT00
 *        PA_IN01 -> PA_OUT01  
 *        PA_IN10 -> PA_OUT10
 *        PA_IN11 -> PA_OUT11
 */
static uint32_t padIn2Out(uint32_t val)
{
    uint32_t out = 0;
    
    // 映射GPIO输入到输出
    if (val & BIT(PA_IN00)) out |= BIT(PA_OUT00);
    if (val & BIT(PA_IN01)) out |= BIT(PA_OUT01);
    if (val & BIT(PA_IN10)) out |= BIT(PA_OUT10);
    if (val & BIT(PA_IN11)) out |= BIT(PA_OUT11);
    
    return out;
}

/**
 * @brief GPIO测试函数
 * @details 配置输入输出引脚，监控输入变化并控制输出响应
 *          支持复位引脚功能动态切换
 */
static void gpioTest(void)
{
    uint32_t tmp_val;
    
    // 恢复nRESET功能
    io_rst = 0;
    iospc_rstpin(0);  // CSC->CSC_PIO[PA19]配置为复位功能
    
    // 初始化GPIO
    debug("0-gpio(dir:0x%X,dat:0x%X,pin:0x%X)\r\n", 
                GPIO_DIR_GET(), GPIO_DAT_GET(), GPIO_PIN_GET());
    GPIO_DAT_CLR(GPIO_OUT0);             // 设置GPIO_OUT0为低电平
    GPIO_DAT_SET(GPIO_OUT1);             // 设置GPIO_OUT1为高电平
    GPIO_DIR_SET(GPIO_OUT0 | GPIO_OUT1); // 使能输出引脚方向控制
    
    GPIO_DIR_CLR(GPIO_IN0 | GPIO_IN1);   // 禁用输入引脚输出使能
    gpio_dir_input(PA_IN00, IE_DOWN);    // 配置PA_IN00为下拉输入
    gpio_dir_input(PA_IN01, IE_DOWN);    // 配置PA_IN01为下拉输入
    gpio_dir_input(PA_IN10, IE_UP);      // 配置PA_IN10为上拉输入
    gpio_dir_input(PA_IN11, IE_UP);      // 配置PA_IN11为上拉输入
    debug("1-gpio(dir:0x%X,dat:0x%X,pin:0x%X)\r\n", 
                GPIO_DIR_GET(), GPIO_DAT_GET(), GPIO_PIN_GET());
    
    // 初始化引脚状态值（上拉输入默认高电平）
    pin_val = GPIO_IN1;
    
    while (1)
    {
        // 引脚状态扫描
        tmp_val = gpio_get_all() & (GPIO_IN0 | GPIO_IN1); // 获取输入引脚当前值
        pin_chg = tmp_val ^ pin_val;  // 计算引脚状态变化
        pin_val = tmp_val;            // 更新当前引脚状态
        
        // 处理引脚状态变化
        if (pin_chg)
        {
            uint32_t posedge = pin_val & pin_chg;   // 上升沿检测
            uint32_t negedge = ~pin_val & pin_chg;  // 下降沿检测
            
            // 处理上升沿：对应输出引脚置高
            if (posedge)
            {
                GPIO_DAT_SET(padIn2Out(posedge)); // 根据输入映射设置输出
            }
            
            // 处理下降沿：对应输出引脚置低
            if (negedge)
            {
                GPIO_DAT_CLR(padIn2Out(negedge)); // 根据输入映射清除输出
                
                // PA_IN11下降沿触发复位引脚功能切换
                if (negedge & BIT(PA_IN11))
                {
                    // 切换复位引脚模式
                    io_rst = !io_rst;
                    iospc_rstpin(io_rst);  // 配置PA19为GPIO或nRESET功能
                    
                    // 如果配置为GPIO模式，设置输出方向
                    if (io_rst)
                    {
                        GPIO_DIR_SET_LO(BIT(PA_RSTPIN)); // CSC->CSC_PIO[PA19]方向控制
                    }
                }
            }
            
            // 如果复位引脚为GPIO模式，在每次输入变化时翻转电平
            if (io_rst)
            {
                GPIO_DAT_TOG(BIT(PA_RSTPIN)); // 翻转PA19引脚电平
            }
            
            debug("pin trg(val:0x%X,chg:0x%X,pos:0x%X,neg:0x%X)\r\n", 
                        pin_val, pin_chg, posedge, negedge);
        }
    }
}

/**
 * @brief 系统初始化函数
 * @note 预留系统时钟和外设配置接口
 */
static void sysInit(void)
{
    // Todo config, if need
    // 可在此配置系统时钟、电源管理等
}

/**
 * @brief 设备初始化函数
 * @details 关闭看门狗，初始化调试接口
 */
static void devInit(void)
{
    uint16_t rsn = rstrsn();  // 读取复位原因
    
    iwdt_disable();           // 禁用独立看门狗
    
    dbgInit();                // 初始化调试接口
    debug("Start(rsn:0x%X)...\r\n", rsn);  // 输出启动信息和复位原因
}

/**
 * @brief 主函数入口
 * @return 程序退出码（通常不会返回）
 */
int main(void)
{
    sysInit();    // 系统初始化
    devInit();    // 设备初始化
    
    gpioTest();   // 执行GPIO测试
}
