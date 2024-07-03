#include "usbd.h"
#include "usbd_audio.h"
#include "drvs.h"

#if (DEMO_AUDIO_MIC)

#define USBD_VID                    0x0D8C
#define USBD_PID                    0x0312
#define USBD_BCD                    0x0306
#define USBD_MAX_POWER              100
#define USBD_LANGID_STRING          0x0409 // English(US)


/* AUDIO Class Config */
#define AUDIO_IN_EP                 0x81
#define AUDIO_IN_EP_INTV            0x01   // unit in 1ms

#define AUDIO_IN_FREQ               8000U // 16K
#define AUDIO_IN_FRAME_SIZE         2      // unit in byte
#define AUDIO_IN_RESOL_BITS         16     // unit in bit
#define AUDIO_IN_CHNLS              1      // Mono:1

/// Packet Size = AudioFreq * DataSize (16bit: 2) * NumChannels(Mono: 1) / 1000ms
#define AUDIO_IN_EP_MPS             ((uint32_t)((AUDIO_IN_FREQ * AUDIO_IN_FRAME_SIZE * AUDIO_IN_CHNLS) / 1000))

/// Mono: 1, Stereo: 2
#if AUDIO_IN_CHNLS == 1
#define AUDIO_INPUT_CTRL            0x03, 0x03
#define AUDIO_INPUT_CHEN            0x0000
#elif AUDIO_IN_CHNLS == 2
#define AUDIO_INPUT_CTRL            0x03, 0x03, 0x03
#define AUDIO_INPUT_CHEN            0x0003
#endif

enum audio_id {
    AUDIO_UNDEFINED_ID              = 0,

    AUDIO_IN_TERM_ID,
    AUDIO_IN_FEAT_ID,
    AUDIO_OUT_TERM_ID,
};


/*!< count of interface descriptor */
enum intf_num {
    /* Audio class interface */
    AUDIO_AC_INTF_NUM               = 0,
    AUDIO_AS_INTF_NUM,

    /* total interface count */
    USB_CONFIG_INTF_CNT,

    /* start&end interface */
    USB_AUDIO_INTF_START            = AUDIO_AC_INTF_NUM,
    USB_AUDIO_INTF_END              = AUDIO_AS_INTF_NUM,
};


/*!< Length of Configure descriptor */
/// Audio Desc Size (Audio Control & Audio Stream)
#define AUDIO_AC_CTRL_SIZE          ( AUDIO_SIZEOF_AC_HEADER_DESC(1) +                          \
                                      AUDIO_SIZEOF_AC_INPUT_TERMINAL_DESC +                     \
                                      AUDIO_SIZEOF_AC_OUTPUT_TERMINAL_DESC +                    \
                                      AUDIO_SIZEOF_AC_FEATURE_UNIT_DESC(AUDIO_IN_CHNLS, 1) )

#define AUDIO_AC_DESC_SIZE          ( USB_INTERFACE_DESC_SIZE + AUDIO_AC_CTRL_SIZE )

#define AUDIO_AS_DESC_SIZE          ( AUDIO_AS_DESCRIPTOR_INIT_LEN(1) )

#define AUDIO_AC_STRING_INDEX       (0) // none

/// Total DescSize
#define USB_CONFIG_TOTAL_SIZE       ( USB_CONFIG_DESC_SIZE                      \
                                    + AUDIO_AC_DESC_SIZE + AUDIO_AS_DESC_SIZE )


