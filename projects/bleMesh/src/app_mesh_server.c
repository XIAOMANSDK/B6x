/**
 ****************************************************************************************
 *
 * @file app_mesh_server.c
 *
 * @brief Mesh Server Application (被控制端)
 *
 *   模型关系图
 *
 *  ┌─────────────────────────────────────────────────────────────────┐
 *  │                    Generic OnOff Server                         │
 *  │                    (简单开关: 0/1)                              │
 *  └─────────────────────────────────────────────────────────────────┘
 *                             ↑
 *  ┌─────────────────────────────────────────────────────────────────┐
 *  │               Lighting Lightness Server                         │
 *  │         (亮度调节: 0x0000-0xFFFF)                               │
 *  │         ┌─────────────────────────────┐                         │
 *  │         │  Generic Level Server      │ (自动关联)               │
 *  │         └─────────────────────────────┘                         │
 *  └─────────────────────────────────────────────────────────────────┘
 *                             ↑
 *  ┌─────────────────────────────────────────────────────────────────┐
 *  │                 Lighting CTL Server                             │
 *  │         ┌─────────────────────────────┐                         │
 *  │         │   Light Lightness Server   │ (亮度 0-65535)           │
 *  │         │   CTL Temperature Server   │ (色温 800K-20000K)       │
 *  │         └─────────────────────────────┘                         │
 *  └─────────────────────────────────────────────────────────────────┘
 *                             ↑
 *  ┌─────────────────────────────────────────────────────────────────┐
 *  │                 Lighting HSL Server                             │
 *  │         ┌─────────────────────────────┐                         │
 *  │         │   Light Lightness Server   │ (亮度 0-65535)           │
 *  │         │   Light Hue Server        │ (色调 0-65535)            │
 *  │         │   Light Saturation Server │ (饱和度 0-65535)          │
 *  │         └─────────────────────────────┘                         │
 *  └─────────────────────────────────────────────────────────────────┘
 ****************************************************************************************
 */

#if (PRF_MESH)

#include "app.h"
#include "drvs.h"
#include "mesh.h"
#include "sig_model.h"
#include "genie_mesh.h"
#include "mm_light.h"

#if (DBG_MESH)
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

#define LLS_LIGHT               0
#define CTL_LIGHT               1
#define HSL_LIGHT               2

#define LIGHT_TYPE              (LLS_LIGHT)


/**
 ****************************************************************************************
 * @brief 打印 Mesh Server 模式信息
 ****************************************************************************************
 */
void app_mesh_print_mode(void)
{
    debug("\r\n");
    debug("========================================\r\n");
    debug("Mesh Application Mode: Server \r\n");
    debug("========================================\r\n");
    debug("Server Moudle:\r\n");
    debug("  - Generic OnOff Server\r\n");
    debug("  - Lighting Lightness Server\r\n");
    debug("  - Lighting CTL Server\r\n");
    debug("  - Lighting HSL Server\r\n");
    debug("========================================\r\n");
    debug("\r\n");
}


/*
* GLOBAL VARIABLES DECLARATIONS
****************************************************************************************
*/

/**
 * @brief Genie Mesh 三元组配置
 *
 * Genie Mesh 是涂鸦智能的 BLE Mesh 协议，三元组用于设备身份认证：
 * - pid: Product ID (4字节)，产品唯一标识
 * - key: 产品密钥 (16字节)，用于加密通信
 * - mac: 设备 MAC 地址 (6字节)，必须是唯一地址
 * - crc: 校验码 (2字节)
 *
 * 注意：多设备场景下必须修改 mac 地址，确保每个设备地址唯一
 */
