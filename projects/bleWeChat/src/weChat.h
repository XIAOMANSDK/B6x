/**
 ****************************************************************************************
 *
 * @file weChat.h
 *
 * @brief WeChat AirSync BLE Protocol Definitions
 *
 ****************************************************************************************
 */

#ifndef _WECHAT_H_
#define _WECHAT_H_

#include "b6x.h"
#include "bledef.h"
#include "drvs.h"
#include "gap.h"
#include "app.h"

/// Little-endian byte swap macros
#define __SWP16_L(A) ((uint16_t)(A) & 0x00FF)
#define __SWP16_H(A) (((uint16_t)(A)  >> 8) & 0x00FF)

#define FIX_HEAD_MAGIC 0xFE
#define FIX_HEAD_VER   0x01

/// Protocol packet lengths
#define BLE_REQ_AUTH_LEN   0x1A  ///< Auth request length (26 bytes)
#define BLE_REQ_INIT_LEN   0x0A  ///< Init request length (10 bytes)
#define BLE_REQ_DATA_LEN   0x0F  ///< Data request length (15 bytes)
#define BLE_DATA_LEN_MAX   0xFF  ///< Max data length (255 bytes)

/// WeChat AirSync Command IDs
enum EmCmdId
{
    ECI_none = 0,

    // req: device -> WeChat/server
    ECI_req_auth     = 0x2711,  ///< 10001 - Auth login
    ECI_req_sendData = 0x2712,  ///< 10002 - Send data to WeChat
    ECI_req_init     = 0x2713,  ///< 10003 - Initialize

    // resp: WeChat/server -> device
    ECI_resp_auth     = 0x4E21,  ///< 20001
    ECI_resp_sendData = 0x4E22,  ///< 20002
    ECI_resp_init     = 0x4E23,  ///< 20003

    // push: WeChat/server -> device
    ECI_push_recvData          = 0x7531,  ///< 30001 - Receive data from WeChat
    ECI_push_switchView        = 0x7532,  ///< 30002 - Enter/exit page
    ECI_push_switchBackgroud   = 0x7533,  ///< 30003 - Switch background

    ECI_err_decode = 0x752F,  ///< 29999 - Decode error
};

/// WeChat AirSync Error Codes
enum EmErrorCode
{
    EEC_system                               = -1,   ///< General error
    EEC_needAuth                             = -2,   ///< Device not logged in
    EEC_sessionTimeout                       = -3,   ///< Session timeout, re-login required
    EEC_decode                               = -4,   ///< Proto decode failure
    EEC_deviceIsBlock                        = -5,   ///< Device blocked from login
    EEC_serviceUnAvalibleInBackground        = -6,   ///< iOS background mode unavailable
    EEC_deviceProtoVersionNeedUpdate         = -7,   ///< Device proto version too old
    EEC_phoneProtoVersionNeedUpdate          = -8,   ///< Phone proto version too old
    EEC_maxReqInQueue                        = -9,   ///< Too many queued requests
    EEC_userExitWxAccount                    = -10,  ///< User exited WeChat account
};

/// WeChat Auth Methods
enum EmAuthMethod
{
    EAM_md5          = 1,  ///< MD5 auth with optional AES encryption
    EAM_macNoEncrypt = 2,  ///< MAC address auth without encryption
};

/// Device data types sent to WeChat
enum EmDeviceDataType {
    EDDT_manufatureSvr          = 0,     ///< Manufacturer custom server
    EDDT_wxWristBand            = 1,     ///< WeChat wristband
    EDDT_wxDeviceHtmlChatView   = 10001, ///< WeChat HTML5 chat view
};

/// WeChat AirSync Protocol Fixed Header (little-endian fields)
typedef struct
{
    uint8_t  bMagicNumber;
    uint8_t  bVer;
    uint8_t  nLength[2]; ///< Little-endian
    uint8_t  nCmdId[2];  ///< Little-endian
    uint8_t  nSeq[2];    ///< Little-endian
} BpFixHead;

extern uint16_t ble_head_nSeq;

/// Protocol Packet
typedef struct pt_pkt
{
    /* head(8B): 1-sync; 1-ver; 2-len; 2-cmd; 2-seq */
    BpFixHead head;

    /* payl(nB): variable payload */
    uint8_t   payl[];
} pkt_t;

void ble_parser_rsp(struct pt_pkt *pkt, uint16_t status);
void ble_req_auth(void);

/// BLE UART Buffer
#define RXD_BUFF_SIZE        (0x100)  ///< 256 bytes

/// UART ring buffer structure
struct rxd_buffer
{
    volatile uint16_t head;
    volatile uint16_t tail;
    uint8_t  data[RXD_BUFF_SIZE] __attribute__((aligned(4)));
};

extern struct rxd_buffer uart1_rxd;

#endif /* _WECHAT_H_ */
