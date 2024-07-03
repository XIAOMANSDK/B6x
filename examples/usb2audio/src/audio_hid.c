#include "usbd.h"
#include "usbd_hid.h"
#include "usbd_audio.h"
#include "drvs.h"

#if (DEMO_AUDIO_HID)

#define USBD_VID                    0x0D8C
#define USBD_PID                    0x0312
#define USBD_BCD                    0x0306
#define USBD_MAX_POWER              100
#define USBD_LANGID_STRING          0x0409 // English(US)


/*
 * Audio DEFINES
 ****************************************************************************
 */

#define AUDIO_IN_EP                 0x83
#define AUDIO_IN_EP_INTV            0x01   // unit in 1ms

#define AUDIO_IN_FREQ               8000U // 16K
#define AUDIO_IN_FRAME_SIZE         2      // unit in byte
#define AUDIO_IN_RESOL_BITS         16     // unit in bit
#define AUDIO_IN_CHNLS              1      // Mono:1

/// Packet Size = AudioFreq * DataSize (16bit: 2) * NumChannels(Mono: 1) / 1000ms
#define AUDIO_IN_EP_MPS             ((uint32_t)((AUDIO_IN_FREQ * AUDIO_IN_FRAME_SIZE * AUDIO_IN_CHNLS) / 1000))

/// Mono: 1, Stereo: 2
#define AUDIO_INPUT_CTRL            0x43, 0x00
#define AUDIO_INPUT_CHEN            0x0000

enum audio_id {
    AUDIO_UNDEFINED_ID              = 0,

    AUDIO_IN_TERM_ID                = 4,
    AUDIO_IN_FEAT_ID                = 5,
    AUDIO_OUT_TERM_ID               = 6,
    AUDIO_OUT_SELTR_ID              = 7,
};


/*
 * HID DEFINES
 ****************************************************************************
 */

/*!< standard interface config */
#define HID_STD_IN_EP               0x82 // address
#define HID_STD_IN_EP_MPS           16   // max packet size
#define HID_STD_IN_EP_INTV          1    // polling time in ms
#define HID_STD_REPORT_DESC_SIZE    sizeof(hid_std_report_desc)

/*!< custom-raw interface config */
#define HID_RAW_IN_EP               0x84
#define HID_RAW_IN_EP_MPS           64
#define HID_RAW_IN_EP_INTV          1
#define HID_RAW_OUT_EP              0x04
#define HID_RAW_OUT_EP_MPS          64
#define HID_RAW_OUT_EP_INTV         1 
#define HID_RAW_REPORT_DESC_SIZE    sizeof(hid_raw_report_desc)

static const uint8_t hid_std_report_desc[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x02,        // Usage (Mouse)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x01,        //   Report ID (1)
    0x09, 0x01,        //   Usage (Pointer)
    0xA1, 0x00,        //   Collection (Physical)
    0x95, 0x05,        //     Report Count (5)
    0x75, 0x01,        //     Report Size (1)
    0x05, 0x09,        //     Usage Page (Button)
    0x19, 0x01,        //     Usage Minimum (0x01)
    0x29, 0x05,        //     Usage Maximum (0x05)
    0x15, 0x00,        //     Logical Minimum (0)
    0x25, 0x01,        //     Logical Maximum (1)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x01,        //     Report Count (1)
    0x75, 0x03,        //     Report Size (3)
    0x81, 0x01,        //     Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x75, 0x08,        //     Report Size (8)
    0x95, 0x01,        //     Report Count (1)
    0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
    0x09, 0x38,        //     Usage (Wheel)
    0x15, 0x81,        //     Logical Minimum (-127)
    0x25, 0x7F,        //     Logical Maximum (127)
    0x81, 0x06,        //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x0C,        //     Usage Page (Consumer)
    0x0A, 0x38, 0x02,  //     Usage (AC Pan)
    0x95, 0x01,        //     Report Count (1)
    0x81, 0x06,        //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              //   End Collection
    0x85, 0x02,        //   Report ID (2)
    0x09, 0x01,        //   Usage (Consumer Control)
    0xA1, 0x00,        //   Collection (Physical)
    0x75, 0x0C,        //     Report Size (12)
    0x95, 0x02,        //     Report Count (2)
    0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
    0x09, 0x30,        //     Usage (X)
    0x09, 0x31,        //     Usage (Y)
    0x16, 0x01, 0xF8,  //     Logical Minimum (-2047)
    0x26, 0xFF, 0x07,  //     Logical Maximum (2047)
    0x81, 0x06,        //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              //   End Collection
    0xC0,              // End Collection
    0x05, 0x0C,        // Usage Page (Consumer)
    0x09, 0x01,        // Usage (Consumer Control)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x04,        //   Report ID (4)
    0x75, 0x10,        //   Report Size (16)
    0x95, 0x01,        //   Report Count (1)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0x8C, 0x02,  //   Logical Maximum (652)
    0x19, 0x00,        //   Usage Minimum (Unassigned)
    0x2A, 0x8C, 0x02,  //   Usage Maximum (AC Send)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              // End Collection
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x06,        // Usage (Keyboard)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x05,        //   Report ID (5)
    0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
    0x95, 0x06,        //   Report Count (6)
    0x75, 0x08,        //   Report Size (8)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
    0x19, 0x00,        //   Usage Minimum (0x00)
    0x2A, 0xFF, 0x00,  //   Usage Maximum (0xFF)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              // End Collection

    // 138 bytes
};

