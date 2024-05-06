/*
*	modified  by anqiren  2014/12/02  V1.0bat
*
**/

#include "ble_wechat_service.h"
#include "nrf_gpio.h"
#include "ble_conn_params.h"
#include "ble_stack_handler.h"
#include "ble_types.h"
#include "mpbledemo2.h"
#include "ble_wechat_util.h"

static uint8_t m_addl_adv_manuf_data [BLE_GAP_ADDR_LEN];


#define USING_PRODUCT PRODUCT_TYPE_MPBLEDEMO2

/*****************************************************************************
* data handle
*****************************************************************************/
 data_info g_send_data = {NULL, 0, 0};
 data_info g_rcv_data = {NULL, 0, 0};
 
static int ble_wechat_indicate_data_chunk(ble_wechat_t *p_wcs, data_handler *p_data_handler)
{
		ble_gatts_hvx_params_t hvx_params;
		uint16_t chunk_len = 0;
	
		chunk_len = g_send_data.len-g_send_data.offset;
		chunk_len = chunk_len > BLE_WECHAT_MAX_DATA_LEN?BLE_WECHAT_MAX_DATA_LEN:chunk_len;

		if (chunk_len == 0) {
				p_data_handler->m_data_free_func(g_send_data.data, g_send_data.len);
				g_send_data.data = NULL;
				g_send_data.len = 0;
				g_send_data.offset = 0;
				return 0;
		}
		memset(&hvx_params, 0, sizeof(hvx_params));
		hvx_params.handle = p_wcs->indicate_handles.value_handle;
    hvx_params.p_data = g_send_data.data+g_send_data.offset;
    hvx_params.p_len  = &chunk_len;
    hvx_params.type   = BLE_GATT_HVX_INDICATION;

		g_send_data.offset += chunk_len;
		return sd_ble_gatts_hvx(p_wcs->conn_handle, &hvx_params);
}

//该函数仅负责发送一帧函数，并记录当前offset，当成功发送一帧数据之后，ble stack会抛上来一个确认事件，p_ble_evt->header.evt_id = BLE_GATTS_EVT_HVC
//当收到这个事件以后调用下面的 on_indicate_comfirm 函数以发送剩余数据直到数据发送完毕
void on_indicate_comfirm(ble_wechat_t *p_wcs, ble_evt_t * p_ble_evt, data_handler *p_data_handler)
{
		ble_wechat_indicate_data_chunk(p_wcs,p_data_handler);
}

/**@brief Function for error handling, which is called when an error has occurred. 
 * @warning This handler is an example only and does not fit a final product. You need to analyze 
 *          how your product is supposed to react in case of error.
 *
 * @param[in] error_code  Error code supplied to the handler.
 * @param[in] p_data_handler where Error come from
 */
//打印出错误信息并调用当前设备的错误处理函数
void wechat_error_chack(ble_wechat_t *p_wcs, data_handler *p_data_handler, int error_code)
{
	switch(error_code)
	{
		case EEC_system:
		{
		#ifdef CATCH_LOG
			printf("\r\n! error: system error");
		#endif
		}
			break ;
		case EEC_needAuth:
		#ifdef CATCH_LOG
			printf("\r\n! error: needAuth");
		#endif
			break ;
		case EEC_sessionTimeout:
		#ifdef CATCH_LOG
			printf("\r\n! error: sessionTimeout");
		#endif
			break ;
		case EEC_decode:
		#ifdef CATCH_LOG
			printf("\r\n! error: decode");
		#endif
			break ;
		case EEC_deviceIsBlock:
		#ifdef CATCH_LOG
			printf("\r\n! error: deviceIsBlock");
		#endif
			break ;
		case EEC_serviceUnAvalibleInBackground:
		#ifdef CATCH_LOG
			printf("\r\n! error: serviceUnAvalibleInBackground");
		#endif
			break ;
		case EEC_deviceProtoVersionNeedUpdate:
		#ifdef CATCH_LOG
			printf("\r\n! error: deviceProtoVersionNeedUpdate");
		#endif
			break ;
		case EEC_phoneProtoVersionNeedUpdate:
		#ifdef CATCH_LOG
			printf("\r\n! error: phoneProtoVersionNeedUpdate");
		#endif
			break ;
		case EEC_maxReqInQueue:
		#ifdef CATCH_LOG
			printf("\r\n! error: maxReqInQueue");
		#endif
			break ;
		case EEC_userExitWxAccount:
		#ifdef CATCH_LOG
			printf("\r\n! error: userExitWxAccount");
		#endif
			break ;
		default:
			break ;
	}
	p_data_handler->m_data_error_func(error_code);//该函数在设备中定义
}

