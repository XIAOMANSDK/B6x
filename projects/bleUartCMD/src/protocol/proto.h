#ifndef _PROTO_H_
#define _PROTO_H_

#include <stdint.h>
#include <stdbool.h>


/***************************************************************************
 *  Proto Macro API
 ***************************************************************************/

/// Protocol Defines

enum PT_TYPE
{
    PT_TYPE_CMD          = 0x01,
    PT_TYPE_RSP          = 0x02,
};

enum PT_ERR_CODE
{
    PT_OK                = 0x00,

    // proto sch error A0~A4
    PT_ERROR             = 0xA0,
    PT_ERR_CRC           = 0xA0,
    PT_ERR_LEN           = 0xA1,
    PT_ERR_CODE          = 0xA2,
    PT_ERR_HEAD          = 0xA3,
    PT_ERR_TIMEOUT       = 0xA4,
    // proto app error A6~AF
    PT_ERR_VERIFY        = 0xA6,
    PT_ERR_STATUS        = 0xA7,
};

/// Packet Defines @see pt_pkt
#define PKT_HDR_POS        0
#define PKT_HDR_SIZE       3
#define HDR_SYNC_POS       (PKT_HDR_POS)
#define HDR_SYNC_SIZE      (0x01)
#define HDR_CMD_POS        (PKT_HDR_POS + HDR_SYNC_SIZE)
#define HDR_CMD_SIZE       (0x01)
#define HDR_LEN_POS        (PKT_HDR_POS + HDR_SYNC_SIZE + HDR_CMD_SIZE)
#define HDR_LEN_SIZE       (0x01)

#define PKT_PAYL_POS       3
#define PAYL_DAT_MAX_SIZE  (0xFF)
#define PKT_PAYL_MAX_SIZE  (PAYL_DAT_MAX_SIZE)

#define PKT_MAX_SIZE       (((PKT_HDR_SIZE + PKT_PAYL_MAX_SIZE)+3)/4)*4

enum PKT_FMT
{
    PKT_FIXLEN           = 0,
    PKT_VARLEN           = 1,

    PKT_LEN_MSK          = 0x01,
};

/// Protocol Packet
typedef struct pt_pkt
{
    /* head(4B): 1-sync; 1-cmd; 1-len */
    uint8_t   type;
    uint8_t   code;
    uint8_t   len;

    /* payl(nB): A set of variable value */
    uint8_t   payl[];
} pkt_t;

typedef void(*parse_fnct)(struct pt_pkt *pkt, uint8_t status);

/// PT Interface
void proto_init(parse_fnct parse);

void proto_schedule(void);


/***************************************************************************
 *  Proto Command & Response Macros
 ***************************************************************************/

enum PT_CMD_CODE
{
    PT_CMD_SET_BLE_ADDR             = 0x01, // ���� BLE ��ַ
    PT_CMD_SET_VISIBILITY           = 0x02, // ���ÿɷ��ֺ͹㲥
    PT_CMD_SET_BLE_NAME             = 0x04, // ���� BLE ����   
    PT_CMD_SEND_BLE_DATA            = 0x09, // ���� BLE ����
    
    PT_CMD_STATUS_REQUEST           = 0x0B, // ��������״̬
    PT_CMD_SET_UART_FLOW            = 0x0E, // ���� UART ����
    PT_CMD_SET_UART_BAUD            = 0x0F, // ���� UART ������
    PT_CMD_VERSION_REQUEST          = 0x10, // ��ѯģ��̼��汾
    PT_CMD_BLE_DISCONNECT           = 0x12, // �Ͽ� BLE ����
    
    PT_CMD_SET_NVRAM                = 0x26, // �·� NV ����
    PT_CMD_CONFIRM_GKEY             = 0x28, // Numeric Comparison ��Է�ʽ�ж���Կ�ıȽ�
    PT_CMD_SET_CREDIT_GIVEN         = 0x29, // ���� Spp ����
    PT_CMD_SET_ADV_DATA             = 0x2A, // ���� ADV ����
    PT_CMD_POWER_REQ                = 0x2B, // ��ѯģ���Դ��ѹ
    PT_CMD_POWER_SET                = 0x2C, // ��ȡ��Դ��ѹ���ܿ���
    