static const uint8_t hid_raw_report_desc[] = {
    0x06, 0x02, 0xFF,  // Usage Page (Vendor Defined 0xFF02)
    0x09, 0x02,        // Usage (0x02)
    0xA1, 0x01,        // Collection (Application)
    0x09, 0x03,        //   Usage (0x03)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x40,        //   Report Count (64)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0x04,        //   Usage (0x04)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x40,        //   Report Count (64)
    0x91, 0x00,        //   Output (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,              // End Collection

    // 34 bytes
};


/*
 * Device Descriptor
 ****************************************************************************
 */

/*!< count of interface descriptor */
enum intf_num {
    /* Audio class interface */
    AUDIO_AC_INTF_NUM               = 0,
    AUDIO_AS_INTF_NUM,

    /* HID class interface */
    HID_STD_INTF_NUM,
    HID_RAW_INTF_NUM,

    /* total interface count */
    USB_CONFIG_INTF_CNT,

    /* start&end interface */
    USB_AUDIO_INTF_START            = AUDIO_AC_INTF_NUM,
    USB_AUDIO_INTF_END              = AUDIO_AS_INTF_NUM,

    USB_HID_INTF_START              = HID_STD_INTF_NUM,
    USB_HID_INTF_END                = HID_RAW_INTF_NUM,
};


/*!< Length of Configure descriptor */
/// Audio Desc Size (Audio Control & Audio Stream)
#define AUDIO_AC_CTRL_SIZE          ( AUDIO_SIZEOF_AC_HEADER_DESC(1) +                          \
                                      AUDIO_SIZEOF_AC_INPUT_TERMINAL_DESC +                     \
                                      AUDIO_SIZEOF_AC_OUTPUT_TERMINAL_DESC +                    \
                                      /*AUDIO_SIZEOF_AC_SELECTOR_UNIT_DESC +*/                      \
                                      AUDIO_SIZEOF_AC_FEATURE_UNIT_DESC(AUDIO_IN_CHNLS, 1) )

#define AUDIO_AC_DESC_SIZE          ( USB_INTERFACE_DESC_SIZE + AUDIO_SIZEOF_AC_SELECTOR_UNIT_DESC + AUDIO_AC_CTRL_SIZE )

#define AUDIO_AS_DESC_SIZE          ( AUDIO_AS_DESCRIPTOR_INIT_LEN(1) )

#define AUDIO_AC_STRING_INDEX       (0)

/// HID Desc Size
#define HID_STD_DESC_SIZE           ( 18 + 7*1 ) // 1 EP: IN
#define HID_RAW_DESC_SIZE           ( 18 + 7*2 ) // 2 EP: IN and OUT

/// Total Desc Size
#define USB_CONFIG_TOTAL_SIZE       ( USB_CONFIG_DESC_SIZE                     \
                                    + AUDIO_AC_DESC_SIZE + AUDIO_AS_DESC_SIZE  \
                                    + HID_STD_DESC_SIZE + HID_RAW_DESC_SIZE )


