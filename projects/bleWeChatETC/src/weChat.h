// proto version: 1.0.4

#ifndef _WECHAT_H_
#define _WECHAT_H_

#include "b6x.h"
#include "bledef.h"
#include "drvs.h"
#include "gap.h"
#include "app.h"

//�����ǰ
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

    // req�� �����豸 -> ΢��/���̷�����
    ECI_req_auth = 0x2711/*10001*/, // ��¼
    ECI_req_sendData = 0x2712/*10002*/, // �����豸�������ݸ�΢�Ż���
    ECI_req_init = 0x2713/*10003*/, // ��ʼ��

    // resp��΢��/���̷����� -> �����豸
    ECI_resp_auth = 0x4E21/*20001*/,
    ECI_resp_sendData = 0x4E22/*20001*/,
    ECI_resp_init = 0x4E23/*20001*/,

    // push��΢��/���̷����� -> �����豸
    ECI_push_recvData = 0x7531/*30001*/, // ΢�Ż��̷������ݸ������豸
    ECI_push_switchView = 0x7532/*30001*/, // ����/�˳�����
    ECI_push_switchBackgroud = 0x7533/*30001*/, // �л���̨

    ECI_err_decode = 0x752F/*29999*/, // ����ʧ�ܵĴ����롣ע�⣺�ⲻ�� cmdid��Ϊ��ʡ�̶���ͷ��С����������Ĵ�������ڰ�ͷ�� cmdid �ֶΡ�
};

enum EmErrorCode
{
    EEC_system = -1, // ͨ�õĴ���
    EEC_needAuth = -2, // �豸δ��¼
    EEC_sessionTimeout = -3, // session ��ʱ����Ҫ���µ�¼
    EEC_decode = -4, // proto ����ʧ��
    EEC_deviceIsBlock = -5, // �豸�����쳣�����±�΢����ʱ�Խ�ֹ��¼
    EEC_serviceUnAvalibleInBackground = -6, // ios ���ں�̨ģʽ���޷���������
    EEC_deviceProtoVersionNeedUpdate = -7, // �豸�� proto �汾���ϣ���Ҫ����
    EEC_phoneProtoVersionNeedUpdate = -8, // ΢�ſͻ��˵� proto �汾���ϣ���Ҫ����
    EEC_maxReqInQueue = -9, // �豸�����˶�����󣬲���û���յ��ذ���΢�ſͻ����������ӵ����
    EEC_userExitWxAccount = -10, // �û��˳�΢���ʺš�
};

// req, resp ========================================
enum EmAuthMethod
{
    EAM_md5 = 1, // �豸ͨ�� Md5DeviceTypeAndDeviceId����ͨ��΢�� app ����֤��1. ������� aes ���ܣ�ע������ AesSign ��ֵ�� 2. �����û�м��ܣ�ע������ AesSign Ϊ�ջ��߳���Ϊ�㡣
    EAM_macNoEncrypt = 2, // �豸ͨ�� mac ��ַ�ֶΣ���û�м��ܣ���ͨ��΢�� app ����֤��
};

// �豸�������ݸ�΢�Ż��� ----------------------------
// �豸��������
enum EmDeviceDataType {
    EDDT_manufatureSvr = 0, // �����Զ�������
    EDDT_wxWristBand = 1, // ΢�Ź���ƽ̨�ֻ�����
    EDDT_wxDeviceHtmlChatView = 10001, // ΢�ſͻ����豸 html5 �Ự��������
};

// ΢��AirSyncЭ��̶���ͷ
typedef struct 
{
    uint8_t  bMagicNumber;
    uint8_t  bVer;
    uint8_t  nLength[2]; //�����ǰ
    uint8_t  nCmdId[2]; //�����ǰ
    uint8_t  nSeq[2]; //�����ǰ
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

