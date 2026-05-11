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

// Factory reset parameter settings: user info, slave info
typedef enum
{
    All_FACTORY_REST = 0,           // Full factory reset
    PAIR_FACTORY_RESET = 1,         // Pairing info factory reset
}PARA_SET_FACTORY;

// AT command error codes
enum err_code
{
  ERR_HARD_WARE,                // Device hardware error
  ERR_NO_CONNECT,               // Not connected
  ERR_OPERATION,                // Operation error
  ERR_INVALID,                  // Invalid parameter
  ERR_TIMEOUT,                  // Timeout
  ERR_MEMORY,                   // Memory failure
  ERR_PROTOCOL,                 // Protocol error
};

// AT command string enumeration
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
    uint8_t name_len;     // Device name length (max 11 chars: letters/digits/dash/underscore)
    uint8_t rssi;                              // RSSI signal value

    uint16_t adv_intv_time;                 // Advertising interval

    uint32_t baudrate;                 // UART baud rate

    uint8_t uuids[MAX_UUID_LEN];
    uint8_t uuidn[MAX_UUID_LEN];
    uint8_t uuidw[MAX_UUID_LEN];

    uint8_t uuid_len;
    uint8_t name[DEV_NAME_MAX_LEN];

    uint8_t adv_data_len;
    uint8_t adv_data[MAX_ADV_DATA_LEN];

    uint8_t mac_addr[GAP_BD_ADDR_LEN];            // Local MAC address

    uint8_t connect_mac_addr[GAP_BD_ADDR_LEN];    // Remote peer MAC address

}SYS_CONFIG __attribute__((aligned(4)));

extern SYS_CONFIG sys_config;
extern bool scan_time_out;
extern bool disconnect_all;
extern struct connected_result connected_list[];

void atConfigFlashRead(void);
void atSetBleDefault(PARA_SET_FACTORY flag);
bool atProc(const uint8_t *buff, uint8_t buff_len);
void atBleTx(const uint8_t *buff, uint8_t buff_len);

#endif
