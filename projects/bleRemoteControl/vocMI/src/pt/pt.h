/**
 * @file pt.h
 * @brief pt组件总头文件
 * @author lijianpeng (bosco@pthyidh.com)
 * @date 2024-03-13 01:53:28
 * 
 * @copyright Copyright (c) 2024 by 深圳市鹏天微智能科技有限公司, All Rights Reserved.
 */
#ifndef PT_H
#define PT_H

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>

#include "pt_def.h"
#include "pt_log.h"
#include "pt_list.h"

#define pt_calloc  calloc
#define pt_malloc  malloc
#define pt_free    free
#define pt_memcpy  memcpy
#define pt_snprintf snprintf

#define pt_assert assert

#endif /* PT_H */

