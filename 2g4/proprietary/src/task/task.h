/**
 ****************************************************************************************
 *
 * @file task.h
 *
 * @brief Header file - Task manager
 *
 ****************************************************************************************
 */

#ifndef _TASK_H_
#define _TASK_H_

#include <stdint.h>

/*
 * DEFINES
 ****************************************************************************************
 */
#define TASK_SYSCALL_EVENT(call_n)     ( 0x00010000 << (call_n) )

typedef void (*task_fn_t)(void *, uint32_t);

/*
 * FUNCTION DECLARATION
 ****************************************************************************************
 */

void task_init(void);
void task_conf(task_fn_t sys_fn, void *sys_param,
               task_fn_t user_fn, void *user_param, uint32_t user_flag);

void task_invoke(void);
void task_event(uint32_t event);
void task_syscall(uint32_t call_n);

#endif