/**@brief     Function for handling the @ref BLE_GATTS_EVT_WRITE event from the S110 SoftDevice.
 *
 * @param[in] p_ble_evt Pointer to the event received from BLE stack.
 */
//这个是微信客户端发起写事件的处理函数，用来接收push下来的数据。当接收完所有数据之后
//会调用设备的 p_data_handler->m_data_consume_func(g_rcv_data.data, g_rcv_data.len)函数，
//该函数在设备中定义
static void on_write(ble_wechat_t *p_wcs, ble_evt_t * p_ble_evt, data_handler *p_data_handler)
{
		int error_code;
		ble_gatts_evt_write_t * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;
		int chunk_size = 0;
		if (p_evt_write->handle == p_wcs->write_handles.value_handle &&
				p_evt_write->len <= BLE_WECHAT_MAX_DATA_LEN) 
		{
				if (g_rcv_data.len == 0) 
				{
					BpFixHead *fix_head = (BpFixHead *) p_evt_write->data;
					g_rcv_data.len = ntohs(fix_head->nLength);
					g_rcv_data.offset = 0;
					g_rcv_data.data = (uint8_t *)malloc(g_rcv_data.len);
				}	
				chunk_size = g_rcv_data.len - g_rcv_data.offset;
				chunk_size = chunk_size < p_evt_write->len ? chunk_size : p_evt_write->len;
				memcpy(g_rcv_data.data+g_rcv_data.offset, p_evt_write->data, chunk_size);
				g_rcv_data.offset += chunk_size;
				if (g_rcv_data.len <= g_rcv_data.offset) 
				{
					error_code = p_data_handler->m_data_consume_func(g_rcv_data.data, g_rcv_data.len);
					p_data_handler->m_data_free_func(g_rcv_data.data,g_rcv_data.len);
					wechat_error_chack(p_wcs, p_data_handler, error_code);
					g_rcv_data.len = 0;
					g_rcv_data.offset = 0;
					
				}	
		}
}

//微信服务事件处理函数
void ble_wechat_on_ble_evt(ble_wechat_t *p_wcs,ble_evt_t * p_ble_evt, data_handler *p_data_handler) {
		switch (p_ble_evt->header.evt_id)
    {
				case BLE_GAP_EVT_CONNECTED://获取当前链接
					p_wcs->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;	
						break;
        case BLE_GATTS_EVT_WRITE://接收数据
            on_write(p_wcs,p_ble_evt,p_data_handler);
            break;
        case BLE_GATTS_EVT_HVC://数据确认
						on_indicate_comfirm(p_wcs,p_ble_evt,p_data_handler);
						break;	
				case BLE_GAP_EVT_DISCONNECTED://断开链接
						p_wcs->conn_handle = BLE_CONN_HANDLE_INVALID;
						break;
        default:
            break;
    }
}

//在GATTS service中添加微信服务
//Add WeChat Service to GATTS sercvice
uint32_t ble_wechat_add_service(ble_wechat_t *p_wechat)
{
		uint32_t err_code;
		ble_uuid_t ble_wechat_uuid;
		BLE_UUID_BLE_ASSIGN(ble_wechat_uuid, BLE_UUID_WECHAT_SERVICE);
		err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_wechat_uuid, &p_wechat->service_handle);
		return err_code;
}

