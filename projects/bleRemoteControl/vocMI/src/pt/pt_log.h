/**
 * @file pt_log.h
 * @brief log调试宏头文件
 * @author lijianpeng (bosco@pthyidh.com)
 * @date 2023-07-24 11:10:22
 * 
 * @copyright Copyright (c) 2023 by 深圳市鹏天微智能科技有限公司, All Rights Reserved.
 */

#ifndef PT_LOG_H
#define PT_LOG_H

#include <stdio.h>
// #include "dbg.h"

#if (DBG_PT_LOG)
#define _printf printf
#else
#define _printf
#endif

#define LOG_NONE    0
#define LOG_ERROR   1
#define LOG_WARN    2
#define LOG_INFO    3
#define LOG_DEBUG   4

#if (DBG_MODE == 0)
#define LOG_LEVEL   LOG_NONE
#else
/* 修改log宏等级 */
#define LOG_LEVEL   LOG_DEBUG
#endif

/* 日志前缀 */
#define USE_PREFIX

#ifdef USE_PREFIX
#define pt_printf(lvl, fmt, ...)      _printf("%c/%03d> " fmt , lvl, __LINE__, ##__VA_ARGS__)
#else /*!< not def  */
#define pt_printf(lvl, fmt, ...)      _printf(fmt, ##__VA_ARGS__)
#endif /*!< #ifdef  */

/* 不带前缀的输出 */
#if(LOG_LEVEL)
#define PT_LOG(...)         _printf(__VA_ARGS__)
#else
#define PT_LOG(...)
#endif

#if (LOG_LEVEL >= LOG_DEBUG)
#define PT_LOGD(...)        pt_printf('D', __VA_ARGS__)
#else
#define PT_LOGD(...)        ((void)0)
#endif

#if (LOG_LEVEL >= LOG_INFO)
#define PT_LOGI(...)        pt_printf('I', __VA_ARGS__)
#else
#define PT_LOGI(...)        ((void)0)
#endif

#if (LOG_LEVEL >= LOG_WARN)
#define PT_LOGW(...)        pt_printf('W', __VA_ARGS__)
#else
#define PT_LOGW(...)        ((void)0)
#endif

#if (LOG_LEVEL >= LOG_ERROR)
#define PT_LOGE(...)        pt_printf('E', __VA_ARGS__)
#else
#define PT_LOGE(...)        ((void)0)
#endif

#define PT_LOG_HEX(dat, len) \
    do { \
        _printf("(%03d)[%s]> ", __LINE__, __MODULE__); \
        for (int i = 0; i < len; i++)             \
        {                                         \
            _printf("%02X ", *((dat) + i));         \
        }                                         \
        _printf("\r\n");                            \
    } while (0)

#endif /* PT_LOG_H */