/*!< USB device descriptor */
const uint8_t usb_descriptor[] = {
    /* Descriptor - Device (Size:18) */
    USB_DEVICE_DESCRIPTOR_INIT(USB_1_1, 0x00, 0x00, 0x00, USBD_VID, USBD_PID, USBD_BCD, 0x01),
    
    /* Descriptor - Configuration (Total Size:9+Intf_Size) */
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_TOTAL_SIZE, USB_CONFIG_INTF_CNT, 
            0x01, USB_CONFIG_BUS_POWERED | USB_CONFIG_REMOTE_WAKEUP, USBD_MAX_POWER),
    
    /* Descriptor - Audio Control */
    AUDIO_AC_HEADER_INIT(AUDIO_AC_INTF_NUM, AUDIO_AC_CTRL_SIZE, AUDIO_AC_STRING_INDEX, AUDIO_AS_INTF_NUM),
    AUDIO_AC_INPUT_TERMINAL_DESCRIPTOR_INIT(AUDIO_IN_TERM_ID, AUDIO_INTERM_MIC, AUDIO_IN_CHNLS, AUDIO_INPUT_CHEN),
    AUDIO_AC_OUTPUT_TERMINAL_DESCRIPTOR_INIT(AUDIO_OUT_TERM_ID, AUDIO_TERMINAL_STREAMING, 0x02, AUDIO_OUT_SELTR_ID),
    AUDIO_AC_SELECTOR_UNIT_DESCRIPTOR_INIT(AUDIO_OUT_SELTR_ID, AUDIO_IN_FEAT_ID, 0x01),
    AUDIO_AC_FEATURE_UNIT_DESCRIPTOR_INIT(AUDIO_IN_FEAT_ID, AUDIO_IN_TERM_ID, 0x01, AUDIO_INPUT_CTRL),

    /* Descriptor - Audio Stream */
    AUDIO_AS_DESCRIPTOR_INIT(AUDIO_AS_INTF_NUM, AUDIO_OUT_TERM_ID, AUDIO_IN_CHNLS, AUDIO_IN_FRAME_SIZE, AUDIO_IN_RESOL_BITS, 
                                AUDIO_IN_EP, 0x05, AUDIO_IN_EP_MPS, AUDIO_IN_EP_INTV, AUDIO_SAMPLE_FREQ_3B(AUDIO_IN_FREQ)),

    /* Descriptor - Keyboard Interface (Size:18+7*1) */
    HID_INTERFACE_INIT1(HID_STD_INTF_NUM, 1, HID_SUBCLASS_NONE, HID_PROTOCOL_BOOT, 0, HID_STD_REPORT_DESC_SIZE, HID_BCD_0201),
    HID_ENDPOINT_DESC(HID_STD_IN_EP, HID_STD_IN_EP_MPS, HID_STD_IN_EP_INTV),
    
    /* Descriptor - Custom-Raw Interface (Size:18+7*2) */
    HID_INTERFACE_INIT1(HID_RAW_INTF_NUM, 2, HID_SUBCLASS_NONE, HID_PROTOCOL_NONE, 0, HID_RAW_REPORT_DESC_SIZE, HID_BCD_0201),
    HID_ENDPOINT_DESC(HID_RAW_IN_EP, HID_RAW_IN_EP_MPS, HID_RAW_IN_EP_INTV),
    HID_ENDPOINT_DESC(HID_RAW_OUT_EP, HID_RAW_OUT_EP_MPS, HID_RAW_OUT_EP_INTV),

    ///////////////////////////////////////
    /// string0 descriptor
    ///////////////////////////////////////
    USB_LANGID_INIT(USBD_LANGID_STRING),

    // String1 - iManufacturer
    0x2A,                       /* bLength */
    USB_DESC_TYPE_STRING,       /* bDescriptorType */
    WCHAR('M'),                 /* wcChar0 */
    WCHAR('e'),                 /* wcChar1 */
    WCHAR('i'),                 /* wcChar2 */
    WCHAR(' '),                 /* wcChar3 */
    WCHAR('S'),                 /* wcChar4 */
    WCHAR('h'),                 /* wcChar5 */
    WCHAR('e'),                 /* wcChar6 */
    WCHAR('n'),                 /* wcChar7 */
    WCHAR('g'),                 /* wcChar8 */
    WCHAR(' '),                 /* wcChar9 */
    WCHAR('T'),                 /* wcChar10 */
    WCHAR('e'),                 /* wcChar11 */
    WCHAR('c'),                 /* wcChar12 */
    WCHAR('h'),                 /* wcChar13 */
    WCHAR('n'),                 /* wcChar14 */
    WCHAR('o'),                 /* wcChar15 */
    WCHAR('l'),                 /* wcChar16 */
    WCHAR('o'),                 /* wcChar17 */
    WCHAR('g'),                 /* wcChar18 */
    WCHAR('y'),                 /* wcChar19 */

    // String2 - iProduct
    0x2A,                       /* bLength */
    USB_DESC_TYPE_STRING,       /* bDescriptorType */
    WCHAR('U'),                 /* wcChar0 */
    WCHAR('S'),                 /* wcChar1 */
    WCHAR('B'),                 /* wcChar2 */
    WCHAR(' '),                 /* wcChar3 */
    WCHAR('C'),                 /* wcChar4 */
    WCHAR('o'),                 /* wcChar5 */
    WCHAR('m'),                 /* wcChar6 */
    WCHAR('p'),                 /* wcChar7 */
    WCHAR('o'),                 /* wcChar8 */
    WCHAR('s'),                 /* wcChar9 */
    WCHAR('i'),                 /* wcChar10 */
    WCHAR('t'),                 /* wcChar11 */
    WCHAR('e'),                 /* wcChar12 */
    WCHAR(' '),                 /* wcChar13 */
    WCHAR('D'),                 /* wcChar14 */
    WCHAR('e'),                 /* wcChar15 */
    WCHAR('v'),                 /* wcChar16 */
    WCHAR('i'),                 /* wcChar17 */
    WCHAR('c'),                 /* wcChar18 */
    WCHAR('e'),                 /* wcChar19 */

