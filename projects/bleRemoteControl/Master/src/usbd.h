/**
 * @file usbd.h
 *
 * @brief Header file of USB Device Descriptors & Interface
 *
 */

#ifndef _USBD_H_
#define _USBD_H_

#include <stdint.h>
#include <stdbool.h>
#include "string.h"

#include "usb_def.h"


/*
 * DEFINES
 ****************************************************************************************
 */

enum usbd_event_type {
    /** USB error reported by the controller */
    USBD_EVENT_ERROR,
    /** USB reset */
    USBD_EVENT_RESET,
    /** Start of Frame received */
    USBD_EVENT_SOF,
    /** USB enumeration completed */
    USBD_EVENT_CONNECTED,
    /** USB configuration done */
    USBD_EVENT_CONFIGURED,
    /** USB suspended by the HOST */
    USBD_EVENT_SUSPEND, //5
    /** USB connection lost */
    USBD_EVENT_DISCONNECTED,
    /** USB resumed by the HOST */
    USBD_EVENT_RESUME,

    /** USB interface selected */
    USBD_EVENT_SET_INTERFACE,
    /** Set Feature REMOTE_WAKEUP received */
    USBD_EVENT_SET_REMOTE_WAKEUP,
    /** Clear Feature REMOTE_WAKEUP received */
    USBD_EVENT_CLR_REMOTE_WAKEUP, //10
    /** Set Feature ENDPOINT_HALT received */
    USBD_EVENT_SET_ENDPOINT_HALT,
    /** Clear Feature ENDPOINT_HALT received */
    USBD_EVENT_CLR_ENDPOINT_HALT,

    /** Initial USB connection status */
    USBD_EVENT_UNKNOWN
};

enum usbd_status_type {
    USBD_OK,
    USBD_BUSY,
    USBD_FAIL,
    USBD_ERR_INVAILD,
    USBD_ERR_STATE,
    USBD_ERR_UNDRN,
    USBD_ERR_OVER,
    USBD_ERR_TIMEOUT,
};

/* Callback function for the USB Endpoint status */
typedef void (*usbd_endpoint_handler)(uint8_t ep);

/* Callback function for specific setup requests */
typedef uint8_t (*usbd_request_handler)(struct usb_setup_packet *setup,
                                    uint8_t **data, uint16_t *dlen);

/**
 * @brief USB Endpoint Structure.
 *
 * Structure containing the USB endpoint.
 */
typedef struct usbd_ep_tag {
    /** Endpoint Address: IN = 0x80|<ep_idx>, OUT = 0x00|<ep_idx> */
    uint8_t  ep_addr;
    /** Endpoint Transfer Type: Bulk, Interrupt, Control or Isochronous */
    uint8_t  ep_type;
    /** Endpoint max packet size <= USB_EP_MPS */
    uint16_t ep_mps;
    /** Endpoint status callback function */
    usbd_endpoint_handler ep_cb;
} usbd_ep_t;

#define USBD_EP_T(addr, type, mps, func)    \
            { .ep_addr=addr, .ep_type=type, .ep_mps=mps, .ep_cb=func }


/**
 * @brief USB Class Structure.
 *
 * Structure containing the USB class.
 */
typedef struct usbd_class_tag {
    /** Interface start number */
    uint8_t intf_start;
    /** Interface end number */
    uint8_t intf_end;
    /** Handler for USB Class specific commands */
    usbd_request_handler class_handler;
} usbd_class_t;

#define USBD_CLASS_T(istart, iend, ihandler)    \
            { .intf_start=istart, .intf_end=iend, .class_handler=ihandler }


/**
 * @brief USB Configuration Structure.
 *
 * Structure containing the USB configuration.
 */
typedef struct usbd_config_tag {
    uint8_t              conf_val;
    uint8_t              intf_cnt;
    uint8_t              class_cnt;
    uint8_t              ep_cnt;
    const usbd_class_t  *class_tab;
    const usbd_ep_t     *ep_tab;
} usbd_config_t;

#define USBD_CONFIG_T(_conf_val, _intf_cnt, classes, endpoints)   \
            { .conf_val=_conf_val, .intf_cnt=_intf_cnt,           \
              .class_tab=classes, .class_cnt=ARRAY_SIZE(classes), \
              .ep_tab=endpoints,  .ep_cnt=ARRAY_SIZE(endpoints) }


/*
 * FUNCTION DECLARATION
 ****************************************************************************************
 */

/**
 * @brief Init device controller.
 * @return N/A.
 */
void usbd_init(void);

/**
 * @brief Deinit device controller.
 * @return N/A.
 */
void usbd_deinit(void);

bool usbd_resume(bool en);
/**
 * @brief Register Device Description and Configuration.
 *
 * @param[in] desc  Pointer of description array
 * @param[in] conf  Pointer of configuration struct
 *
 * @return N/A.
 */
void usbd_register(const uint8_t *desc, const usbd_config_t *conf);

/**
 * @brief Device be configured or not.
 * @return 0 on not, other on conf_num
 */
uint8_t usbd_is_configured(void);

/**
 * @brief Write data to the specified endpoint with poll mode.
 *
 * This function is called to write data to the specified endpoint. The
 * supplied usbd_endpoint_handler function will be called when data is transmitted
 * out.
 *
 * @param[in]  ep        Endpoint address corresponding to the one
 *                       listed in the device configuration table
 * @param[in]  data      Pointer to data to write
 * @param[in]  data_len  Length of the data requested to write. This may
 *                       be zero for a zero length status packet.
 * @param[out] wr_bytes  Bytes scheduled for transmission. This value
 *                       may be NULL if the application expects all
 *                       bytes to be written
 *
 * @return 0 on success, errno code on fail.
 */
uint8_t usbd_ep_write(uint8_t ep, uint16_t data_len, const uint8_t *data, uint16_t *wr_bytes);

/**
 * @brief Read data from the specified endpoint
 *
 * This function is called by the endpoint handler function, after an OUT
 * interrupt has been received for that EP. The application must only call this
 * function through the supplied usbd_ep_callback function. This function clears
 * the ENDPOINT NAK when max_data_len is 0, if all data in the endpoint FIFO has been read,
 * so as to accept more data from host.
 *
 * @param[in]  ep        Endpoint address corresponding to the one
 *                       listed in the device configuration table
 * @param[in]  buff      Pointer to data buffer to write to
 * @param[in]  max_len   Max length of data to read
 *
 * @return Number of bytes read.
 */
uint16_t usbd_ep_read(uint8_t ep, uint16_t max_len, uint8_t *buff);

/**
 * @brief USB interrupt handler.
 * @return N/A.
 */
__USBIRQ void USB_IRQHandler(void);

/** Handler for Application to notify events */
/*
 * USB Event Notify Handler
 ****************************************************************************
 */
__USBIRQ void usbd_notify_handler(uint8_t event, void *arg);

/** Handler for USB Vendor specific commands */
__USBIRQ uint8_t usbd_vendor_handler(struct usb_setup_packet *setup, uint8_t **data, uint16_t *dlen);

#endif // _USBD_H_
