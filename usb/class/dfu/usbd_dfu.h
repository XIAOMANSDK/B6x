/**
 ****************************************************************************************
 *
 * @file usbd_dfu.h
 *
 * @brief Header file of USB Device Firmware Upgrade Class function
 *
 ****************************************************************************************
 */
#ifndef USBD_DFU_H
#define USBD_DFU_H

#include "usb_dfu.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Modify the following parameters according to different platforms */
#ifndef DFU_APP_ADDR
#define DFU_APP_ADDR                0x18004000
#endif

#ifndef DFU_INFO_ADDR
#define DFU_INFO_ADDR               0x18000000
#endif
#ifndef DFU_INFO_SIZE
#define DFU_INFO_SIZE               0x100 // Info Page
#endif

#ifndef DFU_XFER_SIZE
#define DFU_XFER_SIZE               1024
#endif

#ifndef DFU_ATTR_CAPABLE
#define DFU_ATTR_CAPABLE            (DFU_ATTR_WILL_DETACH | DFU_ATTR_CAN_UPLOAD | DFU_ATTR_CAN_DNLOAD) // 0x0B
#endif

#ifndef DFU_ENB_UPLOAD
#define DFU_ENB_UPLOAD              1
#endif

/* Timeouts, unit in ms */
#ifndef DFU_DETACH_TO
#define DFU_DETACH_TO               200
#endif

/** DFUSE Special Commands with Download Request  */
#define DFU_CMD_SE_CMDS             0x00U
#define DFU_CMD_SE_SETADDR          0x21U
#define DFU_CMD_SE_ERASE            0x41U

#define DFU_CMD_WRITE               0x81U
#define DFU_CMD_ENDED               0x82U
#define DFU_CMD_UNKNOWN             0x00U

/*Length of template descriptor: 18 bytes*/
#define DFU_DESCRIPTOR_LEN          (9 + 9)

/* Macro for template descriptor */
#define DFU_DESCRIPTOR_INIT(bInterface, bMode, bAttr, str_idx)                            \
    0x09,                           /* bLength */                                         \
    USB_DESC_TYPE_INTERFACE,        /* bDescriptorType */                                 \
    bInterface,                     /* bInterfaceNumber */                                \
    0x00,                           /* bAlternateSetting */                               \
    0x00,                           /* bNumEndpoints Default Control Pipe only */         \
    USB_DEVICE_CLASS_APP_SPECIFIC,  /* bInterfaceClass */                                 \
    0x01,                           /* bInterfaceSubClass Device Firmware Upgrade */      \
    bMode,                          /* bInterfaceProtocol 1=Runtime 2=dfu mode */         \
    str_idx, /* iInterface */       /*!< Device Firmware Update Functional Descriptor  */ \
    0x09,                           /* bLength */                                         \
    0x21,                           /* DFU Functional Descriptor */                       \
    bAttr,                          /* bmAttributes */                                    \
    WBVAL(DFU_DETACH_TO),           /* wDetachTimeOut unit in ms */                       \
    WBVAL(DFU_XFER_SIZE),           /* wTransferSize */                                   \
    WBVAL(DFU_VERSION)              /* bcdDFUVersion */


/*
 * FUNCTION DECLARATION
 ****************************************************************************************
 */

/* DFU Interface functions that need to be implemented by the user */
uint8_t dfu_itf_read(uint32_t addr, uint8_t *buff, uint32_t len);

uint8_t dfu_itf_write(uint32_t addr, const uint8_t *data, uint32_t len);

uint8_t dfu_itf_erase(uint32_t addr);

void dfu_itf_leave(uint16_t timeout);

/* Init DFU environment */
void usbd_dfu_init(void);

/* Sched DFU operations */
void usbd_dfu_schedule(void);

/* DFU Request Handler */
uint8_t usbd_dfu_class_handler(struct usb_setup_packet *setup, uint8_t **data, uint16_t *len);

#ifdef __cplusplus
}
#endif

#endif /* USBD_DFU_H */
