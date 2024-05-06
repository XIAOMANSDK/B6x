/**
 ****************************************************************************************
 *
 * @file usb_log.h
 *
 * @brief Header file of USB debug/log print.
 *
 ****************************************************************************************
 */

#ifndef _USB_LOG_H
#define _USB_LOG_H

#include <stdio.h>

/* Debug level */
#define USB_DBG_DISABLE           0
#define USB_DBG_ERROR             1
#define USB_DBG_WARNING           2
#define USB_DBG_INFO              3
#define USB_DBG_LOG               4

#ifndef USB_DBG_LEVEL
#define USB_DBG_LEVEL             USB_DBG_DISABLE //USB_DBG_ERROR
#endif

#ifndef USB_PRINTF
#define USB_PRINTF                printf
#endif

/*
 * The color for terminal (foreground)
 * 30 - BLACK     31 - RED      
 * 32 - GREEN     33 - YELLOW   
 * 34 - BLUE      35 - PURPLE   
 * 36 - CYAN      37 - WHITE    
 */
#ifndef COLOR_TERMINAL
#define COLOR_TERMINAL            (0)
#endif

#if  (COLOR_TERMINAL)
#define usb_log_print(lvl_name, color_n, fmt, ...)            \
    do {                                                      \
        USB_PRINTF("\033[" #color_n "m");                     \
        USB_PRINTF("[" lvl_name "/USB] " fmt, ##__VA_ARGS__); \
        USB_PRINTF("\033[0m");                                \
    } while (0)
#else
#define usb_log_print(lvl_name, color_n, fmt, ...)            \
    do {                                                      \
        USB_PRINTF("[" lvl_name "/USB] " fmt, ##__VA_ARGS__); \
    } while (0)
#endif


#if (USB_DBG_LEVEL >= USB_DBG_LOG)
#define USB_LOG_DBG(fmt, ...)     usb_log_print("D", 0, fmt, ##__VA_ARGS__)
#else
#define USB_LOG_DBG(...)
#endif

#if (USB_DBG_LEVEL >= USB_DBG_INFO)
#define USB_LOG_INFO(fmt, ...)    usb_log_print("I", 32, fmt, ##__VA_ARGS__)
#else
#define USB_LOG_INFO(...)
#endif

#if (USB_DBG_LEVEL >= USB_DBG_WARNING)
#define USB_LOG_WRN(fmt, ...)     usb_log_print("W", 33, fmt, ##__VA_ARGS__)
#else
#define USB_LOG_WRN(...)
#endif

#if (USB_DBG_LEVEL >= USB_DBG_ERROR)
#define USB_LOG_ERR(fmt, ...)     usb_log_print("E", 31, fmt, ##__VA_ARGS__)
#else
#define USB_LOG_ERR(...)
#endif

#if (USB_DBG_LEVEL != USB_DBG_DISABLE)
#define USB_LOG_RAW(fmt, ...)     USB_PRINTF(fmt, ##__VA_ARGS__)
#else
#define USB_LOG_RAW(...)
#endif


/**
 * @brief print the contents of usb setup packet(pointer)
 */
#define USB_PRINT_SETUP(setup)                                          \
    USB_LOG_INFO("Setup: bmRequestType 0x%02x, bRequest 0x%02x, "       \
                    "wValue 0x%04x, wIndex 0x%04x, wLength 0x%04x\r\n", \
                    setup->bmRequestType, setup->bRequest,              \
                    setup->wValue, setup->wIndex, setup->wLength);

/**
 * @brief assert the position of file and line
 */
void usb_assert(const char *filename, int linenum);

#define USB_ASSERT(f)                       \
    do {                                    \
        if (!(f))                           \
            usb_assert(__FILE__, __LINE__); \
    } while (0)


#endif
