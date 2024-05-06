
#ifndef MPBLEDEMO2
#define MPBLEDEMO2

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "nrf_gpio.h"
#include "epb_MmBp.h"
#include "ble_wechat_util.h"
#include "ble_wechat_service.h"
#include "app_error.h"


#define CMD_NULL 0
#define CMD_AUTH 1
#define CMD_INIT 2
#define CMD_SENDDAT 3

#define DEVICE_TYPE "gh_1bafe245c2cb"
#define DEVICE_ID "WeChatBluetoothDevice"


#define PROTO_VERSION 0x010004
#define AUTH_PROTO 1

#define MAC_ADDRESS_LENGTH 6

//#define EAM_md5AndNoEnrypt 1     //认证方式只能定义其中的一种
//#define EAM_md5AndAesEnrypt 1
#define EAM_macNoEncrypt 2


#define DEVICE_KEY {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};

#ifdef EAM_macNoEncrypt
	#define AUTH_METHOD EAM_macNoEncrypt
	#define MD5_TYPE_AND_ID_LENGTH 0
	#define CIPHER_TEXT_LENGTH 0
#endif

#ifdef EAM_md5AndAesEnrypt
	#define AUTH_METHOD EAM_md5AndAesEnrypt
	#define MD5_TYPE_AND_ID_LENGTH 16
	#define CIPHER_TEXT_LENGTH 16
#endif
#ifdef EAM_md5AndNoEnrypt
	#define AUTH_METHOD EAM_md5AndNoEnrypt
	#define MD5_TYPE_AND_ID_LENGTH 16
	#define CIPHER_TEXT_LENGTH 0
#endif

#define CHALLENAGE_LENGTH 4

#define MPBLEDEMO2_MAGICCODE_H 0xfe
#define MPBLEDEMO2_MAGICCODE_L 0xcf
#define MPBLEDEMO2_VERSION 0x01
//#define SEND_HELLO_WECHAT "Hello, WeChat!"
#define SEND_HELLO_WECHAT "Hello, WeChat!"

/* Hardware Resources define */
#define MPBLEDEMO2_LIGHT 19
#define MPBLEDEMO2_BUTTON_1 17

//解包错误码，数值可以根据需求自定义
typedef enum
{
	errorCodeUnpackAuthResp = 0x9990,
	errorCodeUnpackInitResp = 0x9991,
	errorCodeUnpackSendDataResp = 0x9992,
	errorCodeUnpackCtlCmdResp = 0x9993,
	errorCodeUnpackRecvDataPush = 0x9994,
	errorCodeUnpackSwitchViewPush = 0x9995,
	errorCodeUnpackSwitchBackgroundPush = 0x9996,
	errorCodeUnpackErrorDecode = 0x9997,
}mpbledemo2UnpackErrorCode;

//打包错误码，数值可以根据需求自定义
typedef enum
{
	errorCodeProduce = 0x9980,
}mpbledemo2PackErrorCode;

//对Mpbledemo2的相关命令定义
typedef enum
{
	sendTextReq = 0x01,				//发送字符
	sendTextResp = 0x1001,		//收到字符
	openLightPush = 0x2001,		//开灯命令
	closeLightPush = 0x2002,	//关灯命令
}BleDemo2CmdID;

//Mpbledemo2数据头格式
typedef struct
{
	uint8_t m_magicCode[2];//魔数
	uint16_t m_version;//版本
	uint16_t m_totalLength;//数据总长度
	uint16_t m_cmdid;//命令号
	uint16_t m_seq;//序号
	uint16_t m_errorCode;//错误码
}BlueDemoHead;

//Mpbledemo2数据结构体
//这个结构体主要用于本地对设备的控制，要向设备传输某参数时，首先初始化此结构体，
//然后把这个结构体当做一个参数传递过去
//在mpbledemo2_data_produce_func函数中使用
typedef struct 
{
	int cmd;
	CString send_msg;
} mpbledemo2_info;

//Mpbledemo2 状态结构体
 typedef struct 
{
	bool wechats_switch_state; //公众账号切换到前台的状态
	bool indication_state;//indication char是否被订阅，true表示被订阅，false表示未订阅
	bool auth_state;//auth状态，true表示auth成功，false表示为auth成功或者未进行auth
	bool init_state;//int状态，true表示init成功，false表示为init成功或者未进行init
	bool auth_send;//auth发送状态，true表示已经发送auth包，false表示未发送auth包
	bool init_send;//init发送状态，true表示已经发送init包，false表示未发送init包
	unsigned short send_data_seq;//记录发送数据的序列号，非必须
	unsigned short push_data_seq;//记录接收数据的序列号，非必须
	unsigned short seq;//当前序列号
}mpbledemo2_state;
extern data_handler mpbledemo2_data_handler;
#endif