/*!< USB device descriptor */
const uint8_t audio_descriptor[] = {
    /* Descriptor - Device (Size:18) */
    USB_DEVICE_DESCRIPTOR_INIT(USB_1_1, 0x00, 0x00, 0x00, USBD_VID, USBD_PID, USBD_BCD, 0x01),
    
    /* Descriptor - Configuration (Total Size:9+Intf_Size) */
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_TOTAL_SIZE, USB_CONFIG_INTF_CNT, 
            0x01, USB_CONFIG_BUS_POWERED | USB_CONFIG_REMOTE_WAKEUP, USBD_MAX_POWER),
    
    /* Descriptor - Audio Control */
    AUDIO_AC_HEADER_INIT(AUDIO_AC_INTF_NUM, AUDIO_AC_CTRL_SIZE, AUDIO_AC_STRING_INDEX, AUDIO_AS_INTF_NUM),

    AUDIO_AC_INPUT_TERMINAL_DESCRIPTOR_INIT(AUDIO_IN_TERM_ID, AUDIO_INTERM_MIC, AUDIO_IN_CHNLS, AUDIO_INPUT_CHEN),
    AUDIO_AC_FEATURE_UNIT_DESCRIPTOR_INIT(AUDIO_IN_FEAT_ID, AUDIO_IN_TERM_ID, 0x01, AUDIO_INPUT_CTRL),
    AUDIO_AC_OUTPUT_TERMINAL_DESCRIPTOR_INIT(AUDIO_OUT_TERM_ID, AUDIO_TERMINAL_STREAMING, 0x02, AUDIO_IN_FEAT_ID),

    /* Descriptor - Audio Stream */
    AUDIO_AS_DESCRIPTOR_INIT(AUDIO_AS_INTF_NUM, AUDIO_OUT_TERM_ID, AUDIO_IN_CHNLS, AUDIO_IN_FRAME_SIZE, AUDIO_IN_RESOL_BITS, 
                                AUDIO_IN_EP, 0x05, AUDIO_IN_EP_MPS, AUDIO_IN_EP_INTV, AUDIO_SAMPLE_FREQ_3B(AUDIO_IN_FREQ)),

    ///////////////////////////////////////
    /// string0 descriptor
    ///////////////////////////////////////
    USB_LANGID_INIT(USBD_LANGID_STRING),

    // String1 - iManufacturer
    0x10,                       /* bLength */
    USB_DESC_TYPE_STRING,       /* bDescriptorType */
    WCHAR('X'),                 /* wcChar0 */
    WCHAR('M'),                 /* wcChar1 */
    WCHAR('-'),                 /* wcChar2 */
    WCHAR('U'),                 /* wcChar3 */
    WCHAR('S'),                 /* wcChar4 */
    WCHAR('B'),                 /* wcChar5 */
    WCHAR('D'),                 /* wcChar6 */

    // String2 - iProduct
    0x12,                       /* bLength */
    USB_DESC_TYPE_STRING, /* bDescriptorType */
    WCHAR('U'),                 /* wcChar0 */
    WCHAR('A'),                 /* wcChar1 */
    WCHAR('C'),                 /* wcChar2 */
    WCHAR(' '),                 /* wcChar3 */
    WCHAR('D'),                 /* wcChar4 */
    WCHAR('E'),                 /* wcChar5 */
    WCHAR('M'),                 /* wcChar6 */
    WCHAR('O'),                 /* wcChar7 */

//    // String3 - iSerialNumber
//    0x02,                       /* bLength */
//    USB_DESC_TYPE_STRING, /* bDescriptorType */

    /* Descriptor - Device Qualifier (Size:10) */
    #if (USBD_BCD == USB_2_0)
    USB_QUALIFIER_INIT(0x01),
    #endif

    0x00
};


/*
 * Configuration
 ****************************************************************************
 */

extern void usbd_audio_ep_in_handler(uint8_t ep);

/*!< table of Audio entity */
static const audio_entity_t audio_entity_tab[] = {
    AUDIO_ENTITY_T(AUDIO_IN_EP, AUDIO_IN_FEAT_ID, AUDIO_CONTROL_FEATURE_UNIT),
};

/*!< table of endpoints */
static const usbd_ep_t endpoint_tab[] = {
    USBD_EP_T(AUDIO_IN_EP, USB_EP_TYPE_ISOCHRONOUS, AUDIO_IN_EP_MPS, &usbd_audio_ep_in_handler),
};

/*!< table of class */
static const usbd_class_t class_tab[] = {
    USBD_CLASS_T(USB_AUDIO_INTF_START, USB_AUDIO_INTF_END, &usbd_audio_class_handler),
};

