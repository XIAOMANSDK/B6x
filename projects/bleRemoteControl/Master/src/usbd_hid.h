/**
 ****************************************************************************************
 *
 * @file usbd_hid.h
 *
 * @brief Header file of USB HID function
 *
 ****************************************************************************************
 */

#ifndef _USBD_HID_H_
#define _USBD_HID_H_

#include <stdint.h>
#include "usb_hid.h"

#if !defined(HID_INST_CNT)
// MAX Count of HID Instances
#define HID_INST_CNT              (2)
#endif

/* Macro for template descriptor */
#define HID_INTERFACE_INIT(intf_num, ep_cnt, subclass, proto, str_idx, desc_size)         \
    /* Descriptor of interface */                                                         \
    0x09,                         /* bLength: Interface Descriptor size */                \
    USB_DESC_TYPE_INTERFACE,      /* bDescriptorType: Interface descriptor type */        \
    intf_num,                     /* bInterfaceNumber: Number of Interface */             \
    0x00,                         /* bAlternateSetting: Alternate setting */              \
    ep_cnt,                       /* bNumEndpoints */                                     \
    0x03,                         /* bInterfaceClass: HID */                              \
    subclass,                     /* bInterfaceSubClass : 1=BOOT, 0=no boot */            \
    proto,                        /* bInterfaceProtocol : 0=none, 1=keyboard, 2=mouse */  \
    str_idx,                      /* iInterface: Index of string descriptor */            \
    /* Descriptor of HID Report */                                                        \
    0x09,                         /* bLength: HID Descriptor size */                      \
    HID_DESC_TYPE_HID,            /* bDescriptorType: HID */                              \
    0x11,                         /* bcdHID: HID Class Spec release number */             \
    0x01,                                                                                 \
    0x00,                         /* bCountryCode: Hardware target country */             \
    0x01,                         /* bNumDescriptors: Number of HID class descriptors */  \
    HID_DESC_TYPE_HID_REPORT,     /* bDescriptorType */                                   \
    desc_size,                    /* wItemLength: Total length of Report descriptor */    \
    0x00                                                                                  \

#define HID_ENDPOINT_DESC(ep_addr, ep_mps, interval)                                      \
    0x07,                         /* bLength: Endpoint Descriptor size */                 \
    USB_DESC_TYPE_ENDPOINT,       /* bDescriptorType: */                                  \
    ep_addr,                      /* bEndpointAddress: Endpoint Address (IN) */           \
    0x03,                         /* bmAttributes: Interrupt endpoint */                  \
    ep_mps,                       /* wMaxPacketSize: 4 Byte max */                        \
    0x00,                                                                                 \
    interval                      /* bInterval: Polling Interval */


enum hid_state_type {
    HID_STATE_IDLE,
    HID_STATE_BUSY,
};

typedef struct hid_intf_tag {
    uint8_t        intf_num;
    uint8_t        ep_in;
    uint16_t       desc_size;
    const uint8_t *report_desc;
} hid_intf_t;

#define HID_INTF_T(_intf_num, _ep_in, _desc)               \
            { .intf_num=_intf_num,      .ep_in=_ep_in,     \
              .desc_size=sizeof(_desc), .report_desc=_desc }

typedef struct usbd_hid_tag {
    const hid_intf_t *hid_intf;
    union
    {
        struct
        {
            volatile uint8_t hid_state;
            uint8_t report;
            uint8_t idle_state;
            uint8_t protocol;
        };
        uint32_t hid_info;
    };
} usbd_hid_t;

void usbd_hid_init(uint8_t idx, const hid_intf_t *intf);

void usbd_hid_reset(void);

void usbd_hid_leds(uint8_t state);

uint8_t usbd_hid_send_report(uint8_t ep, uint8_t len, const uint8_t *data);

void usbd_hid_ep_in_handler(uint8_t ep);

uint8_t usbd_hid_class_handler(struct usb_setup_packet *setup, uint8_t **data, uint16_t *len);

#endif // _USBD_HID_H_