static void get_mac_addr(uint8_t *p_mac_addr)
{
		uint32_t error_code;
		ble_gap_addr_t *p_mac_addr_t = (ble_gap_addr_t*)malloc(sizeof(ble_gap_addr_t));
		error_code = sd_ble_gap_address_get(p_mac_addr_t);
		APP_ERROR_CHECK(error_code);
		uint8_t *d = p_mac_addr_t->addr;
		for ( uint8_t i = 6; i >0;)
		{	
			i--;
			p_mac_addr[5-i]= d[i];
		}
		free(p_mac_addr_t);
		p_mac_addr_t = NULL;
}

//微信服务特征字主要有三个：indicate_char,write_char,read_char
//为了支持同一手机上一个APP链接了设备后，微信还能搜索并连接到设备的情况
//设备需要在微信的service下面暴露一个 read charcater,内容为6字节的MAC地址
//当IOS上的其他APP链接还是那个设备时，设备不会再广播，微信会读取该特征值，以确定是否要链接该设备
//Add the indicate characteristic 
uint32_t ble_wechat_add_indicate_char(ble_wechat_t *p_wechat)
{
		ble_gatts_char_md_t char_md;
    ble_gatts_attr_md_t cccd_md;
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          char_uuid;
    ble_gatts_attr_md_t attr_md;
		char *data = "indicate char";
		memset(&cccd_md, 0, sizeof(cccd_md));
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);
		cccd_md.vloc = BLE_GATTS_VLOC_STACK;
	
		memset(&char_md, 0, sizeof(char_md));
    char_md.char_props.indicate = 1;
    char_md.p_char_user_desc  = NULL;
    char_md.p_char_pf         = NULL;
    char_md.p_user_desc_md    = NULL;
    char_md.p_cccd_md         = NULL;//&cccd_md;
    char_md.p_sccd_md         = NULL;
	
		BLE_UUID_BLE_ASSIGN(char_uuid, BLE_UUID_WECHAT_INDICATE_CHARACTERISTICS);
		memset(&attr_md, 0, sizeof(attr_md));
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&attr_md.write_perm);
    attr_md.vloc       = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth    = 0;
    attr_md.wr_auth    = 0;
    attr_md.vlen       = 1;
    
    memset(&attr_char_value, 0, sizeof(attr_char_value));
   
    attr_char_value.p_uuid       = &char_uuid;
    attr_char_value.p_attr_md    = &attr_md;
    attr_char_value.init_len     = strlen(data);
    attr_char_value.init_offs    = 0;
    attr_char_value.max_len      = BLE_WECHAT_MAX_DATA_LEN;
    attr_char_value.p_value      = (uint8_t *)data;
    
    return sd_ble_gatts_characteristic_add(p_wechat->service_handle,
                                           &char_md,
                                           &attr_char_value,
                                           &p_wechat->indicate_handles);
}
//添加write_char
//Add the write characteristic 
static uint32_t ble_wechat_add_write_char(ble_wechat_t *p_wechat)
{
		ble_gatts_char_md_t char_md;
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          char_uuid;
    ble_gatts_attr_md_t attr_md;
		char *data = "write char";
	
		memset(&char_md, 0, sizeof(char_md));
    char_md.char_props.write = 1;
    char_md.p_char_user_desc  = NULL;
    char_md.p_char_pf         = NULL;
    char_md.p_user_desc_md    = NULL;
    char_md.p_cccd_md         = NULL;
    char_md.p_sccd_md         = NULL;
	
		BLE_UUID_BLE_ASSIGN(char_uuid, BLE_UUID_WECHAT_WRITE_CHARACTERISTICS);
	
		memset(&attr_md, 0, sizeof(attr_md));
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);
    attr_md.vloc       = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth    = 0;
    attr_md.wr_auth    = 0;
    attr_md.vlen       = 1;
    
    memset(&attr_char_value, 0, sizeof(attr_char_value));
    
    attr_char_value.p_uuid       = &char_uuid;
    attr_char_value.p_attr_md    = &attr_md;
    attr_char_value.init_len     = strlen(data);
    attr_char_value.init_offs    = 0;
    attr_char_value.max_len      = BLE_WECHAT_MAX_DATA_LEN;
    attr_char_value.p_value      = (uint8_t *)data;
    
    return sd_ble_gatts_characteristic_add(p_wechat->service_handle,
                                           &char_md,
                                           &attr_char_value,
                                           &p_wechat->write_handles);
}
//添加read_char，需要获取当前设备的MAC地址，用来初始化
//Add the read characteristic 
static uint32_t ble_wechat_add_read_char(ble_wechat_t *p_wechat)
{
	//m_addl_adv_manuf_data.
		get_mac_addr(m_addl_adv_manuf_data);
		ble_gatts_char_md_t char_md;
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          char_uuid;
    ble_gatts_attr_md_t attr_md;
	
		memset(&char_md, 0, sizeof(char_md));
    char_md.char_props.read = 1;
    char_md.p_char_user_desc  = NULL;
    char_md.p_char_pf         = NULL;
    char_md.p_user_desc_md    = NULL;
    char_md.p_cccd_md         = NULL;
    char_md.p_sccd_md         = NULL;
	
		BLE_UUID_BLE_ASSIGN(char_uuid, BLE_UUID_WECHAT_READ_CHARACTERISTICS);
	
		memset(&attr_md, 0, sizeof(attr_md));
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);
    attr_md.vloc       = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth    = 0;
    attr_md.wr_auth    = 0;
    attr_md.vlen       = 1;
    
    memset(&attr_char_value, 0, sizeof(attr_char_value));
    
    attr_char_value.p_uuid       = &char_uuid;
    attr_char_value.p_attr_md    = &attr_md;
    attr_char_value.init_len     = sizeof(m_addl_adv_manuf_data);
    attr_char_value.init_offs    = 0;
    attr_char_value.max_len      = BLE_WECHAT_MAX_DATA_LEN;
    attr_char_value.p_value      = m_addl_adv_manuf_data;
    
    return sd_ble_gatts_characteristic_add(p_wechat->service_handle,
                                           &char_md,
                                           &attr_char_value,
                                           &p_wechat->read_handles);
}
//Add the WeChat characteristic include indicate write and read characteristic
uint32_t ble_wechat_add_characteristics(ble_wechat_t *p_wechat)
{
		uint32_t err_code;
		err_code = ble_wechat_add_indicate_char(p_wechat);
		APP_ERROR_CHECK(err_code);
		err_code = ble_wechat_add_write_char(p_wechat);
		APP_ERROR_CHECK(err_code);
		err_code = ble_wechat_add_read_char(p_wechat);
		APP_ERROR_CHECK(err_code);
		return err_code;
}

//device sent data on the indicate characteristic
//微信服务数据发送函数
int ble_wechat_indicate_data(ble_wechat_t *p_wcs,data_handler *p_data_handler, uint8_t *data, int len)
{

		if (data == NULL || len == 0) {
				return 0;	
		}
		if (g_send_data.len != 0 && g_send_data.offset != g_send_data.len) {
			printf("\r\n offset:%d %d",g_send_data.offset,g_send_data.len);
//			g_send_data.len = 0;
//			g_send_data.offset = 0;
			
				return 0;
		}
//		nrf_gpio_pin_clear(19);
		g_send_data.data = data;
		g_send_data.len = len;
		g_send_data.offset = 0;
		return (ble_wechat_indicate_data_chunk(p_wcs,p_data_handler));
}