/*!< USBD Configuration */
static const usbd_config_t audio_configuration[] = {
    USBD_CONFIG_T(1, USB_CONFIG_INTF_CNT, class_tab, endpoint_tab)
};


/*
 * Handlers
 ****************************************************************************
 */

enum mic_state_tag {
    MIC_OFF,
    MIC_IDLE, // on
    MIC_BUSY,
};

volatile uint8_t mic_state = MIC_OFF;

/// Audio Entity Retrieve
const audio_entity_t *usbd_audio_get_entity(uint8_t bEntityId)
{
    const audio_entity_t *entity = NULL;

    if (bEntityId == AUDIO_IN_FEAT_ID) {
        entity = &audio_entity_tab[0];
    }
    return entity;
}

/// Callback for SET_INTERFACE
void usbd_audio_onchange_handler(uint8_t intf_num, uint8_t alt_setting)
{
    if (intf_num == AUDIO_AS_INTF_NUM) {
        if (alt_setting == 1) {
            mic_state = MIC_IDLE;
            USB_LOG_RAW("Mic On\r\n");
        } else {
            mic_state = MIC_OFF;
            USB_LOG_RAW("Mic Off\r\n");
        }
    }
}

/// Callback for Mic endpoint
void usbd_audio_ep_in_handler(uint8_t ep)
{
    if (mic_state == MIC_BUSY) {
        mic_state = MIC_IDLE;
    }
    //USB_LOG_RAW("ep_in:0x%x\r\n", ep);
}

/*
 * Test
 ****************************************************************************
 */

static uint8_t mic_tmp = 0;
static uint8_t mic_buffer[AUDIO_IN_EP_MPS];

void usbdInit()
{
    // enable USB clk and iopad
    rcc_usb_en();

    usbd_init();
    usbd_register(audio_descriptor, audio_configuration);

    NVIC_EnableIRQ(USB_IRQn);
}

//void usbdTest()
//{
//    if (usbd_is_configured()) {
//        if (mic_state == MIC_IDLE) {
//            
//            uint8_t status = 0;
//            memset(mic_buffer, mic_tmp, AUDIO_IN_EP_MPS);
//            
//            GPIO_DIR_SET_HI(GPIO16);
//            mic_state = MIC_BUSY;
//            status = usbd_ep_write(AUDIO_IN_EP, AUDIO_IN_EP_MPS, mic_buffer, NULL);

//            if (status != USBD_OK) {
//                if (mic_state != MIC_OFF) {
//                    mic_state = MIC_IDLE;
//                }
//                USB_LOG_RAW("err:%d, tmp:%02X\r\n", status, mic_tmp);
//            } else {
//                USB_LOG_RAW("curr tmp:%02X\r\n", mic_tmp);
//            }
//            GPIO_DIR_SET_LO(GPIO16);
//            
//            mic_tmp++;
//        }
//    }
//}

#include "micphone.h"
bool mic_flag = false;

void usbdTest()
{
    uint8_t *ptr = micDataGet();

    if (ptr != NULL)
    {
        mic_flag = true;
        memcpy(mic_buffer, ptr, AUDIO_IN_EP_MPS);  // PCM_SAMPLE_NB = 8;
    }
    
    if (usbd_is_configured()) {
        if ((mic_state == MIC_IDLE) && mic_flag) {
            
            uint8_t status = 0;
//            memset(mic_buffer, mic_tmp, AUDIO_IN_EP_MPS);
            
            GPIO_DIR_SET_HI(GPIO16);
            mic_state = MIC_BUSY;
            status = usbd_ep_write(AUDIO_IN_EP, AUDIO_IN_EP_MPS, mic_buffer, NULL);

            if (status != USBD_OK) {
                if (mic_state != MIC_OFF) {
                    mic_state = MIC_IDLE;
                }
                USB_LOG_RAW("err:%d, tmp:%02X\r\n", status, mic_tmp);
            } else {
                USB_LOG_RAW("curr tmp:%02X\r\n", mic_tmp);
            }
            GPIO_DIR_SET_LO(GPIO16);
            
            mic_tmp++;
            
            mic_flag = false;
        }
    }
}
#endif
