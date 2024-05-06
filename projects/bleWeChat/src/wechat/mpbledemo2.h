
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

//#define EAM_md5AndNoEnrypt 1     //��֤��ʽֻ�ܶ������е�һ��
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

//��������룬��ֵ���Ը��������Զ���
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

//��������룬��ֵ���Ը��������Զ���
typedef enum
{
	errorCodeProduce = 0x9980,
}mpbledemo2PackErrorCode;

//��Mpbledemo2����������
typedef enum
{
	sendTextReq = 0x01,				//�����ַ�
	sendTextResp = 0x1001,		//�յ��ַ�
	openLightPush = 0x2001,		//��������
	closeLightPush = 0x2002,	//�ص�����
}BleDemo2CmdID;

//Mpbledemo2����ͷ��ʽ
typedef struct
{
	uint8_t m_magicCode[2];//ħ��
	uint16_t m_version;//�汾
	uint16_t m_totalLength;//�����ܳ���
	uint16_t m_cmdid;//�����
	uint16_t m_seq;//���
	uint16_t m_errorCode;//������
}BlueDemoHead;

//Mpbledemo2���ݽṹ��
//����ṹ����Ҫ���ڱ��ض��豸�Ŀ��ƣ�Ҫ���豸����ĳ����ʱ�����ȳ�ʼ���˽ṹ�壬
//Ȼ�������ṹ�嵱��һ���������ݹ�ȥ
//��mpbledemo2_data_produce_func������ʹ��
typedef struct 
{
	int cmd;
	CString send_msg;
} mpbledemo2_info;

//Mpbledemo2 ״̬�ṹ��
 typedef struct 
{
	bool wechats_switch_state; //�����˺��л���ǰ̨��״̬
	bool indication_state;//indication char�Ƿ񱻶��ģ�true��ʾ�����ģ�false��ʾδ����
	bool auth_state;//auth״̬��true��ʾauth�ɹ���false��ʾΪauth�ɹ�����δ����auth
	bool init_state;//int״̬��true��ʾinit�ɹ���false��ʾΪinit�ɹ�����δ����init
	bool auth_send;//auth����״̬��true��ʾ�Ѿ�����auth����false��ʾδ����auth��
	bool init_send;//init����״̬��true��ʾ�Ѿ�����init����false��ʾδ����init��
	unsigned short send_data_seq;//��¼�������ݵ����кţ��Ǳ���
	unsigned short push_data_seq;//��¼�������ݵ����кţ��Ǳ���
	unsigned short seq;//��ǰ���к�
}mpbledemo2_state;
extern data_handler mpbledemo2_data_handler;
#endif