const tri_tuple_t genie_triple =
{
    /* pid - 4B */
    .pid = 0x006653de,
    /* key - 16B */
    .key = {0x4e, 0x14, 0x96, 0x96, 0xfc, 0x16, 0x41, 0xca, 0x3b, 0x8d, 0x0b, 0x5a, 0xaf, 0x5f, 0xee, 0xba},
    /* mac - 6B MSB */
    .mac = {0xfc, 0x42, 0x65, 0x49, 0x20, 0x2e}, // *MUST Changed for multi device
    /* crc - 2B */
    .crc = 0x0000,
};


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief 初始化 Genie 三元组
 *
 * 从 Flash 存储读取三元组配置
 * Demo 示例直接使用 const 数据，实际项目应从 Flash 读取
 ****************************************************************************************
 */
static void genie_triple_init(void)
{
    // Read value from storage, here use const data for demo.
}

/**
 ****************************************************************************************
 * @brief 配置 Provisioning 参数
 *
 * 为设备配置 Provisioning 入网所需参数：
 * - 生成设备 UUID (基于三元组)
 * - 配置 OOB (Out-of-Band) 认证信息
 *
 * @param param  Provisioning 参数结构体指针
 ****************************************************************************************
 */
static void genie_prov_param_conf(ms_prov_param_t *param)
{
    // 生成设备 UUID (基于三元组 pid 和 mac)
    genie_gen_uuid(param->dev_uuid, &genie_triple);

    DEBUG("Dev UUID:");
    debugHex(param->dev_uuid, 16);

    // 配置 Provisioning 参数
    param->static_oob     = 0x01;      // 支持 Static OOB 认证
    param->uri_hash       = 0x0000;    // URI 哈希值
    param->oob_info       = 0x00;      // OOB 信息标志
    param->pub_key_oob    = 0x00;      // 公钥 OOB 方式
    param->out_oob_size   = 0x00;      // 输出 OOB 数据大小
    param->in_oob_size    = 0x00;      // 输入 OOB 数据大小
    param->out_oob_action = 0x00;      // 输出 OOB 动作类型
    param->in_oob_action  = 0x00;      // 输入 OOB 动作类型
    param->info           = 0x00;      // 其他信息
}

/**
 ****************************************************************************************
 * @brief 生成 OOB 认证数据
 *
 * 基于 Genie 三元组计算 16 字节的认证数据
 *
 * @param p_oob  输出的 OOB 认证数据缓冲区
 ****************************************************************************************
 */
static void genie_oob_auth_conf(uint8_t *p_oob)
{
    // 基于三元组计算认证数据
    genie_calc_auth(p_oob, &genie_triple);

    DEBUG("OOB Data:");
    debugHex(p_oob, 16);
}

/**
 ****************************************************************************************
 * @brief 创建并初始化 Mesh Server 应用
 *
 * Mesh 应用层初始化流程：
 * 1. 初始化 Genie 三元组
 * 2. 设置设备 SSID (Subnet ID)
 * 3. 注册 Mesh Server 模型 (同时支持多种模型)
 * 4. 启动 Mesh Bearer (广告和广播)
 *
 * 本工程同时注册以下 Server 模型：
 * - Generic OnOff (简单开关)
 * - Lighting Lightness (亮度调节)
 * - Lighting CTL (色温调节)
 * - Lighting HSL (颜色调节)
 *  ┌────────────────────┬──────────────────────────┬────────────────────┐
 *  │        模型        │        Server API        │      功能说明      │
 *  ├────────────────────┼──────────────────────────┼────────────────────┤
 *  │ Generic OnOff      │ mm_gens_oo_register()    │ 简单开关 (0/1)     │
 *  ├────────────────────┼──────────────────────────┼────────────────────┤
 *  │ Lighting Lightness │ mm_lights_ln_register()  │ 亮度调节 (0-65535) │
 *  ├────────────────────┼──────────────────────────┼────────────────────┤
 *  │ Lighting CTL       │ mm_lights_ctl_register() │ 亮度+色温控制      │
 *  ├────────────────────┼──────────────────────────┼────────────────────┤
 *  │ Lighting HSL       │ mm_lights_hsl_register() │ 亮度+色调+饱和度   │
 *  └────────────────────┴──────────────────────────┴────────────────────┘
 ****************************************************************************************
 */
