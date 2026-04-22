/**
 ****************************************************************************************
 *
 * @file usbd_cdc.h
 *
 * @brief Header file of USB CDC function
 *
 ****************************************************************************************
 */

#ifndef _USBD_CDC_H_
#define _USBD_CDC_H_

#include <stdint.h>
#include "usb_cdc.h"


#if !defined(CDC_INST_CNT)
// MAX Count of CDC Instances
#define CDC_INST_CNT              (3)
#endif

/* Max packet size */
#define CDC_INT_EP_MPS            (0x0040)
#define CDC_BULK_EP_MPS           (0x0040)

/* Length of CDC descriptor: 66 bytes */
#define CDC_ACM_DESCRIPTOR_LEN    (8 + 9 + 5 + 5 + 4 + 5 + 7 + 9 + 7 + 7)

/* Macro for template descriptor */
#define CDC_ACM_DESCRIPTOR_INIT(bFirstInterface, int_ep, out_ep, in_ep, str_idx)   \
    /* Interface Associate */                                                      \
    0x08,                                      /* bLength */                       \
    USB_DESC_TYPE_INTERFACE_ASSOCIATION,       /* bDescriptorType */               \
    bFirstInterface,                           /* bFirstInterface */               \
    0x02,                                      /* bInterfaceCount */               \
    USB_DEVICE_CLASS_CDC,                      /* bFunctionClass */                \
    CDC_ABSTRACT_CONTROL_MODEL,                /* bFunctionSubClass */             \
    CDC_COMMON_PROTOCOL_ATCMD,                 /* bFunctionProtocol */             \
    0x00,                                      /* iFunction */                     \
    /* Interface CDC_CMD */                                                        \
    0x09,                                      /* bLength */                       \
    USB_DESC_TYPE_INTERFACE,                   /* bDescriptorType */               \
    bFirstInterface,                           /* bInterfaceNumber */              \
    0x00,                                      /* bAlternateSetting */             \
    0x01,                                      /* bNumEndpoints */                 \
    USB_DEVICE_CLASS_CDC,                      /* bInterfaceClass */               \
    CDC_ABSTRACT_CONTROL_MODEL,                /* bInterfaceSubClass */            \
    CDC_COMMON_PROTOCOL_ATCMD,                 /* bInterfaceProtocol */            \
    str_idx,                                   /* iInterface */                    \
    /* Header Functional Descriptor */                                             \
    0x05,                                      /* bLength */                       \
    CDC_CS_INTERFACE,                          /* bDescriptorType */               \
    CDC_FUNC_DESC_HEADER,                      /* bDescriptorSubtype */            \
    WBVAL(CDC_V1_10),                          /* bcdCDC */                        \
    /* Call Management Functional */                                               \
    0x05,                                      /* bLength */                       \
    CDC_CS_INTERFACE,                          /* bDescriptorType */               \
    CDC_FUNC_DESC_CALL_MANAGEMENT,             /* bDescriptorSubtype */            \
    0x00,                                      /* bmCapabilities */                \
    (uint8_t)(bFirstInterface + 1),            /* bDataInterface */                \
    /* ACM Functional Descriptor */                                                \
    0x04,                                      /* bLength */                       \
    CDC_CS_INTERFACE,                          /* bDescriptorType */               \
    CDC_FUNC_DESC_ABSTRACT_CONTROL_MANAGEMENT, /* bDescriptorSubtype */            \
    0x02,                                      /* bmCapabilities */                \
    /* Union Functional Descriptor */                                              \
    0x05,                                      /* bLength */                       \
    CDC_CS_INTERFACE,                          /* bDescriptorType */               \
    CDC_FUNC_DESC_UNION,                       /* bDescriptorSubtype */            \
    bFirstInterface,                           /* bMasterInterface */              \
    (uint8_t)(bFirstInterface + 1),            /* bSlaveInterface0 */              \
    0x07,                                      /* bLength */                       \
    USB_DESC_TYPE_ENDPOINT,                    /* bDescriptorType */               \
    int_ep,                                    /* bEndpointAddress */              \
    0x03,                                      /* bmAttributes */                  \
    WBVAL(CDC_INT_EP_MPS),                     /* wMaxPacketSize */                \
    0x00,                                      /* bInterval */                     \
    /* Interface CDC_DATA */                                                       \
    0x09,                                      /* bLength */                       \
    USB_DESC_TYPE_INTERFACE,                   /* bDescriptorType */               \
    (uint8_t)(bFirstInterface + 1),            /* bInterfaceNumber */              \
    0x00,                                      /* bAlternateSetting */             \
    0x02,                                      /* bNumEndpoints */                 \
    CDC_DATA_INTERFACE_CLASS,                  /* bInterfaceClass */               \
    0x00,                                      /* bInterfaceSubClass */            \
    0x00,                                      /* bInterfaceProtocol */            \
    0x00,                                      /* iInterface */                    \
    0x07,                                      /* bLength */                       \
    USB_DESC_TYPE_ENDPOINT,                    /* bDescriptorType */               \
    out_ep,                                    /* bEndpointAddress */              \
    0x02,                                      /* bmAttributes */                  \
    WBVAL(CDC_BULK_EP_MPS),                    /* wMaxPacketSize */                \
    0x00,                                      /* bInterval */                     \
    0x07,                                      /* bLength */                       \
    USB_DESC_TYPE_ENDPOINT,                    /* bDescriptorType */               \
    in_ep,                                     /* bEndpointAddress */              \
    0x02,                                      /* bmAttributes */                  \
    WBVAL(CDC_BULK_EP_MPS),                    /* wMaxPacketSize */                \
    0x00                                       /* bInterval */


enum cdc_param_type {
    CDC_LINE_CODING,
    CDC_LINE_STATE,
};

enum cdc_state_type {
    CDC_STATE_IDLE,
    CDC_STATE_BUSY,
};

typedef struct usbd_cdc_tag {
    /* CDC ACM interface */
    uint8_t        intf_num;
    uint8_t        ep_in;
    /* CDC ACM state */
    uint8_t        cdc_state;
    /* CDC ACM line state bitmap, DTE side */
    uint8_t        line_state;
    /* CDC ACM line coding properties */
    struct cdc_line_coding line_coding;
    /* CDC ACM tx data */
    const uint8_t *txdata_ptr;
    uint16_t       txdata_len;
    uint16_t       txdata_res;
} usbd_cdc_t;


void usbd_cdc_init(uint8_t idx, uint8_t intf_num, uint8_t ep_in);

void usbd_cdc_reset(void);

void usbd_cdc_updated(usbd_cdc_t *cdc, uint8_t type);

uint8_t usbd_cdc_ep_send(uint8_t ep, uint16_t len, const uint8_t *data);

uint8_t usbd_cdc_id_send(uint8_t id, uint16_t len, const uint8_t *data);

void usbd_cdc_bulk_in_handler(uint8_t ep);

void usbd_cdc_bulk_out_handler(uint8_t ep);

uint8_t usbd_cdc_class_handler(struct usb_setup_packet *setup, uint8_t **data, uint16_t *len);


#endif // _USBD_CDC_H_
