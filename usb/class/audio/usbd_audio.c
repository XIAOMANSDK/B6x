/**
 ****************************************************************************************
 *
 * @file usbd_audio.c
 *
 * @brief Function of USB Audio Device (UAC)
 *
 ****************************************************************************************
 */

#include "usbd.h"
#include "usbd_audio.h"


/*
 * DEFINE
 ****************************************************************************
 */

#if !defined(AUDIO_VOL_MIN)
#define AUDIO_VOL_MIN                   0xdb00 // -37.0dB
#define AUDIO_VOL_MAX                   0x0000 // 0db
#define AUDIO_VOL_RES                   0x0030 // 0.187db
#define AUDIO_VOL_CUR                   0xFE02 // -1.992dB
#endif


/*
 * Utils Function
 ****************************************************************************
 */

int audio_vol2db(uint16_t volume)
{
    int volume_db;

    if (volume < 0x8000) {
        volume_db = volume / 256;
    } else if (volume > 0x8000) {
        volume_db = (0xffff - volume + 1) / -256;
    }
    volume_db += 128; /* 0 ~ 255 */
    return volume_db;
}

uint16_t audio_db2vol(int volume_db)
{
    uint16_t volume;

    volume_db -= 128;
    if (volume_db >= 0) {
        volume = volume_db * 256;
    } else {
        volume = volume_db * 256 + 0xffff + 1;
    }
    return volume;
}


/*
 * __WEAK APIs
 ****************************************************************************
 */

/// Entity number convert to instance
__WEAK const audio_entity_t *usbd_audio_get_entity(uint8_t bEntityId)
{
    return NULL;
}

/// Mute SET/GET
__WEAK void usbd_audio_set_mute(uint8_t ep, uint8_t ch, bool mute)
{
    
}

__WEAK bool usbd_audio_get_mute(uint8_t ep, uint8_t ch)
{
    return 0;
}

/// Volume SET/GET
__WEAK void usbd_audio_set_volume(uint8_t ep, uint8_t ch, uint16_t volume)
{
    
}

__WEAK uint16_t usbd_audio_get_volume(uint8_t ep, uint8_t ch)
{
    return AUDIO_VOL_CUR;
}

/// Sampling Freq SET/GET
__WEAK void usbd_audio_set_sampling_freq(uint8_t ep, uint32_t sampling_freq)
{
    
}

__WEAK uint32_t usbd_audio_get_sampling_freq(uint8_t ep)
{
    return 0;
}

#if (AUDIO_ONCHANGE_EVENT)
/// Callback for SET_INTERFACE
__WEAK void usbd_audio_onchange_handler(uint8_t intf_num, uint8_t alt_setting)
{

}
#endif


/*
 * Request Handler
 ****************************************************************************
 */

static __inline void vol_write(uint8_t *data, uint16_t vol)
{
    data[0] = (uint8_t)(vol);
    data[1] = (uint8_t)(vol >> 8);
}

static __inline uint16_t vol_read(uint8_t *data)
{
    return ((uint16_t)data[0]) | ((uint16_t)data[1] << 8);
}

#if (CONFIG_USBD_AUDIO_VERSION < USBD_AUDIO_V2)

/******************************************/
/************ UAC v1.0 Handler ************/
/******************************************/

#if (AUDIO_SAMPLING_FREQ_CONTROL)
static __inline void freq_write(uint8_t *data, uint32_t freq)
{
    // 3B in v1.0
    data[0] = (uint8_t)(freq);
    data[1] = (uint8_t)(freq >> 8);
    data[2] = (uint8_t)(freq >> 16);
}

static __inline uint32_t freq_read(uint8_t *data)
{
    // 3B in v1.0
    return ((uint32_t)data[0] << 0) | ((uint32_t)data[1] << 8) | ((uint32_t)data[2] << 16);
}
#endif