void app_mesh_create(void)
{
    m_lid_t mdl_lid;  // 模型本地 ID

    // 初始化 Genie 三元组 (从存储读取)
    genie_triple_init();

    // Config SSID - 设置子网 ID
    // GENIE_MESH_CID: Genie Mesh 公司 ID
    // 参数: cid, pid, vid, location
    ms_set_ssid(GENIE_MESH_CID, 0/*pid*/, 0/*vid*/, 0/*loc*/);

    // ========================================
    // 1. Generic OnOff Server 模型 (简单开关)
    // ========================================
    // 注册通用开关服务器模型 (Generic OnOff Server)，支持 1 个元素，使能发布
    mdl_lid = mm_gens_oo_register(1, true);
    DEBUG("mm_gens_oo, mdl_lid=%d", mdl_lid);

    #if (LIGHT_TYPE == LLS_LIGHT)
    // ========================================
    // 2. Lighting Lightness Server 模型 (亮度调节)
    // ========================================
    // 注册 Light Lightness Server 模型
    // 参数: element_index, main
    // 注意: Lighting 模型会自动注册关联的 Generic OnOff 和 Generic Level 模型
    mdl_lid = mm_lights_ln_register(2, true);
    DEBUG("mm_lights_ln, mdl_lid=%d", mdl_lid);
    #elif (LIGHT_TYPE == CTL_LIGHT)
    // ========================================
    // 3. Lighting CTL Server 模型 (色温调节)
    // ========================================
    // 注册 Light CTL Server 模型
    // 参数: element_index, main
    // 自动关联: Light Lightness + Generic OnOff + Generic Level
    mdl_lid = mm_lights_ctl_register(2, true);
    DEBUG("mm_lights_ctl, mdl_lid=%d", mdl_lid);
    #elif (LIGHT_TYPE == HSL_LIGHT)
    // ========================================
    // 4. Lighting HSL Server 模型 (颜色调节)
    // ========================================
    // 注册 Light HSL Server 模型
    // 参数: element_index, main
    // 自动关联: Light Lightness + Generic OnOff + Generic Level
    mdl_lid = mm_lights_hsl_register(2, true);
    DEBUG("mm_lights_hsl, mdl_lid=%d", mdl_lid);
    #endif

    // Start Mesh Bearer - 启动 Mesh 传输层 (开始发送广告)
    mesh_enable();
}


/*
 * MESSAGE HANDLERS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief  MESH Service Message Handler
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Mesh 状态指示消息处理器
 *
 * 处理 Mesh 协议栈发送的状态通知消息：
 * - MS_PROV_STATE_IND: Provisioning 状态变更
 * - MS_PROXY_ADV_UPD_IND: Proxy 广告状态更新
 ****************************************************************************************
 */
APP_MSG_HANDLER(ms_state_ind)
{
    DEBUG("ms_state_ind(op:%d,v1:%d,v2:%d)", param->ind_code, param->state, param->status);

    switch (param->ind_code)
    {
        case MS_PROV_STATE_IND:
        {
            DEBUG("    PROV_STATE:%d", param->state);
        } break;

        case MS_PROXY_ADV_UPD_IND:
        {
            DEBUG("    PROXY_ADV(sta:%d,rsn:%d)", param->state, param->status);
        } break;

        default:
        {
            // todo more...
        } break;
    }
}

APP_MSG_HANDLER(ms_fndh_fault_ind)
{
    DEBUG("ms_fault_ind(op:%d,tst:%d,cid:0x%X)", param->ind_code, param->test_id, param->comp_id);

    if ( (param->ind_code == MS_FAULT_GET_REQ_IND)
        || (param->ind_code == MS_FAULT_TEST_REQ_IND) )
    {
        //ms_fndh_fault_rsp(true, param->comp_id, param->test_id, , );
    }
}

