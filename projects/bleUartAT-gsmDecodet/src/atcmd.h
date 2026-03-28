#ifndef _ATCMD_H_
#define _ATCMD_H_

#include "b6x.h"
#include "bledef.h"
#include "drvs.h"
#include "gap.h"
#include "app.h"
/*
 * DEFINES
 ****************************************************************************************
 */
#define MAX_UUID_LEN                       0x10//
#define MAX_ADV_DATA_LEN                   0x20//
#define SYS_CONFIG_OFFSET 0x1000

// AT Default Congif
#define AT_DFT_UART_BAUD    (115200)
#define AT_DFT_ADV_DATA_LEN (11)
#define AT_DFT_ADV_INTV     (0x20)
#define AT_DFT_RSSI         (0)

// �������û���������Ϣ��ӻ���Ϣ
typedef enum
{
    All_FACTORY_REST = 0,           //ȫ���ָ���������
    PAIR_FACTORY_RESET = 1,         //�����Ϣ�ָ���������-�൱����������Ϣ��ӻ���Ϣ
}PARA_SET_FACTORY;

// AT ָ�������
enum err_code
{
  ERR_HARD_WARE,                // �豸Ӳ��������
  ERR_NO_CONNECT,               // û������        
  ERR_OPERATION,                // ����������
  ERR_INVALID,                  // ����������Ч
  ERR_TIMEOUT,                  // ��ʱ
  ERR_MEMORY,                   // �ڴ�ʧ��
  ERR_PROTOCOL,                 // Э��ʧ��        
};

// AT ָ���ַ�����
enum cmd_str
{
    CMD_NULL,                 // 
    CMD_ECHO,                 // 
    CMD_ALL,                  // 
    CMD_MAC_R,                // 
    CMD_VER_R,                // 
    CMD_NAME_R,               // 
    CMD_NAME_S,               //
    CMD_BAUD_R,
    CMD_BAUD_S,
    CMD_DISCON_S,
    CMD_SCAN_S,
    CMD_CON_MAC_R,
    CMD_CON_MAC_S,
    CMD_UUIDS_R,
    CMD_UUIDS_S,
    CMD_UUIDN_R,
    CMD_UUIDN_S,
    CMD_UUIDW_R,
    CMD_UUIDW_S,
    CMD_AINTVL_R,
    CMD_AINTVL_S,
    CMD_AMDATA_R,
    CMD_AMDATA_S,
    CMD_RENEW_S,
    CMD_RESET_S,
    CMD_HELP,
    // GSM Audio Commands
    CMD_GSMPLAY,
    CMD_GSMSTOP,
    CMD_GSMSTATUS,

    CMD_CODE_MAX,
};

typedef struct at_cmd_format
{
    uint8_t str_len_min;
    uint8_t str_len_max;
    const char *str;    
}AT_CMD_FORMAT_T;

///Information about Connected device
struct connected_result
{
    struct gap_bdaddr paddr;
};

typedef struct 
{
    uint8_t name_len;     // �豸���ƣ�� 11 λ���ֻ���ĸ�����л��ߺ��»��ߣ��������������ַ�
    uint8_t rssi;                              //  RSSI �ź�ֵ 
                                  
    uint16_t adv_intv_time;                 // �㲥��� 
    
    uint32_t baudrate;                 //������

    uint8_t uuids[MAX_UUID_LEN];
    uint8_t uuidn[MAX_UUID_LEN];
    uint8_t uuidw[MAX_UUID_LEN];
    
    uint8_t uuid_len;     
    uint8_t name[DEV_NAME_MAX_LEN]; 
    
    uint8_t adv_data_len;
    uint8_t adv_data[MAX_ADV_DATA_LEN]; 
    
    uint8_t mac_addr[GAP_BD_ADDR_LEN];            //����mac��ַ ���12λ �ַ���ʾ

    uint8_t connect_mac_addr[GAP_BD_ADDR_LEN];    //�Զ�����mac��ַ      

}SYS_CONFIG __attribute__((aligned(4)));

extern SYS_CONFIG sys_config;
extern bool scan_time_out;
extern bool disconnect_all;
extern struct connected_result connected_list[];

void atConfigFlashRead(void);
void atSetBleDefault(PARA_SET_FACTORY flag);
bool atCmdHandle(const uint8_t *buff, uint8_t buff_len);  
bool atProc(const uint8_t *buff, uint8_t buff_len);
void atBleTx(const uint8_t *buff, uint8_t buff_len);

#endif

