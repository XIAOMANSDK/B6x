/**
 ****************************************************************************************
 *
 * @file usbd_cdc.c
 *
 * @brief Function of USB Communications Device Class (CDC) 
 *
 ****************************************************************************************
 */

#include "usbd.h"
#include "usbd_cdc.h"


/// CDC global environment
static usbd_cdc_t usbd_cdcs[CDC_INST_CNT];


/// Interface number convert to cdc instance
static usbd_cdc_t *find_cdc_by_intf(uint8_t intf_num)
{
    for (uint8_t i = 0; i < CDC_INST_CNT; i++) {
        if (usbd_cdcs[i].intf_num == intf_num) {
            return &usbd_cdcs[i];
        }
    }
    
    USB_LOG_ERR("Not Find CDC(intf:%d)\r\n", intf_num);
    return NULL;
}

/// Endpoint address convert to cdc instance
static usbd_cdc_t *find_cdc_by_ep(uint8_t ep_addr)
{
    for (uint8_t i = 0; i < CDC_INST_CNT; i++) {
        if (usbd_cdcs[i].ep_in == ep_addr) {
            return &usbd_cdcs[i];
        }
    }
    
    USB_LOG_ERR("Not Find CDC(ep:%d)\r\n", ep_addr);
    return NULL;
}

static void dflt_cdc_param(usbd_cdc_t *cdc)
{
    cdc->cdc_state = CDC_STATE_IDLE;
    cdc->txdata_len = 0;
    cdc->txdata_res = 0;
    
    cdc->line_state = 0;
    cdc->line_coding.dwDTERate = 115200;
    cdc->line_coding.bDataBits = 8;
    cdc->line_coding.bParityType = 0;
    cdc->line_coding.bCharFormat = 0;
}

void usbd_cdc_init(uint8_t idx, uint8_t intf_num, uint8_t ep_in)
{
    //usbd_cdc_t *cdc;
    if (idx < CDC_INST_CNT) {
        dflt_cdc_param(&usbd_cdcs[idx]);
        usbd_cdcs[idx].intf_num = intf_num;
        usbd_cdcs[idx].ep_in = ep_in;
    }
}

void usbd_cdc_reset(void)
{
    for (uint8_t i = 0; i < CDC_INST_CNT; i++) {
        dflt_cdc_param(&usbd_cdcs[i]);
    }
}

__WEAK void usbd_cdc_updated(usbd_cdc_t *cdc, uint8_t type)
{
    /*!< here you can update params to hardware */
    
}

static uint8_t usbd_cdc_send(usbd_cdc_t *cdc, uint16_t len, const uint8_t *data)
{
    uint8_t status = USBD_FAIL;

    if (cdc && (cdc->cdc_state == CDC_STATE_IDLE)) {
        cdc->txdata_ptr = data;
        cdc->txdata_len = len;
        //cdc->txdata_res = 0;
        cdc->cdc_state = CDC_STATE_BUSY; // Update before isr occure
        
        status = usbd_ep_write(cdc->ep_in, cdc->txdata_len, cdc->txdata_ptr, &cdc->txdata_res);
        if (status != USBD_OK) {
            cdc->cdc_state = CDC_STATE_IDLE; // fail to recover
        } else {
            // continue send in bulk_in_handler
        }
    }
    
    return status;
}

uint8_t usbd_cdc_ep_send(uint8_t ep, uint16_t len, const uint8_t *data)
{
    uint8_t status = USBD_FAIL;

    if (usbd_is_configured()) {
        usbd_cdc_t *curr_cdc = find_cdc_by_ep(ep);
        
        status = usbd_cdc_send(curr_cdc, len, data);
    }
    
    return status;
}

uint8_t usbd_cdc_id_send(uint8_t id, uint16_t len, const uint8_t *data)
{
    uint8_t status = USBD_FAIL;

    if ((id < CDC_INST_CNT) && usbd_is_configured()) {
        usbd_cdc_t *curr_cdc = &usbd_cdcs[id];
        
        status = usbd_cdc_send(curr_cdc, len, data);
    }
    
    return status;
}

