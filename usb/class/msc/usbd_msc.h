/**
 ****************************************************************************************
 *
 * @file usbd_msc.h
 *
 * @brief Header file of USB Mass Storage Class function
 *
 ****************************************************************************************
 */
#ifndef USBD_MSC_H
#define USBD_MSC_H

#include "usb_msc.h"

/* Default Config */
#ifndef CONFIG_USBD_MSC_MAX_LUN
#define CONFIG_USBD_MSC_MAX_LUN         (1)
#endif

#ifndef CONFIG_USBD_MSC_MAX_BUFSIZE
#define CONFIG_USBD_MSC_MAX_BUFSIZE     (4096)
#endif

#ifndef CONFIG_USBD_MSC_MANUF_STR
#define CONFIG_USBD_MSC_MANUF_STR       "XM"
#endif

#ifndef CONFIG_USBD_MSC_PRODUCT_STR
#define CONFIG_USBD_MSC_PRODUCT_STR     "B6x-MSC"
#endif

#ifndef CONFIG_USBD_MSC_VERSION_STR
#define CONFIG_USBD_MSC_VERSION_STR     "0.01"
#endif

/* Max packet size */
#define MSC_INT_EP_MPS                  (0x0040)
#define MSC_BULK_EP_MPS                 (0x0040)


/*
 * Driver Function
 ****************************************************************************
 */

void usbd_msc_init(const uint8_t out_ep, const uint8_t in_ep);

void usbd_msc_reset(void);

bool usbd_msc_get_popup(void);

void usbd_msc_set_readonly(bool readonly);

/*
 * Request Handler
 ****************************************************************************
 */

uint8_t usbd_msc_class_handler(struct usb_setup_packet *setup, uint8_t **data, uint16_t *len);

void usbd_msc_bulk_in_handler(uint8_t ep);

void usbd_msc_bulk_out_handler(uint8_t ep);

/*
 * Flash Callback
 ****************************************************************************
 */

void usbd_msc_get_cap(uint8_t lun, uint32_t *block_num, uint32_t *block_size);

bool usbd_msc_sector_read(uint8_t lun, uint32_t sector, uint8_t *buffer, uint32_t length);

bool usbd_msc_sector_write(uint8_t lun, uint32_t sector, const uint8_t *data, uint32_t length);

#endif /* USBD_MSC_H */