static uint8_t audio_class_request_handler(struct usb_setup_packet *setup, uint8_t **data, uint16_t *len)
{
    uint8_t status = USBD_OK;
    uint8_t recip  = setup->bmRequestType & USB_REQUEST_RECIP_MASK;

    if (recip == USB_REQUEST_RECIP_INTERFACE) {
        uint8_t entity_id = HI_BYTE(setup->wIndex);
        const audio_entity_t *entity = usbd_audio_get_entity(entity_id);

        if (entity) {
            uint8_t chnl    = LO_BYTE(setup->wValue);
            uint8_t ctrlsel = HI_BYTE(setup->wValue);
            uint8_t ep      = entity->epAddr;
            uint8_t subtype = entity->bSubtype;

            USB_LOG_DBG("Audio_V1 entity_id:%02x, subtype:%02x, cs:%02x\r\n", entity_id, subtype, ctrlsel);
            switch (subtype) {
                case AUDIO_CONTROL_FEATURE_UNIT:
                {
                    switch (ctrlsel) {
                        case AUDIO_FU_CONTROL_MUTE:
                        {
                            switch (setup->bRequest) {
                                case AUDIO_REQUEST_SET_CUR:
                                {
                                    usbd_audio_set_mute(ep, chnl, (*data)[0]); // 1B
                                } break;

                                case AUDIO_REQUEST_GET_CUR:
                                {
                                    (*data)[0] = usbd_audio_get_mute(ep, chnl);
                                    *len = 1;
                                } break;

                                default:
                                {
                                    status = USBD_FAIL;
                                } break;
                            }
                        } break;

                        case AUDIO_FU_CONTROL_VOLUME:
                        {
                            switch (setup->bRequest) {
                                case AUDIO_REQUEST_SET_CUR:
                                {
                                    uint16_t volume = vol_read(*data); // 2B

                                    USB_LOG_DBG("Set ep:0x%02x ch:%d volume:0x%04x\r\n", ep, chnl, volume);
                                    usbd_audio_set_volume(ep, chnl, volume);
                                } break;

                                case AUDIO_REQUEST_GET_CUR:
                                {
                                    uint16_t volume = usbd_audio_get_volume(ep, chnl);

                                    vol_write(*data, volume);
                                    *len = 2;
                                } break;
                                case AUDIO_REQUEST_GET_MIN:
                                {
                                    vol_write(*data, AUDIO_VOL_MIN);
                                    *len = 2;
                                } break;
                                case AUDIO_REQUEST_GET_MAX:
                                {
                                    vol_write(*data, AUDIO_VOL_MAX);
                                    *len = 2;
                                } break;
                                case AUDIO_REQUEST_GET_RES:
                                {
                                    vol_write(*data, AUDIO_VOL_RES);
                                    *len = 2;
                                } break;

                                default:
                                {
                                    status = USBD_FAIL;
                                } break;
                            }
                        } break;

                        case AUDIO_FU_CONTROL_AGC:
                        {
                            if (setup->bRequest == AUDIO_REQUEST_GET_CUR) {
                                (*data)[0] = 0; // false - user modify
                                *len = 1;
                            } else {
                                status = USBD_FAIL;
                            }
                        } break;

                        default:
                        {
                            status = USBD_FAIL;
                        } break;
                    }
                } break;

                default:
                {
                    status = USBD_FAIL;
                } break;
            }

            if (status != USBD_OK) {
                USB_LOG_WRN("Unhandled Audio Class Interface(bReq:0x%02x, cs:0x%02x)\r\n", setup->bRequest, ctrlsel);
            }
        } else {
            USB_LOG_ERR("no Audio entity_id:%02x\r\n", entity_id);
            status = USBD_FAIL;
        }
    }
    #if (AUDIO_SAMPLING_FREQ_CONTROL)
    else if (recip == USB_REQUEST_RECIP_ENDPOINT)
    {
        uint8_t ep      = LO_BYTE(setup->wIndex);
        uint8_t ctrlsel = HI_BYTE(setup->wValue);

        if (ctrlsel == AUDIO_EP_CONTROL_SAMPLING_FEQ) {
            switch (setup->bRequest) {
                case AUDIO_REQUEST_SET_CUR:
                {
                    uint32_t sampling_freq = freq_read(*data); // 3B

                    USB_LOG_DBG("Set ep:0x%02x %d Hz\r\n", ep, (int)sampling_freq);
                    usbd_audio_set_sampling_freq(ep, sampling_freq);
                } break;

                case AUDIO_REQUEST_GET_CUR:
                case AUDIO_REQUEST_GET_MIN:
                case AUDIO_REQUEST_GET_MAX:
                case AUDIO_REQUEST_GET_RES:
                {
                    uint32_t sampling_freq = usbd_audio_get_sampling_freq(ep);

                    USB_LOG_DBG("Get ep:0x%02x %d Hz\r\n", ep, (int)sampling_freq);
                    freq_write(*data, sampling_freq); // 3B
                    *len = 3;
                } break;

                default:
                {
                    status = USBD_FAIL;
                } break;
            }
        } else {
            status = USBD_FAIL;
        }

        if (status != USBD_OK) {
            USB_LOG_WRN("Unhandled Audio Class Endpoint(bReq:0x%02x, cs:0x%02x)\r\n", setup->bRequest, ctrlsel);
        }
    }
    #endif //(AUDIO_SAMPLING_FREQ_CONTROL)
    else {
        status = USBD_FAIL;
    }

    return status;
}