//    // String3 - iSerialNumber
//    0x02,                       /* bLength */
//    USB_DESC_TYPE_STRING, /* bDescriptorType */

    /* Descriptor - Device Qualifier (Size:10) */
    #if (USBD_BCD == USB_2_0)
    USB_QUALIFIER_INIT(0x01),
    #endif

    0x00
};

const uint8_t usb_string_iSerial[] = {
    // String3 - iSerial
    0x22,                       /* bLength */
    USB_DESC_TYPE_STRING,       /* bDescriptorType */
    WCHAR('X'),                 /* wcChar0 */
    WCHAR('S'),                 /* wcChar1 */
    WCHAR('M'),                 /* wcChar2 */
    WCHAR('A'),                 /* wcChar3 */
    WCHAR('6'),                 /* wcChar4 */
    WCHAR('2'),                 /* wcChar5 */
    WCHAR('4'),                 /* wcChar6 */
    WCHAR('0'),                 /* wcChar7 */
    WCHAR('4'),                 /* wcChar8 */
    WCHAR('0'),                 /* wcChar9 */
    WCHAR('5'),                 /* wcChar10 */
    WCHAR('0'),                 /* wcChar11 */
    WCHAR('0'),                 /* wcChar12 */
    WCHAR('6'),                 /* wcChar13 */
    WCHAR('0'),                 /* wcChar14 */
    WCHAR('1'),                 /* wcChar15 */
};

const uint8_t usb_string_iAudio[] = {
    // String - AUDIO_AC_STRING_INDEX
    0x10,                       /* bLength */
    USB_DESC_TYPE_STRING,       /* bDescriptorType */
    WCHAR('A'),                 /* wcChar0 */
    WCHAR('i'),                 /* wcChar1 */
    WCHAR('M'),                 /* wcChar2 */
    WCHAR('o'),                 /* wcChar3 */
    WCHAR('u'),                 /* wcChar4 */
    WCHAR('s'),                 /* wcChar5 */
    WCHAR('e'),                 /* wcChar6 */
};


/*
 * Configuration
 ****************************************************************************
 */

/*!< Declaration of endpoint Handlers  */
void usbd_audio_ep_in_handler(uint8_t ep);
void usbd_hid_raw_out_handler(uint8_t ep);

/*!< table of Audio entity */
#define USB_AUDIO_ENTITY_CNT        ARRAY_SIZE(audio_entity_tab)

static const audio_entity_t audio_entity_tab[] = {
    AUDIO_ENTITY_T(AUDIO_IN_EP, AUDIO_IN_FEAT_ID, AUDIO_CONTROL_FEATURE_UNIT),
};

/*!< table of hid interface */
#define USB_HID_INTF_CNT            ARRAY_SIZE(hid_interface)

static const hid_intf_t hid_interface[] = {
    HID_INTF_T(HID_STD_INTF_NUM, HID_STD_IN_EP, hid_std_report_desc),
    HID_INTF_T(HID_RAW_INTF_NUM, HID_RAW_IN_EP, hid_raw_report_desc),
};

