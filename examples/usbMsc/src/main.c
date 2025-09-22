/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Main Entry of the application.
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "drvs.h"
#include "dbg.h"

#include "usbd.h"
#include "usbd_msc.h"

#if (DBG_MSC)
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif

#define USBD_VID                    0x13A9
#define USBD_PID                    0x10CC
#define DEV_BCD                     0x0110
#define USBD_MAX_POWER              300
#define USBD_LANGID_STRING          0x0409 // English(US)


/* MSC Class Config */
#define MSC_MAX_MPS                 64

#define MSC_IN_EP                   0x81
#define MSC_OUT_EP                  0x01

/*!< count of interface descriptor */
enum intf_num {
    /* class interface */
    MSC_INTF_NUM                    = 0,

    /* total interface count */
    USB_CONFIG_INTF_CNT,

    /* start&end interface */
    USB_MSC_INTF_START              = MSC_INTF_NUM,
    USB_MSC_INTF_END                = MSC_INTF_NUM,
};

/*!< Length of Configure descriptor */
#define USB_MSC_CONFIG_SIZE         (9 + MSC_DESCRIPTOR_LEN)

/*!< USB device descriptor */
const uint8_t msc_descriptor[] = {
    /* Descriptor - Device (Size:18) */
    USB_DEVICE_DESCRIPTOR_INIT(USB_1_1, 0x00, 0x00, 0x00, USBD_VID, USBD_PID, DEV_BCD, 0x01),

    /* Descriptor - Configuration (Total Size:9+Intf_Size) */
    USB_CONFIG_DESCRIPTOR_INIT(USB_MSC_CONFIG_SIZE, USB_CONFIG_INTF_CNT, 
            0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),

    MSC_DESCRIPTOR_INIT(MSC_INTF_NUM, MSC_OUT_EP, MSC_IN_EP, MSC_MAX_MPS, 0x00),//0x02),

    /* Descriptor - String */
    // String0 - Language ID (Size:4)
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
    WCHAR('M'),                 /* wcChar0 */
    WCHAR('S'),                 /* wcChar1 */
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

    /* Descriptor - EOF */
    0x00
};


/*
 * Configuration
 ****************************************************************************
 */

/*!< table of endpoints */
static const usbd_ep_t endpoint_tab[] = {
    USBD_EP_T(MSC_OUT_EP, USB_EP_TYPE_BULK,      MSC_MAX_MPS, &usbd_msc_bulk_out_handler),
    USBD_EP_T(MSC_IN_EP,  USB_EP_TYPE_BULK,      MSC_MAX_MPS, &usbd_msc_bulk_in_handler),
};

/*!< table of class */
static const usbd_class_t class_tab[] = {
    USBD_CLASS_T(USB_MSC_INTF_START, USB_MSC_INTF_END, &usbd_msc_class_handler),
};

/*!< USBD Configuration */
static const usbd_config_t msc_configuration[] = {
    USBD_CONFIG_T(1, USB_CONFIG_INTF_CNT, class_tab, endpoint_tab)
};


/*
 * Handlers
 ****************************************************************************
 */

__USBIRQ void usbd_notify_handler(uint8_t event, void *arg)
{
    DEBUG("evt:%d", event);
    switch (event) {
        case USBD_EVENT_RESET:
            break;
        case USBD_EVENT_CONNECTED:
            break;
        case USBD_EVENT_DISCONNECTED:
            break;
        case USBD_EVENT_RESUME:
            break;
        case USBD_EVENT_SUSPEND:
            break;
        case USBD_EVENT_CONFIGURED:
            break;
        case USBD_EVENT_SET_REMOTE_WAKEUP:
            break;
        case USBD_EVENT_CLR_REMOTE_WAKEUP:
            break;

        default:
            break;
    }
}


/*
 * FUNCTIONS
 ****************************************************************************************
 */

static void usbdInit(void)
{
    // enable USB clk and iopad
    rcc_usb_en();

    usbd_init();
    usbd_register(msc_descriptor, msc_configuration);

    usbd_msc_init(MSC_OUT_EP, MSC_IN_EP);

    NVIC_EnableIRQ(USB_IRQn);
}

static void usbdTest()
{
    if (usbd_is_configured()) {

    }
}

static void sysInit(void)
{
    // switch syclk to 48M for USB
    rcc_sysclk_set(SYS_CLK_48M);

    rcc_fshclk_set(FSH_CLK_DPSC42);
}

static void devInit(void)
{
    uint16_t rsn = rstrsn();
    
    iwdt_disable();
    
    dbgInit();
    debug("Start(rsn:0x%X)...\r\n", rsn);
    
    usbdInit();
}

int main(void)
{
    sysInit();
    devInit();

    // Global Interrupt Enable
    GLOBAL_INT_START();

    while (1)
    {
        usbdTest();
    }
}
