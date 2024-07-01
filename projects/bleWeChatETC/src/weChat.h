// proto version: 1.0.4

#ifndef _WECHAT_H_
#define _WECHAT_H_

#include "b6x.h"
#include "bledef.h"
#include "drvs.h"
#include "gap.h"
#include "app.h"

//大端在前
#define __SWP16_L(A) ((uint16_t)(A) & 0x00FF)                  
#define __SWP16_H(A) (((uint16_t)(A)  >> 8) & 0x00FF)
                   
#define FIX_HEAD_MAGIC 0xFE
#define FIX_HEAD_VER   0x01


#define BLE_REQ_AUTH_LEN   0x1A
#define BLE_REQ_INIT_LEN   0x0A
#define BLE_REQ_DATA_LEN   0x0F
#define BLE_DATA_LEN_MAX   0xFF

enum EmCmdId
{
    ECI_none = 0,

    // req： 蓝牙设备 -> 微信/厂商服务器
    ECI_req_auth = 0x2711/*10001*/, // 登录
    ECI_req_sendData = 0x2712/*10002*/, // 蓝牙设备发送数据给微信或厂商
    ECI_req_init = 0x2713/*10003*/, // 初始化

    // resp：微信/厂商服务器 -> 蓝牙设备
    ECI_resp_auth = 0x4E21/*20001*/,
    ECI_resp_sendData = 0x4E22/*20001*/,
    ECI_resp_init = 0x4E23/*20001*/,

    // push：微信/厂商服务器 -> 蓝牙设备
    ECI_push_recvData = 0x7531/*30001*/, // 微信或厂商发送数据给蓝牙设备
    ECI_push_switchView = 0x7532/*30001*/, // 进入/退出界面
    ECI_push_switchBackgroud = 0x7533/*30001*/, // 切换后台

    ECI_err_decode = 0x752F/*29999*/, // 解密失败的错误码。注意：这不是 cmdid。为节省固定包头大小，这种特殊的错误码放在包头的 cmdid 字段。
};

enum EmErrorCode
{
    EEC_system = -1, // 通用的错误
    EEC_needAuth = -2, // 设备未登录
    EEC_sessionTimeout = -3, // session 超时，需要重新登录
    EEC_decode = -4, // proto 解码失败
    EEC_deviceIsBlock = -5, // 设备出现异常，导致被微信临时性禁止登录
    EEC_serviceUnAvalibleInBackground = -6, // ios 处于后台模式，无法正常服务
    EEC_deviceProtoVersionNeedUpdate = -7, // 设备的 proto 版本过老，需要更新
    EEC_phoneProtoVersionNeedUpdate = -8, // 微信客户端的 proto 版本过老，需要更新
    EEC_maxReqInQueue = -9, // 设备发送了多个请求，并且没有收到回包。微信客户端请求队列拥塞。
    EEC_userExitWxAccount = -10, // 用户退出微信帐号。
};

// req, resp ========================================
enum EmAuthMethod
{
    EAM_md5 = 1, // 设备通过 Md5DeviceTypeAndDeviceId，来通过微信 app 的认证。1. 如果是用 aes 加密，注意设置 AesSign 有值。 2. 如果是没有加密，注意设置 AesSign 为空或者长度为零。
    EAM_macNoEncrypt = 2, // 设备通过 mac 地址字段，且没有加密，来通过微信 app 的认证。
};

// 设备发送数据给微信或厂商 ----------------------------
// 设备数据类型
enum EmDeviceDataType {
    EDDT_manufatureSvr = 0, // 厂商自定义数据
    EDDT_wxWristBand = 1, // 微信公众平台手环数据
    EDDT_wxDeviceHtmlChatView = 10001, // 微信客户端设备 html5 会话界面数据
};

// 微信AirSync协议固定包头
typedef struct 
{
    uint8_t  bMagicNumber;
    uint8_t  bVer;
    uint8_t  nLength[2]; //大端在前
    uint8_t  nCmdId[2]; //大端在前
    uint8_t  nSeq[2]; //大端在前
}BpFixHead;

extern uint16_t ble_head_nSeq;

/// Protocol Packet
typedef struct pt_pkt_wc
{
    /* head(4B): 1-sync; 1-cmd; 2-len */
    BpFixHead head;

    /* payl(nB): A set of variable value */
    uint8_t   payl[];
} pkt_wc;

void ble_parser_rsp(struct pt_pkt_wc *pkt, uint16_t status);
void ble_req_auth(void);


//BLE UART BUFF
/// Structures
#define RXD_BUFF_SIZE        (0x100) 

struct rxd_buffer
{
    volatile uint16_t head;
    volatile uint16_t tail;
    uint8_t  data[RXD_BUFF_SIZE] __attribute__((aligned(4)));
};
extern struct rxd_buffer ble_rxd;

#endif