    PT_CMD_PASSKEY_ENTRY            = 0x30, // ���� Passkey
    PT_CMD_SET_GPIO                 = 0x31, // ��ʼ�� gpio
    PT_CMD_READ_GPIO                = 0x32, // ��ȡ gpio ״̬
    PT_CMD_LE_SET_PAIRING           = 0x33, // �������ģʽ
    PT_CMD_LE_SET_ADV_DATA          = 0x34, // ���� adv ����
    PT_CMD_LE_SET_SCAN_DATA         = 0x35, // ���� scan ����
    PT_CMD_LE_SEND_CONN_UPDATE_REQ  = 0x36, // �������Ӳ���
    PT_CMD_LE_SET_ADV_PARM          = 0x37, // ���ù㲥����
    PT_CMD_LE_START_PAIRING         = 0x38, // ��ʼ���
    
    PT_CMD_SET_WAKE_GPIO            = 0x40, // ���û��� IO
    PT_CMD_SET_TX_POWER             = 0x42, // ���÷��书��
    PT_CMD_LE_CONFIRM_GKEY          = 0x48, // Ble Numeric Comparison ��Է�ʽ�ж���Կ�ıȽ�
    PT_CMD_REJECT_JUSTWORK          = 0x49, // �ܾ� justwork ��Է�ʽ(pci ��֤ʱ��ʹ��)
    
    PT_CMD_RESET_CHIP_REQ           = 0x51, // ��λоƬ
    
    PT_CMD_LE_SET_FIXED_PASSKEY     = 0x61, // ���ù̶��� passkey
    
    PT_CMD_DELETE_CUSTOMIZE_SERVICE = 0x76, // ɾ�� BLE ��ϵͳ��������
    PT_CMD_ADD_SERVICE_UUID         = 0x77, // ���� BLE �Զ������
    PT_CMD_ADD_CHARACTERISTIC_UUID  = 0x78, // ���� BLE �Զ�������
    
    PT_TEST_CMD_CLOSE_LPM           = 0xFF, // �ر� LPM
};


//EVENT RSP
enum PT_RSP_CODE
{
    PT_RSP_SPP_CONN_REP           = 0x00, // BT3.0 ���ӽ���
    PT_RSP_LE_CONN_REP            = 0x02, // BLE ���ӽ���
    PT_RSP_SPP_DIS_REP            = 0x03, // BT3.0 ���ӶϿ�
    PT_RSP_LE_DIS_REP             = 0x05, // BLE ���ӶϿ�
    
    PT_RSP_CMD_RES                = 0x06, // ���������
    PT_RSP_SPP_DATA_REP           = 0x07, // ���յ� BT3.0 ���ݣ�SPP��   
    PT_RSP_LE_DATA_REP            = 0x08, // ���յ� BLE ����   
    PT_RSP_STANDBY_REP            = 0x09, // ģ��׼����
    PT_RSP_STATUS_RES             = 0x0A, // ״̬�ظ�
    
    PT_RSP_NVRAM_REP              = 0x0D, // �ϴ� NVRAM ����
//    PT_RSP_INVALID_PACKET         = 0x0F, // HCI ����ʽ����
    PT_RSP_GKEY                   = 0x0E, // ���� Numeric Comparison ��Է�ʽ�в�������Կ
    PT_RSP_INVALID_PACKET         = 0x0F, // ֪ͨ MCU �����İ���ʽ����
    
    PT_RSP_GET_PASSKEY            = 0x10, // ���� passkey ��ֵ�� MCU
    PT_RSP_LE_TK                  = 0x11, // BLE PASSKEY ��Է�ʽ��֪ͨMCU ������Կ   
    PT_RSP_LE_PAIRING_STATE       = 0x14, // ֪ͨ MCU Ble �����״̬
    PT_RSP_LE_ENCRYPTION_STATE    = 0x15, // ֪ͨ MCU ��ǰ����״̬
    
    PT_RSP_LE_GKEY                = 0x1d, // ���� Numeric Comparison ��Է�ʽ�в�������Կ

    PT_RSP_UUID_HANDLE            = 0x29, // ֪ͨ MCU �����õ� uuid ��Ӧ��handle     
};


enum PAYL_LEN
{
    PLEN_CMD_SET_BLE_ADDR         = 0x06,  // PKT_FIXLEN
    PLEN_CMD_SET_VISIBILITY       = 0x01,  // PKT_FIXLEN
    PLEN_CMD_SET_BLE_NAME         = 0x18,  // PKT_VARLEN
    PLEN_CMD_STATUS_REQUEST       = 0x00,  // PKT_FIXLEN
    
