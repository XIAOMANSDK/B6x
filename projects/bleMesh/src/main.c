/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Main Entry of the application.
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "bledef.h"
#include "mesh.h"
#include "drvs.h"
#include "regs.h"
#include "app.h"
#include "sftmr.h"
#include "leds.h"
#include "uartRb.h"
#include "dbg.h"


/*
 * DEFINES
 ****************************************************************************************
 */


/*
 * FUNCTIONS
 ****************************************************************************************
 */

extern void user_procedure(void);

#if (PRF_MESH)
// 声明 Mesh 模式打印函数
extern void app_mesh_print_mode(void);
#endif

/**
 ****************************************************************************************
 * @brief 系统初始化函数
 *
 * 配置系统基础硬件资源：
 * - 禁用独立看门狗
 * - 使能 BLE 和 ADC 外设时钟
 * - 配置 Flash 高速时钟
 ****************************************************************************************
 */
static void sysInit(void)
{
    iwdt_disable();       // 禁用独立看门狗定时器

    rcc_ble_en();          // 使能 BLE 模块时钟
    rcc_adc_en();          // 使能 ADC 模块时钟

    rcc_fshclk_set(FSH_CLK_DPSC42);  // 设置 Flash 高速时钟分频

    // more todo, eg. Set xosc cap  // 更多初始化待完成，如设置晶振负载电容

}

/**
 ****************************************************************************************
 * @brief 设备初始化函数
 *
 * 初始化所有外设和应用模块：
 * - 获取复位原因并输出调试信息
 * - 初始化调试 UART
 * - 初始化 BLE 应用层
 * - 初始化 Mesh 协议栈
 * - 初始化按键和 LED GPIO

  LED 输出映射
  ┌─────────────┬─────────────────────────────┐
  │     LED     │            功能             │
  ├─────────────┼─────────────────────────────┤
  │ LED0 (PA08) │ 显示本地服务器状态 (被控制) │
  ├─────────────┼─────────────────────────────┤
  │ LED1 (PA09) │ 显示远程设备状态 (控制别人) │
  └─────────────┴─────────────────────────────┘

 ****************************************************************************************
 */
static void devInit(void)
{
    uint16_t rsn = rstrsn();  // 获取复位原因 (如: POR, WDT, RESET 等)

    dbgInit();               // 初始化调试输出
    debug("Start(rsn:%X)...\r\n", rsn);  // 输出启动信息和复位原因

    //uart1Rb_Init();         // UART1 环形缓冲区初始化 (可选)

    // Init BLE App
    app_init(rsn);           // 初始化 BLE 应用层，传入复位原因

    mesh_init(0);            // 初始化 Mesh 协议栈，参数 0 表示使用默认配置

    #if (LED_PLAY)
    sftmr_init();            // 初始化软件定时器
    leds_init();             // 初始化 LED 控制
    leds_play(LED_FAST_BL);  // 执行 LED 快速闪烁动画
    #endif //(LED_PLAY)

    // Init KEY0 & LED0~2 (@see B6-32D DKit-Board)
    // KEY0: PA15 按键，配置为上拉输入 (按键一端接 PA15，另一端接 GND)
    gpio_dir_input(PA15, IE_UP);
    // LED0-2: PA08-10 LED，配置为输出高电平 (LED 阳极接 VDD33，阴极接 PA08-10)
    GPIO_DIR_SET_HI(GPIO08 | GPIO09 | GPIO10);

    #if (PRF_MESH)
    // 打印当前 Mesh 应用模式
    app_mesh_print_mode();
    #endif
}

/**
 ****************************************************************************************
 * @brief 主程序入口函数
 *
 * 系统启动流程：
 * 1. 执行系统初始化 (sysInit)
 * 2. 执行设备初始化 (devInit)
 * 3. 使能全局中断
 * 4. 进入主循环，处理以下任务：
 *    - BLE 协议栈调度 (消息和事件处理)
 *    - 软件定时器轮询 (LED 动画)
 *    - 用户自定义流程 (按键处理等)
 ****************************************************************************************
 */
int main(void)
{
    sysInit();              // 系统初始化：时钟、外设时钟
    devInit();              // 设备初始化：调试、BLE、Mesh、GPIO

    // Global Interrupt Enable
    GLOBAL_INT_START();     // 使能全局中断，允许中断响应

    // main loop - 主循环，死循环运行
    while (1)
    {
        // Schedule Messages & Events
        ble_schedule();     // BLE 协议栈调度，处理消息队列和事件

        #if (LED_PLAY)
        // SoftTimer Polling
        sftmr_schedule();   // 软件定时器轮询，处理定时事件 (如 LED 闪烁)
        #endif //(LED_PLAY)

        // User's Procedure
        user_procedure();    // 用户自定义流程，如按键检测、用户逻辑处理
    }
}