APP_MSG_HANDLER(ms_fndh_period_ind)
{
    DEBUG("ms_period_ind(ms:%d,fault_ms:%d)", param->period_ms, param->period_fault_ms);

}

/**
 ****************************************************************************************
 * @brief Provisioning 数据请求消息处理器
 *
 * 处理 Provisioning 过程中的数据请求：
 * - MS_PROV_PARAM_DATA_REQ_IND: 请求 Provisioning 参数
 * - MS_PROV_COMPO_DATA_REQ_IND: 请求设备组成数据
 * - MS_PROV_AUTH_DATA_REQ_IND: 请求 OOB 认证数据
 ****************************************************************************************
 */
APP_MSG_HANDLER(ms_prov_data_req_ind)
{
    DEBUG("ms_prov_data_req(op:%d,val1:0x%02x,val2:0x%04X)", param->req_code, param->value1, param->value2);

    switch (param->req_code)
    {
        case MS_PROV_PARAM_DATA_REQ_IND:
        {
            ms_prov_param_t param;

            // 配置并返回 Provisioning 参数
            genie_prov_param_conf(&param);
            ms_prov_param_rsp(&param);
        } break;

        case MS_PROV_COMPO_DATA_REQ_IND:
        {
            // 仅在设备组成数据页数 > 1 时发生
            //ms_compo_data_rsp(param->value1/*page*/, /*length*/, /*p_data*/);
        } break;

        default: //MS_PROV_AUTH_DATA_REQ_IND
        {
            uint8_t oob_auth[16];

            // 计算 OOB 认证数据并返回
            genie_oob_auth_conf(oob_auth);
            ms_prov_auth_rsp(true, 16, oob_auth);
        } break;
    }
}

#if (BLE_MESH_LPN)
APP_MSG_HANDLER(ms_lpn_offer_ind)
{
    DEBUG("ms_lpn_offer_ind(addr:0x%X,cnt:%d,offer:0x%X)", param->frd_addr, param->frd_cnt, param->wd_offer);

}
#endif //(BLE_MESH_LPN)

APP_MSG_HANDLER(ms_conf_load_ind)
{
    DEBUG("ms_conf_load_ind(typ:0x%X,idx:%d,sta:0x%X)", param->cfg_type, param->index, param->status);

}

APP_MSG_HANDLER(ms_conf_update_ind)
{
    DEBUG("ms_conf_update_ind(typ:0x%X,val1:%d,val2:0x%X)", param->upd_type, param->value1, param->value2);

}

/**
 ****************************************************************************************
 * @brief  MESH Model Message Handler
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Mesh 模型指示消息处理器
 *
 * 处理 Mesh 模型发送的状态通知：
 * - MM_SRV_STATE_UPD_IND: 服务器模型状态更新 (响应控制命令)
 * - MM_CLI_STATE_IND: 客户端模型状态指示 (接收状态变更通知)
 ****************************************************************************************
 */
