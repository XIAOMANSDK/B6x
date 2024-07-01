#ifndef _AT_H_
#define _AT_H_

#include "b6x.h"
#include "bledef.h"
#include "drvs.h"
#include "gap.h"
#include "app.h"
#include "proto.h"



#define MAX_UUID_LEN               0x10//

#define ADDR_OFFSET_CFG            0x1000//

typedef enum
{
    All_FACTORY_REST = 0,           //全部恢复出厂设置
    PAIR_FACTORY_RESET = 1,         //配对信息恢复出厂设置-相当于清除配对信息与从机信息
}PARA_SET_FACTORY;


typedef struct 
{
    uint8_t dev_pair_mode;
    uint8_t auth;
    uint8_t pair_len;
    uint8_t pair_data[6]; 
}pair_info_t;

typedef struct 
{
    uint8_t len;
    uint8_t name[24]; 
}name_info_t;

typedef struct 
{
    uint8_t lenADV;
    uint8_t lenSCAN;
    uint8_t adv[31];
    uint8_t scan[31];    
}adv_info_t;

typedef struct 
{
    uint16_t intervalMIN;
    uint16_t intervalMAX;
    uint16_t latency;
    uint16_t timeout;    
}conn_info_t;

typedef struct 
{
    uint8_t level;
    uint8_t iox; 
}gpio_info_t;

typedef struct 
{
    uint8_t iox;
    uint8_t level;
    uint32_t delay_us;
}wake_info_t;

enum adv_data_idx
{
    adv_type_idx = 0,
    adv_ID_idx   = 1,
    adv_char_idx = 3,    
    adv_PIN_idx  = 5,       
    adv_name_idx = 22,   
};

enum b_tx_power
{
    TX_4DB_P,   // 4dBm
    TX_0DB_P,   // 0dBm
    TX_4DB_M,   // -4dBm
    TX_8DB_M,   // -8dBm
    TX_20DB_M,  // -20dBm    
};

enum adv_enble
{
    ADV_DISENBLE,    // 关闭(默认)
    ADV_ENBLE,       // 打开
};

typedef union 
{
    struct
    {
        uint8_t  BT_DIS   :  1;  // bit0 3.0 可发现
        uint8_t  BT_CONE  :  1;  // bit1 3.0 可连接
        uint8_t  BLE_DIS  :  1;  // bit2 4.0 可发现
        uint8_t  RSV_N    :  1;  // bit3
        uint8_t  BT_COND  :  1;  // bit4 BT3.0 已连接
        uint8_t  BLE_COND :  1;  // bit5 BLE 已连接       
    };
    uint8_t value;
}BLE_STAUTS_TYPEDEF;

typedef struct 
{
    BLE_STAUTS_TYPEDEF  stauts;
    name_info_t name_info;  
    adv_info_t adv_info; 
    
    uint8_t tx_power;
    
    uint32_t baudrate;                 //波特率
    uint16_t vertion;                  //固件版本
    uint8_t uuid_len; 
    uint8_t uuids[MAX_UUID_LEN];
    uint8_t uuidn[MAX_UUID_LEN];
    uint8_t uuidw[MAX_UUID_LEN];

    bd_addr_t addrL;
    wake_info_t  wake_info;
    pair_info_t  pair_info;
    conn_info_t  conn_info;
    uint16_t advr;
}SYS_CONFIG;

extern SYS_CONFIG sysCfg;

void syscfgInit(void);

#endif