void usbd_cdc_bulk_in_handler(uint8_t ep)
{
    usbd_cdc_t *curr_cdc = find_cdc_by_ep(ep);
    
    if (curr_cdc && (curr_cdc->cdc_state == CDC_STATE_BUSY)) {
        uint16_t chunk = curr_cdc->txdata_res;

        if (chunk != curr_cdc->txdata_len) {
            // More bulk, continue send
            curr_cdc->txdata_len -= chunk;
            curr_cdc->txdata_ptr += chunk;
            
            if (usbd_ep_write(ep, curr_cdc->txdata_len, curr_cdc->txdata_ptr, &curr_cdc->txdata_res) != USBD_OK) {
                curr_cdc->cdc_state = CDC_STATE_IDLE; // fail to recover
            }
        } else {
            // Last bulk, send ZLP if times of CDC_BULK_EP_MPS
            if (chunk == CDC_BULK_EP_MPS) {
                curr_cdc->txdata_len = 0;
                curr_cdc->txdata_res = 0;
                usbd_ep_write(ep, 0, NULL, NULL); // send ZLP
            } else {
                curr_cdc->cdc_state = CDC_STATE_IDLE; // finish
            }
        }
    }
}

__WEAK void usbd_cdc_bulk_out_handler(uint8_t ep)
{
    uint8_t data[CDC_BULK_EP_MPS];
    uint16_t read_byte = usbd_ep_read(ep, CDC_BULK_EP_MPS, data);
    USB_LOG_DBG("CDC Bulk Out(ep:%d,len:%d)\r\n", ep, read_byte);
    
    /*!< here you can output data to hardware */
}

/**
 * @brief Handler called for Class requests.
 *
 * @param setup    Information about the request to execute.
 * @param data     Buffer containing the request result.
 * @param len      Size of the buffer.
 *
 * @return  0 on success, errno code on fail.
 */
uint8_t usbd_cdc_class_handler(struct usb_setup_packet *setup, uint8_t **data, uint16_t *len)
{
    usbd_cdc_t *curr_cdc;
    uint8_t intf_num;
    
    if ((setup->bmRequestType & USB_REQUEST_TYPE_MASK) != USB_REQUEST_CLASS) {
        return USBD_FAIL;
    }
    
    intf_num = (uint8_t)setup->wIndex;
    curr_cdc = find_cdc_by_intf(intf_num);
    
    if (!curr_cdc) {
        return USBD_FAIL;
    }
    
    USB_LOG_DBG("CDC Class request:0x%02x,intf:%d\r\n", setup->bRequest, intf_num);

    switch (setup->bRequest) {
        case CDC_REQUEST_SET_LINE_CODING:
        {
            if (memcmp(&(curr_cdc->line_coding), *data, sizeof(struct cdc_line_coding)) != 0) {
                // param update, save it
                memcpy(&(curr_cdc->line_coding), *data, sizeof(struct cdc_line_coding));
                USB_LOG_DBG("CDC_SET_LINE_CODING <Rate:%d,DataBits:%d,Parity:%d,StopBits:%d>\r\n",
                            curr_cdc->line_coding.dwDTERate, curr_cdc->line_coding.bDataBits,
                            curr_cdc->line_coding.bParityType, curr_cdc->line_coding.bCharFormat);
                
                usbd_cdc_updated(curr_cdc, CDC_LINE_CODING);
            }
         } break;

        case CDC_REQUEST_SET_CONTROL_LINE_STATE:
        {
            uint8_t line_state = (uint8_t)setup->wValue;
            
            if (curr_cdc->line_state != line_state) {
                // param update, save it
                curr_cdc->line_state = line_state;
                USB_LOG_DBG("CDC_SET_LINE_STATE <DTR:0x%x,RTS:0x%x>\r\n", (line_state & 0x01), (line_state & 0x02));
                
                usbd_cdc_updated(curr_cdc, CDC_LINE_STATE);
            }
        } break;

        case CDC_REQUEST_GET_LINE_CODING:
        {
            *data = (uint8_t *)&(curr_cdc->line_coding);
            *len = sizeof(struct cdc_line_coding);
            
            USB_LOG_DBG("CDC_GET_LINE_CODING <Rate:%d,DataBits:%d,Parity:%d,StopBits:%d>\r\n",
                        curr_cdc->line_coding.dwDTERate, curr_cdc->line_coding.bDataBits,
                        curr_cdc->line_coding.bParityType, curr_cdc->line_coding.bCharFormat);
        } break;

        default:
            USB_LOG_WRN("Unhandled CDC Class bRequest 0x%02x\r\n", setup->bRequest);
            return USBD_FAIL;
    }

    return USBD_OK;
}
