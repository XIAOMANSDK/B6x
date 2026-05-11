/**
 ****************************************************************************************
 *
 * @file cfg.h
 *
 * @brief App Configure MACRO, --preinclude
 *
 * 应用配置文件，定义各种编译选项和宏配置
 ****************************************************************************************
 */

#ifndef _APP_CFG_H_
#define _APP_CFG_H_

/// System Clock(0=16MHz, 1=32MHz, 2=48MHz, 3=64MHz)
// 系统时钟配置，选择 16MHz
#define SYS_CLK                (0)

/// Debug Mode(0=Disable, 1=via UART, 2=via RTT)
// 调试模式配置：0=关闭, 1=UART调试, 2=RTT调试
#define DBG_MODE               (1)
#define DBG_UART_BAUD          (BRR_921600)     // 调试 UART 波特率 921600
#define UART1_CONF_BAUD        (BRR_921600)     // UART1 波特率 921600
#define DBG_HARDFAULT          (0)               // 硬故障调试开关

/// Mesh App Mode - Mesh 应用模式配置
// 0: Server 模式 (被控制端，注册 Server 模型)
// 1: Client 模式 (控制端，注册 Client 模型)
// 注意：修改此宏后需重新编译生成不同的 bin 文件
//#define MESH_APP_MODE         (1)

/// Redefine BLE Heap Size, Note: Mesh's STACK Size ~2.5K
// BLE 堆大小配置 (Mesh 栈大小约 2.5K)
#define BLE_HEAP_BASE          (0x20004800)     // BLE 堆基地址
#define BLE_HEAP_ENV_SIZE      (0xD00)          // BLE 环境变量大小 (3.3KB)
#define BLE_HEAP_MSG_SIZE      (0x2000)         // BLE 消息队列大小 (8KB)

// BLE 物理层配置：1Mbps PHY
#define BLE_PHY                (GAP_PHY_LE_1MBPS)
// BLE 角色配置：外设角色 + 观察者角色 (支持广播和扫描)
#define BLE_ROLE               (GAP_ROLE_PERIPHERAL | GAP_ROLE_OBSERVER)

/// BLE Configure (Single or Multi-Connections)
// BLE 连接配置 (单设备场景不使用 GATT 连接)
#define BLE_NB_SLAVE           (0)  // 从连接数量
#define BLE_NB_MASTER          (0)  // 主连接数量

// BLE 设备地址 (多设备场景必须修改，确保每个设备唯一)
#if (MESH_APP_MODE == 1)
#define BLE_ADDR               {0x2e, 0x10, 0x49, 0x65, 0x42, 0xFC}
#define BLE_DEV_NAME           "MeshClient"      // 设备名称
#define BLE_DEV_ICON           0x0000            // 设备图标
#elif (MESH_APP_MODE == 0)
#define BLE_ADDR               {0x2e, 0x20, 0x49, 0x65, 0x42, 0xFC}
#define BLE_DEV_NAME           "MeshServer"      // 设备名称
#define BLE_DEV_ICON           0x0001            // 设备图标
#endif
// 广告间隔配置 (单位: 0.625ms, 32 * 0.625ms = 20ms)
#define APP_ADV_INT_MIN        (32)
#define APP_ADV_INT_MAX        (32)

/// Profile Configure
// Profile 功能配置开关
#define PRF_GATT               (0)  // GATT Profile (设备信息服务)
#define PRF_DISS               (0)  // Device Information Service
#define PRF_SESS               (0)  // Serial Service (串口服务)
#define PRF_MESH               (1)  // Mesh Profile (启用 Mesh)

/// Mesh Model Configure - Mesh 模型配置
// 本工程同时支持以下模型：
// 1. Generic OnOff 模型 (简单开关)
//    - 功能：简单的开关控制
//    - 状态：MM_STATE_GEN_ONOFF (0=关, 1=开)
// 2. Lighting Lightness 模型 (亮度调节)
//    - 功能：亮度调节（0-65535 级）
//    - 状态：MM_STATE_LIGHT_LN (0=关, 1-65535=亮度值)
//    - 自动关联：Generic OnOff + Generic Level 模型
// 3. Lighting CTL 模型 (色温调节)
//    - 功能：亮度 + 色温控制
//    - 状态：MM_STATE_LIGHT_CTL_LN (亮度), MM_STATE_LIGHT_CTL_TEMP (色温)
//    - 范围：亮度 0-65535, 色温 0x0320-0x4E20 (800K-20000K)
// 4. Lighting HSL 模型 (颜色调节)
//    - 功能：亮度 + 色调 + 饱和度控制
//    - 状态：MM_STATE_LIGHT_HSL_LN (亮度), MM_STATE_LIGHT_HSL_HUE (色调), MM_STATE_LIGHT_HSL_SAT (饱和度)
//    - 范围：亮度 0-65535, 色调 0-65535, 饱和度 0-65535

/// Serial Service @see prf_sess.h
// 串口服务配置
#define SES_UUID_128           (1)  // 使用 128位 UUID
#define SES_READ_SUP           (0)  // 读支持

/// Debug Configure
// 调试模块开关
#if (DBG_MODE)
    #define DBG_APP            (1)  // 应用层调试
    #define DBG_PROC           (1)  // 用户流程调试
    #define DBG_ACTV           (1)  // Activity 调试
    #define DBG_GAPM           (0)  // GAP Manager 调试
    #define DBG_GAPC           (1)  // GAP Client 调试
    #define DBG_MESH           (1)  // Mesh 调试

    #define DBG_DISS           (0)  // Device Information Service 调试
    #define DBG_SESS           (0)  // Serial Service 调试

#endif

/// Misc Options
// 其他可选功能
#define LED_PLAY               (0)  // LED 播放动画
#define CFG_SLEEP              (0)  // 睡眠模式


#endif  //_APP_CFG_H_