APP_MSG_HANDLER(mm_ind)
{
    DEBUG("mm_ind(op:%d)", param->ind_code);

    switch (param->ind_code)
    {
        case MM_SRV_STATE_UPD_IND:
        {
            // 服务器模型状态更新：收到控制命令后更新本地状态
            struct mm_srv_state_upd_ind *p_ind = (struct mm_srv_state_upd_ind *)param;

            DEBUG("    SRV_STATE_UPD(elt:%d,sid:%d,state:0x%X,ttms:%d)",
                    p_ind->elmt_idx, p_ind->state_id, p_ind->state, p_ind->trans_time_ms);

            // ========================================
            // Generic OnOff 模型状态处理
            // ========================================
            if (p_ind->state_id == MM_STATE_GEN_ONOFF)  // 通用开关状态
            {
                if (p_ind->state)
                {
                    GPIO_DAT_CLR(GPIO08); // LED0 开启 (拉低电平)
                }
                else
                {
                    GPIO_DAT_SET(GPIO08); // LED0 关闭 (拉高电平)
                }
            }
            // ========================================
            // Lighting Lightness 模型状态处理
            // ========================================
            else if (p_ind->state_id == MM_STATE_LIGHT_LN)  // Light Lightness 状态
            {
                // Lightness 值范围: 0x0000 (关闭) ~ 0xFFFF (最亮)
                uint16_t lightness = (uint16_t)p_ind->state;

                // 简单示例：根据 lightness 值控制 LED0 亮度
                // 注意：实际硬件可能需要 PWM 输出才能实现调光
                if (lightness == 0)
                {
                    GPIO_DAT_SET(GPIO08); // 关闭 LED
                }
                else
                {
                    GPIO_DAT_CLR(GPIO08); // 开启 LED
                    // TODO: 可在此处根据 lightness 值调整 PWM 占空比实现调光
                }

                DEBUG("    Lightness=%d", lightness);
            }
            // ========================================
            // Lighting CTL 模型状态处理
            // ========================================
            else if (p_ind->state_id == MM_STATE_LIGHT_CTL_LN)  // Light CTL Lightness
            {
                uint16_t lightness = (uint16_t)p_ind->state;

                if (lightness == 0)
                {
                    GPIO_DAT_SET(GPIO08); // 关闭 LED
                }
                else
                {
                    GPIO_DAT_CLR(GPIO08); // 开启 LED
                }

                DEBUG("    CTL_Lightness=%d", lightness);
            }
            else if (p_ind->state_id == MM_STATE_LIGHT_CTL_TEMP)  // Light CTL Temperature
            {
                uint16_t temp = (uint16_t)p_ind->state;

                DEBUG("    CTL_Temp=%dK", temp);

                // TODO: 可根据色温值调整 LED 色温 (需要可调色温 LED)
            }
            // ========================================
            // Lighting HSL 模型状态处理
            // ========================================
            else if (p_ind->state_id == MM_STATE_LIGHT_HSL_LN)  // Light HSL Lightness
            {
                uint16_t lightness = (uint16_t)p_ind->state;

                if (lightness == 0)
                {
                    GPIO_DAT_SET(GPIO08); // 关闭 LED
                }
                else
                {
                    GPIO_DAT_CLR(GPIO08); // 开启 LED
                }

                DEBUG("    HSL_Lightness=%d", lightness);
            }
            else if (p_ind->state_id == MM_STATE_LIGHT_HSL_HUE)  // Light HSL Hue
            {
                uint16_t hue = (uint16_t)p_ind->state;

                DEBUG("    HSL_Hue=%d", hue);

                // TODO: 可根据色调值调整 LED 颜色 (需要 RGB LED)
            }
            else if (p_ind->state_id == MM_STATE_LIGHT_HSL_SAT)  // Light HSL Saturation
            {
                uint16_t sat = (uint16_t)p_ind->state;

                DEBUG("    HSL_Saturation=%d", sat);

                // TODO: 可根据饱和度值调整 LED 颜色
            }

        } break;

        case MM_CLI_STATE_IND:
        {
            // 客户端模型状态指示：收到其他设备的状态变更通知
            struct mm_cli_state_ind *p_ind = (struct mm_cli_state_ind *)param;

            DEBUG("    CLI_STATE(src:%04X,sid:%d,sv1:%d,sv2:%d,rms:%d)",
                    p_ind->src, p_ind->state_id, p_ind->state_1, p_ind->state_2, p_ind->rem_time_ms);

            // ========================================
            // Generic OnOff 客户端状态指示
            // ========================================
            if (p_ind->state_id == MM_STATE_GEN_ONOFF)
            {
                if (p_ind->state_1)
                {
                    GPIO_DAT_CLR(GPIO09); // LED1 开启
                }
                else
                {
                    GPIO_DAT_SET(GPIO09); // LED1 关闭
                }
            }
            // ========================================
            // Lighting Lightness 客户端状态指示
            // ========================================
            else if (p_ind->state_id == MM_STATE_LIGHT_LN)
            {
                uint16_t lightness = p_ind->state_1;

                if (lightness > 0)
                {
                    GPIO_DAT_CLR(GPIO09); // LED1 开启
                }
                else
                {
                    GPIO_DAT_SET(GPIO09); // LED1 关闭
                }

                DEBUG("    CLI_Lightness=%d", lightness);
            }
            // ========================================
            // Lighting CTL 客户端状态指示
            // ========================================
            else if (p_ind->state_id == MM_STATE_LIGHT_CTL_LN)
            {
                if (p_ind->state_1 > 0)
                {
                    GPIO_DAT_CLR(GPIO09); // LED1 开启
                }
                else
                {
                    GPIO_DAT_SET(GPIO09); // LED1 关闭
                }
            }
            // ========================================
            // Lighting HSL 客户端状态指示
            // ========================================
            else if (p_ind->state_id == MM_STATE_LIGHT_HSL_LN)
            {
                if (p_ind->state_1 > 0)
                {
                    GPIO_DAT_CLR(GPIO09); // LED1 开启
                }
                else
                {
                    GPIO_DAT_SET(GPIO09); // LED1 关闭
                }
            }

        } break;

        default:
        {
            // todo more...
        } break;
    }
}