/*!< table of endpoints */
static const usbd_ep_t endpoint_tab[] = {
    // Audio endpoints
    USBD_EP_T(AUDIO_IN_EP, USB_EP_TYPE_ISOCHRONOUS, AUDIO_IN_EP_MPS, &usbd_audio_ep_in_handler),

    // HID endpoints
    USBD_EP_T(HID_STD_IN_EP,  USB_EP_TYPE_INTERRUPT, HID_STD_IN_EP_MPS,  &usbd_hid_ep_in_handler),
    USBD_EP_T(HID_RAW_IN_EP,  USB_EP_TYPE_INTERRUPT, HID_RAW_IN_EP_MPS,  &usbd_hid_ep_in_handler),
    USBD_EP_T(HID_RAW_OUT_EP, USB_EP_TYPE_INTERRUPT, HID_RAW_OUT_EP_MPS, &usbd_hid_raw_out_handler),
};

/*!< table of class */
static const usbd_class_t class_tab[] = {
    // Audio class
    USBD_CLASS_T(USB_AUDIO_INTF_START, USB_AUDIO_INTF_END, &usbd_audio_class_handler),
    // HID class
    USBD_CLASS_T(USB_HID_INTF_START, USB_HID_INTF_END, &usbd_hid_class_handler),
};

/*!< USBD Configuration */
static const usbd_config_t usb_configuration[] = {
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

void usbd_hid_raw_out_handler(uint8_t ep)
{
    uint8_t custom_data[HID_RAW_OUT_EP_MPS];
    
    /*!< read the data from host send */
    uint16_t dlen = usbd_ep_read(HID_RAW_OUT_EP, HID_RAW_OUT_EP_MPS, custom_data);

    /*!< you can use the data do some thing you like */
    USB_LOG_RAW("HID Raw dlen=%d\r\n", dlen);
}

__USBIRQ void usbd_notify_handler(uint8_t event, void *arg)
{
    switch (event) {
        case USBD_EVENT_RESET:
            usbd_hid_reset();
            break;
        case USBD_EVENT_RESUME:
            break;
        case USBD_EVENT_SUSPEND:
            break;

        default:
            break;
    }
}

__USBIRQ bool usbd_get_string_handler(uint16_t index, uint8_t **data, uint16_t *len)
{
    bool found = false;

    if (index == USB_DESC_TYPE_STRING) {
        *data = (uint8_t *)usb_string_iSerial;
        *len = usb_string_iSerial[0];
        found = true;
    } else if (index == AUDIO_AC_STRING_INDEX) {
        *data = (uint8_t *)usb_string_iAudio;
        *len = usb_string_iAudio[0];
        found = true;
    }

    return found;
}

/*
 * Test Functions
 ****************************************************************************
 */

static uint8_t mic_tmp = 0;
static uint8_t mic_buffer[AUDIO_IN_EP_MPS];

void usbdInit()
{
    // enable USB clk and iopad
    rcc_usb_en();

    usbd_init();
    usbd_register(usb_descriptor, usb_configuration);

    for (uint8_t idx = 0; idx < USB_HID_INTF_CNT; idx++) {
        usbd_hid_init(idx, &hid_interface[idx]);
    }

    NVIC_EnableIRQ(USB_IRQn);
}

//void usbdTest()
//{
//    if (usbd_is_configured()) {
//        if (mic_state == MIC_IDLE) {
//            uint8_t status = 0;
//            memset(mic_buffer, mic_tmp, AUDIO_IN_EP_MPS);

//            GPIO_DAT_SET(GPIO16);
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
//            GPIO_DAT_CLR(GPIO16);
//            
//            mic_tmp++;
//        }
//    }
//}


#include "micphone.h"
bool mic_flag = false;

void usbdTest()
{
    uint8_t *ptr = micDataGet();  // PCM_SAMPLE_NB = 8;

    if (ptr != NULL)
    {
        mic_flag = true;
        memcpy(mic_buffer, ptr, AUDIO_IN_EP_MPS);
    }
    
    if (usbd_is_configured()) {
        if ((mic_state == MIC_IDLE) && mic_flag) {
            uint8_t status = 0;
//            memset(mic_buffer, mic_tmp, AUDIO_IN_EP_MPS);

            GPIO_DAT_SET(GPIO16);
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
            GPIO_DAT_CLR(GPIO16);
            
            mic_tmp++;
            
            mic_flag = false;
        }
    }
}
#endif