#else //!(CONFIG_USBD_AUDIO_VERSION < USBD_AUDIO_V2)

/******************************************/
/************ UAC v1.0 Handler ************/
/******************************************/

const uint8_t demo_sampling_freq_table[] = {
    AUDIO_SAMPLE_FREQ_NUM(3),
    // 8K Sample
    AUDIO_SAMPLE_FREQ_4B(8000),
    AUDIO_SAMPLE_FREQ_4B(8000),
    AUDIO_SAMPLE_FREQ_4B(0x00),
    // 16K Sample
    AUDIO_SAMPLE_FREQ_4B(16000),
    AUDIO_SAMPLE_FREQ_4B(16000),
    AUDIO_SAMPLE_FREQ_4B(0x00),
    // 44.1K
    AUDIO_SAMPLE_FREQ_4B(44100),
    AUDIO_SAMPLE_FREQ_4B(44100),
    AUDIO_SAMPLE_FREQ_4B(0x00)
};

__WEAK const uint8_t* usbd_audio_get_sampling_freq_table(uint8_t ep)
{
    return demo_sampling_freq_table;
}

static __inline void freq_write(uint8_t *data, uint32_t freq)
{
    // 4B in v2.0
    data[0]=(uint8_t)(freq);
    data[1]=(uint8_t)((freq)>>8);
    data[2]=(uint8_t)((freq)>>16);
    data[3]=(uint8_t)((freq)>>24);
}

static __inline uint32_t freq_read(uint8_t *data)
{
    // 4B in v2.0
    return ((uint32_t)data[0] << 0 ) | ((uint32_t)data[1] << 8 ) 
         | ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
}