APP_MSG_HANDLER(mm_req_ind)
{
    DEBUG("mm_req_ind(op:%d)", param->req_code);
}


/**
 ****************************************************************************************
 * @brief SubTask Handler of MESH Message.
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief Mesh 消息子任务处理器
 *
 * Mesh 消息分发中心，根据消息 ID 分发到对应的处理函数：
 * - Mesh 服务消息：状态、故障、Provisioning、配置等
 * - Mesh 模型消息：模型状态、模型请求等
 ****************************************************************************************
 */
APP_SUBTASK_HANDLER(mesh_msg)
{
    switch (msgid)
    {
        /**** Mesh Service Message - Mesh 服务消息 ****/
        case MESH_STATE_IND:
        {
            APP_MSG_FUNCTION(ms_state_ind);
        } break;

        case MESH_FNDH_FAULT_IND:
        {
            APP_MSG_FUNCTION(ms_fndh_fault_ind);
        } break;

        case MESH_FNDH_PERIOD_IND:
        {
            APP_MSG_FUNCTION(ms_fndh_period_ind);
        } break;

        case MESH_PROV_DATA_REQ_IND:
        {
            APP_MSG_FUNCTION(ms_prov_data_req_ind);
        } break;

        #if (BLE_MESH_LPN)
        case MESH_LPN_OFFER_IND:
        {
            APP_MSG_FUNCTION(ms_lpn_offer_ind);
        } break;
        #endif //(BLE_MESH_LPN)

        case MESH_CONF_LOAD_IND:
        {
            APP_MSG_FUNCTION(ms_conf_load_ind);
        } break;

        case MESH_CONF_UPDATE_IND:
        {
            APP_MSG_FUNCTION(ms_conf_update_ind);
        } break;

        /**** Mesh Model Message - Mesh 模型消息   ****/
        case MESH_MDL_IND:
        {
            APP_MSG_FUNCTION(mm_ind);
        } break;

        case MESH_MDL_REQ_IND:
        {
            APP_MSG_FUNCTION(mm_req_ind);
        } break;

        default:
        {
            uint16_t length = ke_param2msg(param)->param_len;

            DEBUG("Unknow MsgId:0x%X, MsgLen:%d", msgid, length);
            debugHex((uint8_t *)param, length);
        } break;
    }

    return (MSG_STATUS_FREE);
}

#endif //(PRF_MESH)
