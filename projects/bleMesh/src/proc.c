/**
 ****************************************************************************************
 *
 * @file proc.c
 *
 * @brief user procedure.
 *
 * 用户自定义流程文件，用于实现按键检测、用户逻辑等
 ****************************************************************************************
 */

#include "b6x.h"
#include "bledef.h"
#include "drvs.h"
#include "mesh.h"

// 声明客户端函数 (仅在 CLIENT 模式下使用)
#if (MESH_APP_MODE == 1)
extern void app_mesh_send_test_cmd(void);
extern void app_mesh_switch_model(void);
#endif


#if (DBG_PROC)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, (int)__LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif


/*
 * DEFINES
 ****************************************************************************************
 */

// KEY0 按键引脚定义 (对应 PA15)
#define GPIO_KEY0       GPIO15

// KEY0 按键状态变量 (记录上一帧按键状态)
static bool key0_press = false;

/*
 * FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief 检测 KEY0 按键点击事件
 *
 * 简单的边沿检测逻辑：
 * - 按键按下时 GPIO 为低电平 (连接到 GND)
 * - 点击事件：从按下变为释放的下降沿
 * - 该函数在主循环中被周期性调用
 *
 * @return  true 检测到按键点击, false 未检测到
 ****************************************************************************************
 */
static bool key0_is_clicked()
{
    // current state (true: pressed, false: released)
    // 读取按键当前状态：低电平表示按下，高电平表示释放
    bool key0_state = ((GPIO_PIN_GET() & GPIO_KEY0) == 0);

    // click action: pressed, then released
    // 检测下降沿：上一帧按下 (key0_press=true) 且当前帧释放 (key0_state=false)
    bool click = (key0_press & !key0_state);

    // update current state - 更新状态变量，为下一帧检测做准备
    key0_press = key0_state;

    return click;
}

/**
 ****************************************************************************************
 * @brief 用户自定义处理函数
 *
 * 在主循环中周期性调用，用于：
 * - 检测用户按键输入
 * - 执行用户自定义逻辑
 * - 触发特定的 Mesh 操作
 ****************************************************************************************
 */
void user_procedure(void)
{
#if (MESH_APP_MODE == 0)
    // ========================================
    // SERVER 模式：被控制端
    // ========================================
    // 检测 KEY0 按键点击
    if (key0_is_clicked())
    {
        // Restart proxy adv - 重启 Mesh Proxy 广告
        // 使能节点开始发送 Mesh Proxy 广告包，允许智能手机通过 GATT 连接访问 Mesh 网络
        uint8_t status = ms_proxy_ctrl(MS_PROXY_ADV_NODE_START);
        DEBUG("ms_proxy_ctrl return (status:0x%X)", status);

    }

#elif (MESH_APP_MODE == 1)
    // ========================================
    // CLIENT 模式：控制端
    // ========================================
    // 检测 KEY0 按键点击
    if (key0_is_clicked())
    {
        // 发送测试控制命令
        app_mesh_send_test_cmd();
    }
    // TODO: 可添加另一个按键用于切换模型类型
    // app_mesh_switch_model();

#else
#error "MESH_APP_MODE must be 0 (Server) or 1 (Client)"
#endif

    // add user more... - 可在此处添加更多用户逻辑
}