static uint8_t audio_class_request_handler(struct usb_setup_packet *setup, uint8_t **data, uint16_t *len)
{
    uint8_t status = USBD_OK;
    uint8_t recip  = setup->bmRequestType & USB_REQUEST_RECIP_MASK;

    if (recip == USB_REQUEST_RECIP_INTERFACE) {
        uint8_t entity_id = HI_BYTE(setup->wIndex);
        const audio_entity_t *entity = usbd_audio_get_entity(entity_id);

        if (entity) {
            uint8_t chnl    = LO_BYTE(setup->wValue);
            uint8_t ctrlsel = HI_BYTE(setup->wValue);
            uint8_t ep      = entity->epAddr;
            uint8_t subtype = entity->bSubtype;

            USB_LOG_DBG("Audio_V2 entity_id:%02x, subtype:%02x, cs:%02x\r\n", entity_id, subtype, ctrlsel);
            switch (subtype) {
                case AUDIO_CONTROL_FEATURE_UNIT:
                {
                    switch (ctrlsel) {
                        case AUDIO_FU_CONTROL_MUTE:
                        {
                            switch (setup->bRequest) {
                                case AUDIO_REQUEST_CUR:
                                {
                                    if (setup->bmRequestType & USB_REQUEST_DIR_MASK) {
                                        (*data)[0] = usbd_audio_get_mute(ep, chnl); // GET 1B
                                        *len = 1;
                                    } else {
                                        usbd_audio_set_mute(ep, chnl, (*data)[0]);  // SET 1B
                                    }
                                } break;

                                default:
                                {
                                    status = USBD_FAIL;
                                } break;
                            }
                        } break;

                        case AUDIO_FU_CONTROL_VOLUME:
                        {
                            switch (setup->bRequest) {
                                case AUDIO_REQUEST_CUR:
                                {
                                    if (setup->bmRequestType & USB_REQUEST_DIR_MASK) {
                                        uint16_t volume = usbd_audio_get_volume(ep, chnl);

                                        vol_write(*data, volume); // GET 2B
                                        *len = 2;
                                    } else {
                                        uint16_t volume = vol_read(*data); // SET 2B

                                        USB_LOG_DBG("Set ep:0x%02x ch:%d volume:0x%02x\r\n", ep, chnl, volume);
                                        usbd_audio_set_volume(ep, chnl, volume);
                                    }
                                } break;

                                case AUDIO_REQUEST_RANGE:
                                {
                                    if (setup->bmRequestType & USB_REQUEST_DIR_MASK) {
                                        vol_write(*data + 0, 1  );
                                        vol_write(*data + 2, 0  );
                                        vol_write(*data + 4, 100);
                                        vol_write(*data + 6, 1  );
                                        *len = 8;
                                    } else {
                                        status = USBD_FAIL;
                                    }
                                } break;

                                default:
                                {
                                    status = USBD_FAIL;
                                } break;
                            }
                        } break;

                        default:
                        {
                            status = USBD_FAIL;
                        } break;
                    }
                } break;

                case AUDIO_CONTROL_CLOCK_SOURCE:
                {
                    switch (ctrlsel) {
                        case AUDIO_CS_CONTROL_SAM_FREQ:
                        {
                            switch (setup->bRequest) {
                                case AUDIO_REQUEST_CUR:
                                {
                                    if (setup->bmRequestType & USB_REQUEST_DIR_MASK) {
                                        uint32_t sampling_freq = usbd_audio_get_sampling_freq(ep);

                                        USB_LOG_DBG("Get ep:0x%02x %d Hz\r\n", ep, (int)sampling_freq);
                                        freq_write(*data, sampling_freq); // GET 4B
                                        *len = 4;
                                    } else {
                                        uint32_t sampling_freq = freq_read(*data); // SET 4B

                                        USB_LOG_DBG("Set ep:0x%02x %d Hz\r\n", ep, (int)sampling_freq);
                                        usbd_audio_set_sampling_freq(ep, sampling_freq);
                                    }
                                } break;

                                case AUDIO_REQUEST_RANGE:
                                {
                                    if (setup->bmRequestType & USB_REQUEST_DIR_MASK) {
                                        const uint8_t *sampling_freq_table = usbd_audio_get_sampling_freq_table(ep);

                                        if (sampling_freq_table) {
                                            uint16_t num = (uint16_t)((uint16_t)(sampling_freq_table[1] << 8) | ((uint16_t)sampling_freq_table[0]));

                                            memcpy(*data, sampling_freq_table, (12 * num + 2));
                                            *len = (12 * num + 2);
                                        } else {
                                            status = USBD_FAIL;
                                        }
                                    } else {
                                        status = USBD_FAIL;
                                    }
                                } break;

                                default:
                                {
                                    status = USBD_FAIL;
                                } break;
                            }
                        } break;

                        case AUDIO_CS_CONTROL_CLOCK_VALID:
                        {
                            if (setup->bmRequestType & USB_REQUEST_DIR_MASK) {
                                (*data)[0] = 1; // GET 1B
                                *len = 1;
                            } else {
                                status = USBD_FAIL;
                            }
                        } break;

                        default:
                        {
                            status = USBD_FAIL;
                        } break;
                    }
                } break;

                default:
                {
                    status = USBD_FAIL;
                } break;
            }
            
            if (status != USBD_OK) {
                USB_LOG_WRN("Unhandled Audio Class bRequest 0x%02x in cs 0x%02x\r\n", setup->bRequest, ctrlsel);
            }
        } else {
            USB_LOG_ERR("no Audio entity_id:%02x\r\n", entity_id);
            status = USBD_FAIL;
        }
    
        return status;
    }

    return USBD_FAIL;
}
#endif

/**
 * @brief Handler called for Class requests.
 *
 * @param setup    Information about the request to execute.
 * @param data     Buffer containing the request result.
 * @param len      Size of the buffer.
 *
 * @return  0 on success, errno code on fail.
 */
uint8_t usbd_audio_class_handler(struct usb_setup_packet *setup, uint8_t **data, uint16_t *len)
{
    // Standard Request -  callback of USB_REQUEST_SET_INTERFACE
    if ((setup->bmRequestType & USB_REQUEST_TYPE_MASK) == USB_REQUEST_STANDARD) {
        #if (AUDIO_ONCHANGE_EVENT)
        if (setup->bRequest == USB_REQUEST_SET_INTERFACE) {
            usbd_audio_onchange_handler(setup->wIndex, setup->wValue);
        }
        #endif

        return USBD_FAIL; // Continue to Standard Handler
    }

    // Audio Class Request
    USB_LOG_DBG("AUDIO Class(bTyp:%X,bReq:%X,wVal:%X,wIdx:%X)\r\n", 
                    setup->bmRequestType, setup->bRequest, setup->wValue, setup->wIndex);

    return audio_class_request_handler(setup, data, len);
}
