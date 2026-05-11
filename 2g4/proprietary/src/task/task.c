#include "task.h"
#include "b6x.h"

#define NVIC_INT_CTRL  (*((volatile uint32_t *)0xe000ed04))
#define NVIC_PENDSVSET (1UL << 28)

#define TASK_PENDSV_PRI (255UL >> (8 - __NVIC_PRIO_BITS))

#define TASK_STACK_SIZE (2048UL / sizeof(uint32_t))

#define TASK_INITIAL_XPSR (0x01000000)

typedef uint32_t task_stack_t;

typedef struct
{
    volatile task_stack_t *stack_top;
    task_fn_t              task_proc;
    void                  *task_param;
    uint32_t               task_flag;
} task_ctrl_t;

static task_stack_t TASK_SYS_PROC_STACK[TASK_STACK_SIZE];
static task_stack_t TASK_USER_PROC_STACK[TASK_STACK_SIZE];

task_ctrl_t task_sys_ctrl =
{
    &TASK_SYS_PROC_STACK[TASK_STACK_SIZE - 1],
};
task_ctrl_t task_user_ctrl =
{
    &TASK_USER_PROC_STACK[TASK_STACK_SIZE - 1],
};

volatile task_ctrl_t *task_current  = 0;
volatile task_ctrl_t *task_next     = 0;
volatile uint32_t     task_sys_flag = 0;

extern void InvokeTask(void);

static void task_error(void)
{
    while (1)
    {
    }
}

static void task_sys_loop()
{
    while (1)
    {
        uint32_t flag;

        __disable_irq();

        flag          = task_sys_flag;
        task_sys_flag = 0;

        if (!flag)
        {
            task_next = &task_user_ctrl;

            __enable_irq();

            SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
        }
        else
        {
            __enable_irq();

            task_sys_ctrl.task_proc(task_sys_ctrl.task_param, flag);
        }
    }
}

static void task_user_loop()
{
    while (1)
    {
        task_user_ctrl.task_proc(task_user_ctrl.task_param, task_user_ctrl.task_flag);
    }
}

static void task_init_stack(task_ctrl_t *tc, void (*loop)(void))
{
    volatile task_stack_t *stack  = tc->stack_top;
    *--stack                      = TASK_INITIAL_XPSR;        /* xPSR */
    *--stack                      = (task_stack_t)loop;       /* PC */
    *--stack                      = (task_stack_t)task_error; /* LR */
    stack                        -= 4;                        /* R12, R3, R2 and R1. */
    *--stack                      = (task_stack_t)0;          /* R0 */
    stack                        -= 8;                        /* R4 ~ R11 */
    tc->stack_top                 = stack;
}

void task_init(void)
{
    NVIC_SetPriority(PendSV_IRQn, TASK_PENDSV_PRI);
}

void task_conf(
    task_fn_t sys_fn, void *sys_param, task_fn_t user_fn, void *user_param, uint32_t user_flag)
{
    task_sys_ctrl.task_proc  = sys_fn;
    task_sys_ctrl.task_param = sys_param;
    task_sys_ctrl.task_flag  = 0;
    task_init_stack(&task_sys_ctrl, task_sys_loop);

    task_user_ctrl.task_proc  = user_fn;
    task_user_ctrl.task_param = user_param;
    task_user_ctrl.task_flag  = user_flag;
    task_init_stack(&task_user_ctrl, task_user_loop);

    task_current = &task_sys_ctrl;
    task_next    = &task_sys_ctrl;
}

void task_invoke(void)
{
    InvokeTask();
}

void task_event(uint32_t event)
{
    task_sys_flag |= event & 0xFFFFUL;

    if (task_current != &task_sys_ctrl)
    {
        task_next     = &task_sys_ctrl;
        SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
    }
}

void task_syscall(uint32_t call_n)
{
    GLOBAL_INT_DISABLE();

    task_sys_flag |= TASK_SYSCALL_EVENT(call_n);
    task_next      = &task_sys_ctrl;

    GLOBAL_INT_RESTORE();
    
    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
}