    PLEN_RSP_OK                   = 0x00,  // 0
    PLEN_RSP_CODE                 = 0x00,  // 0
    PLEN_RSP_VERTION              = 0x02,  // 0  
    PLEN_RSP_GPIO                 = 0x02,  // 0  
    
    PLEN_RSP_STATUS               = 0x01,  // 1status       
                                  
    PLEN_RSP_CMD_RES              = 0xFF,
                                  
    PLEN_RSP_LE_DATA_REP          = 0xFF, //2handle+data
                                  
    PLEN_RSP_NVRAM_REP            = 0xAA, //34*5  
                                  
    PLEN_RSP_GKEY                 = 0x04, //5
    PLEN_RSP_LE_TK                = 0x04, //5
    PLEN_RSP_LE_GKEY              = 0x04, //5
    PLEN_RSP_GET_PASSKEY          = 0x06, //5
    PLEN_RSP_LE_PAIRING_STATE     = 0x02, //5
    PLEN_RSP_LE_ENCRYPTION_STATE  = 0x01, //5
    PLEN_RSP_UUID_HANDLE          = 0x02, //5
    
    PLEN_CMD_DFT                  = 0xFF, //0x103, // 1Lbidx+2Pkgidx+(1~PAYL_DAT_MAX_SIZE)data
    PLEN_RSP_DFT                  = 0x01,  // 1status 
};

/***************************************************************************
 *  Proto Command & Response Structs
 ***************************************************************************/




// HCI EVENT RSP
struct pt_rsp_status
{
    uint8_t status;
};

struct pt_rsp_cmd_res
{
    uint8_t opcode;
    uint8_t status;
    uint8_t  data[];
};

struct pt_rsp_le_data_rep
{
    uint8_t handle[2];
    uint8_t data[];
};

struct pt_rsp_le_pairing_state
{
    uint8_t state[2];
};

struct pt_rsp_le_encryption_state
{
    uint8_t state;
};

struct pt_rsp_uuid_handle
{
    uint8_t handle[2];
};

/***************************************************************************
 *  Proto Command & Response Function
 ***************************************************************************/

#define PKT_ALLOC(payl_len)  uint8_t buff[PKT_HDR_SIZE + payl_len]; pkt_t *pkt = (pkt_t *)buff
#define PKT_PARAM(p_struct)  p_struct *param = (p_struct *)pkt->payl

/// Command Send via UART2
extern uint8_t  pt_code;
extern uint32_t pt_time;
void pt_send_cmd(pkt_t *pkt);

/// Response Send via UART1
void pt_rsp_code(uint8_t rsp);

#define pt_rsp_spp_conn_rep()   pt_rsp_code(PT_RSP_SPP_CONN_REP)
#define pt_rsp_le_conn_rep()    pt_rsp_code(PT_RSP_LE_CONN_REP)
#define pt_rsp_spp_dis_rep()    pt_rsp_code(PT_RSP_SPP_DIS_REP)
#define pt_rsp_le_dis_rep()     pt_rsp_code(PT_RSP_LE_DIS_REP)
#define pt_rsp_standby_rep()    pt_rsp_code(PT_RSP_STANDBY_REP)
#define pt_rsp_invalid_packet() pt_rsp_code(PT_RSP_INVALID_PACKET)

#define pt_rsp_gkey(payl) pt_rsp_key(PT_RSP_GKEY, PLEN_RSP_GKEY, payl)
#define pt_rsp_le_tk(payl) pt_rsp_key(PT_RSP_LE_TK, PLEN_RSP_LE_TK, payl)
#define pt_rsp_le_gkey(payl) pt_rsp_key(PT_RSP_LE_GKEY, PLEN_RSP_LE_GKEY, payl)
#define pt_rsp_get_passkey(payl) pt_rsp_key(PT_RSP_LE_GKEY, PLEN_RSP_GET_PASSKEY, payl)

void pt_rsp_cmd_res(uint8_t opcode, uint8_t status, uint8_t len, const void *payl);
void pt_rsp_le_data_rep(uint16_t handle, uint8_t len, const void *payl);
void pt_rsp_status_res(uint8_t status);
void pt_rsp_nvram_rep(const void *payl);
void pt_rsp_le_pairing_state(uint16_t state);
void pt_rsp_le_encryption_state(uint8_t state);
void pt_rsp_uuid_handle(uint16_t handle);

#define pt_rsp_ok(code) pt_rsp_cmd_res(code, PT_OK, PLEN_RSP_OK, NULL);

void uart_proc(struct pt_pkt *pkt, uint8_t status);
#endif // _PROTO_H_
