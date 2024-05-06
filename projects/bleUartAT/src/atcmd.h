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

// 出厂设置或清除配对信息与从机信息
typedef enum
{
    All_FACTORY_REST = 0,           //全部恢复出厂设置
    PAIR_FACTORY_RESET = 1,         //配对信息恢复出厂设置-相当于清除配对信息与从机信息
}PARA_SET_FACTORY;

// AT 指令错误码
enum err_code
{
  ERR_HARD_WARE,                // 设备硬件出问题
  ERR_NO_CONNECT,               // 没有连接        
  ERR_OPERATION,                // 操作不允许
  ERR_INVALID,                  // 操作参数无效
  ERR_TIMEOUT,                  // 超时
  ERR_MEMORY,                   // 内存失败
  ERR_PROTOCOL,                 // 协议失败        
};

// AT 指令字符串集
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
    uint8_t name_len;     // 设备名称，最长 11 位数字或字母，含中划线和下划线，不建议用其它字符
    uint8_t rssi;                              //  RSSI 信号值 
                                  
    uint16_t adv_intv_time;                 // 广播间隔 
    
    uint32_t baudrate;                 //波特率

    uint8_t uuids[MAX_UUID_LEN];
    uint8_t uuidn[MAX_UUID_LEN];
    uint8_t uuidw[MAX_UUID_LEN];
    
    uint8_t uuid_len;     
    uint8_t name[DEV_NAME_MAX_LEN]; 
    
    uint8_t adv_data_len;
    uint8_t adv_data[MAX_ADV_DATA_LEN]; 
    
    uint8_t mac_addr[GAP_BD_ADDR_LEN];            //本机mac地址 最大12位 字符表示

    uint8_t connect_mac_addr[GAP_BD_ADDR_LEN];    //自动重连mac地址      

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

