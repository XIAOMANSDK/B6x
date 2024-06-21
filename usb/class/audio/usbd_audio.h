/**
 ****************************************************************************************
 *
 * @file usbd_audio.h
 *
 * @brief Header file of USB Audio function
 *
 ****************************************************************************************
 */
#ifndef _USBD_AUDIO_H_
#define _USBD_AUDIO_H_

#include "usb_audio.h"

#define USBD_AUDIO_V1                   0x0100 // V1.0
#define USBD_AUDIO_V2                   0x0200 // v2.0

#if !defined(CONFIG_USBD_AUDIO_VERSION)
#define CONFIG_USBD_AUDIO_VERSION       USBD_AUDIO_V1 
#endif

#if !defined(AUDIO_SAMPLING_FREQ_CONTROL)
#define AUDIO_SAMPLING_FREQ_CONTROL     (0)
#endif

#if !defined(AUDIO_ONCHANGE_EVENT)
#define AUDIO_ONCHANGE_EVENT            (1)
#endif

typedef struct audio_entity_info {
    uint8_t bSubtype;
    uint8_t bEntityId;
    uint8_t epAddr;
} audio_entity_t;

#define AUDIO_ENTITY_T(ep, entity_id, subtype)      \
            { .epAddr=ep, .bEntityId=entity_id, .bSubtype=subtype }


/*
 * __WEAK APIs
 ****************************************************************************
 */

/// Audio Entity Retrieve
const audio_entity_t *usbd_audio_get_entity(uint8_t bEntityId);

/// Mute SET/GET
void usbd_audio_set_mute(uint8_t ep, uint8_t ch, bool mute);

bool usbd_audio_get_mute(uint8_t ep, uint8_t ch);

/// Volume SET/GET
void usbd_audio_set_volume(uint8_t ep, uint8_t ch, uint16_t volume);

uint16_t usbd_audio_get_volume(uint8_t ep, uint8_t ch);

/// Sampling Freq SET/GET
void usbd_audio_set_sampling_freq(uint8_t ep, uint32_t sampling_freq);

uint32_t usbd_audio_get_sampling_freq(uint8_t ep);

#if (CONFIG_USBD_AUDIO_VERSION >= USBD_AUDIO_V2)
const uint8_t *usbd_audio_get_sampling_freq_table(uint8_t ep);
#endif

#if (AUDIO_ONCHANGE_EVENT)
/// Callback for SET_INTERFACE
void usbd_audio_onchange_handler(uint8_t intf_num, uint8_t alt_setting);
#endif


/*
 * Request Handler
 ****************************************************************************
 */

uint8_t usbd_audio_class_handler(struct usb_setup_packet *setup, uint8_t **data, uint16_t *len);


#endif /* _USB_AUDIO_H_ */
