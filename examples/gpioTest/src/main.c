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

/// IN pins (PA08-PA11) are 6 positions above OUT pins (PA02-PA05)
#define PAD_IN_OUT_SHIFT    6

/**
 * @brief Map input pin bits to output pin bits via fixed offset
 * @param[in] val Input pin bitmask
 * @return Output pin bitmask
 */
static uint32_t pad_in_to_out(uint32_t val)
{
    return (val >> PAD_IN_OUT_SHIFT);
}

/**
 * @brief GPIO test function
 * @details Configure input/output pins, monitor input changes and control output response
 *          Support reset pin function switching
 */
static void gpio_test(void)
{
    uint32_t tmp_val;
    uint32_t pin_val;    // Current pin state
    uint32_t pin_chg;    // Pin state change flag
    bool io_rst = false; // Reset pin mode: 0-nReset, 1-GPIO

    // Restore nRESET function
    io_rst = 0;
    iospc_rstpin(0);  // CSC->CSC_PIO[PA19] configured as reset function

    // Initialize GPIO
    debug("0-gpio(dir:0x%" PRIx32 ",dat:0x%" PRIx32 ",pin:0x%" PRIx32 ")\r\n",
                GPIO_DIR_GET(), GPIO_DAT_GET(), GPIO_PIN_GET());
    GPIO_DAT_CLR(GPIO_OUT0);             // Set GPIO_OUT0 to low level
    GPIO_DAT_SET(GPIO_OUT1);             // Set GPIO_OUT1 to high level
    GPIO_DIR_SET(GPIO_OUT0 | GPIO_OUT1); // Enable output pin direction control

    GPIO_DIR_CLR(GPIO_IN0 | GPIO_IN1);   // Disable input pin output enable
    gpio_dir_input(PA_IN00, IE_DOWN);    // Configure PA_IN00 as pull-down input
    gpio_dir_input(PA_IN01, IE_DOWN);    // Configure PA_IN01 as pull-down input
    gpio_dir_input(PA_IN10, IE_UP);      // Configure PA_IN10 as pull-up input
    gpio_dir_input(PA_IN11, IE_UP);      // Configure PA_IN11 as pull-up input
    debug("1-gpio(dir:0x%" PRIx32 ",dat:0x%" PRIx32 ",pin:0x%" PRIx32 ")\r\n",
                GPIO_DIR_GET(), GPIO_DAT_GET(), GPIO_PIN_GET());

    // Initialize pin state values (pull-up input defaults to high level)
    pin_val = GPIO_IN1;

    while (1)
    {
        // Pin state scan
        tmp_val = gpio_get_all() & (GPIO_IN0 | GPIO_IN1); // Get current input pin values
        pin_chg = tmp_val ^ pin_val;  // Calculate pin state change
        pin_val = tmp_val;            // Update current pin state

        // Handle pin state changes
        if (pin_chg)
        {
            uint32_t posedge = pin_val & pin_chg;   // 上升沿检测
            uint32_t negedge = ~pin_val & pin_chg;  // 下降沿检测

            // Handle rising edge: set corresponding output pin high
            if (posedge)
            {
                GPIO_DAT_SET(pad_in_to_out(posedge)); // Set output based on input mapping
            }

            // Handle falling edge: set corresponding output pin low
            if (negedge)
            {
                GPIO_DAT_CLR(pad_in_to_out(negedge)); // Clear output based on input mapping

                // PA_IN11 falling edge triggers reset pin function toggle
                if (negedge & BIT(PA_IN11))
                {
                    // Toggle reset pin mode
                    io_rst = !io_rst;
                    iospc_rstpin(io_rst);  // Configure PA19 as GPIO or nRESET function

                    // If configured as GPIO mode, set output direction
                    if (io_rst)
                    {
                        GPIO_DIR_SET_LO(BIT(PA_RSTPIN)); // CSC->CSC_PIO[PA19] direction control
                    }
                }
            }

            // If reset pin is in GPIO mode, toggle on each input change
            if (io_rst)
            {
                GPIO_DAT_TOG(BIT(PA_RSTPIN)); // Toggle PA19 pin level
            }

            debug("pin trg(val:0x%" PRIx32 ",chg:0x%" PRIx32 ",pos:0x%" PRIx32 ",neg:0x%" PRIx32 ")\r\n",
                        pin_val, pin_chg, posedge, negedge);
        }
    }
}

/**
 * @brief System initialization function
 * @note Reserved for system clock and peripheral configuration
 */
static void sys_init(void)
{
    // Todo config, if need
    // Configure system clock, power management, etc.
}

/**
 * @brief Device initialization function
 * @details Disable watchdog, initialize debug interface
 */
static void dev_init(void)
{
    uint16_t rsn = rstrsn();  // Read reset reason

    iwdt_disable();           // Disable independent watchdog

    dbgInit();                // Initialize debug interface
    debug("Start(rsn:0x%X)...\r\n", rsn);  // Output boot info and reset reason
}

/**
 * @brief Main entry point
 * @return Program exit code (normally does not return)
 */
int main(void)
{
    sys_init();    // System initialization
    dev_init();    // Device initialization

    gpio_test();   // Run GPIO test
}
