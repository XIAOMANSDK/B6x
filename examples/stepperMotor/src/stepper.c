#include "drvs.h"
#include "stepper.h"

// 使用 ATMR 作为步间隔定时器（中断）
#if (SYS_CLK == 1)
    #define TMR_PSC             (32 - 1) // 1us sysclk=(n)MHz
#elif (SYS_CLK == 2)
    #define TMR_PSC             (48 - 1) // 1us sysclk=(n)MHz
#elif (SYS_CLK == 3)
    #define TMR_PSC             (64 - 1) // 1us sysclk=(n)MHz
#else
    #define TMR_PSC             (16 - 1) // 1us sysclk=(n)MHz
#endif //SYS_CLK

#define TMR_ARR             1000  // 1ms

// 基本参数 24BYJ48
#define STEPS_PER_REV     4096     // 逻辑步/转 (e.g. 4096 1转) RPM=PPS * 步距角 * 减速比/360°* 60  （转/min）
#define REVOL_PER_MIN_MAX 15       // 逻辑转/分 (e.g. 最大转速 15转 每分钟)
#define STEPS_PER_S_MAX   1000     // 逻辑步/秒 (e.g. 最大转速 1000步 每秒)
#define STEPS_PER_S_MIN   38       // 逻辑步/秒 (e.g. 初始转速 68步 每秒)

typedef struct {
    // 运动状态
    uint32_t remaining;     // 剩余步数
    uint16_t acceleration;  // 加速步数
    bool     forward;       // 旋转方向:正转/反转
    bool     busy;          // 状态
    
    // 速度控制参数 (以 steps/s)
    uint16_t current_per;   // 当前速度
    uint16_t target_per;    // 目标速度
    uint16_t accel_per;     // 加速度
    
} Stepper_t;

// ====== 步序（8-step 半步） ======
static const uint8_t halfstep_seq[8] = {
    0x08,//{1,0,0,0},
    0x0C,//{1,1,0,0},
    0x04,//{0,1,0,0},
    0x06,//{0,1,1,0},
    0x02,//{0,0,1,0},
    0x03,//{0,0,1,1},
    0x01,//{0,0,0,1},
    0x09,//{1,0,0,1}
};
// ====== end tables ======

static Stepper_t hstepper;
static volatile uint8_t cur_step_index = 0;       // 0..7 for halfstep
static volatile uint32_t step_interval_ticks = 0; // 定时器 ticks (ARR) (单位取决于 timer clock)

// 简单梯形实现：当 remaining > 0，逐步调整 current_speed 到 target_speed
// 这里我们在每步完成后调整 current_speed += accel * dt(=1/current_speed), 使用较简单的递推（可替代为常用的 Bresenham-like）
static void apply_step_pins(uint8_t abcd)
{
    uint32_t setPinBits = 0;
    uint32_t clrPinBits = 0;
    
    if (abcd & 0x08) setPinBits = BIT(PA_MOTOR_A);
    else   clrPinBits = BIT(PA_MOTOR_A);
    
    if (abcd & 0x04) setPinBits |= BIT(PA_MOTOR_B);
    else   clrPinBits |= BIT(PA_MOTOR_B);
    
    if (abcd & 0x02) setPinBits |= BIT(PA_MOTOR_C);
    else   clrPinBits |= BIT(PA_MOTOR_C);
    
    if (abcd & 0x01) setPinBits |= BIT(PA_MOTOR_D);
    else   clrPinBits |= BIT(PA_MOTOR_D);
    
    GPIO_DAT_CLR(clrPinBits);
    GPIO_DAT_SET(setPinBits);
}

void stepperInit(void)
{
    // Init GPIO
    GPIO_DAT_CLR(BIT(PA_MOTOR_A) | BIT(PA_MOTOR_B) | BIT(PA_MOTOR_C) | BIT(PA_MOTOR_D)); // Stop
    GPIO_DIR_SET(BIT(PA_MOTOR_A) | BIT(PA_MOTOR_B) | BIT(PA_MOTOR_C) | BIT(PA_MOTOR_D)); // Output enable
    
    hstepper.remaining = 0;
    hstepper.busy = false;
}

// 将目标 rpm 转换为 steps/s.  rpm max = 15.
static uint16_t rpm_to_steps_per_s(uint8_t rpm)
{
    // steps/s = rpm * steps_per_rev / 60
    // 为避免浮点，用整数运算（乘法先）
    uint32_t tmp = (uint32_t)rpm * STEPS_PER_REV;
    tmp /= 60;
    
    return tmp; // steps/s
}

// 根据 steps/s 计算定时器 ARR（以 timer ticks 单位）
// tick_us = prescaled timer frequency => ticks per second = timer_clock / (prescaler+1)
// interval_ticks = ticks_per_second / steps_per_s
// ATMR->ARR. MAX 16bits
static uint16_t stepsps_to_interval_ticks(uint16_t steps_per_s)
{
    if (steps_per_s == 0) return 0xFFFF;
    uint32_t ticks_per_s = 1000000; // tick_us
    uint32_t interval = ticks_per_s / steps_per_s;
    if (interval == 0) interval = 1;
    if (interval > 0xFFFF) interval = 0xFFFF;
    return interval;
}

void stepperMove(int32_t steps, uint8_t rpm, uint32_t accel_sps2)
{
    // 设置目标与加速度
    hstepper.accel_per = accel_sps2;
    hstepper.target_per = rpm_to_steps_per_s(rpm);
    
    if (steps == 0) return;

    if (hstepper.busy)
    {
        if (steps > 0)
        {
            if (hstepper.forward)
                hstepper.remaining +=  steps;  //正转增加旋转步数
            else
            {
                if (hstepper.remaining > (steps + hstepper.acceleration))
                    hstepper.remaining -=  steps;  //正转减小旋转步数
                else
                {
                    //等待停止正转
                }
            }
        }
        else
        {
            if (hstepper.forward)
            {
                if (hstepper.remaining > (-steps + hstepper.acceleration))
                    hstepper.remaining +=  steps;  //反转减小旋转步数
                else
                {
                    //等待停止反转
                }
            }
            else
                hstepper.remaining -=  steps;  //反转增加旋转步数
        }
    }
    else
    {
        // 设定方向：我们用 steps 的符号确定方向（step_index +1 或 -1）
        hstepper.remaining = (steps > 0) ? steps : -steps;    
    }
    
    if (hstepper.remaining > hstepper.target_per)
        hstepper.acceleration = ((hstepper.target_per - STEPS_PER_S_MIN)/(accel_sps2/100));
    else
        hstepper.acceleration = hstepper.remaining/2;
    
    if (hstepper.busy) return;
    
    // 记录方向 sign
    if (steps > 0) {
        // 正方向 (index++)
        hstepper.forward = true;
    } else {
        // 负方向 (index--)
        hstepper.forward = false;
    }

    // 启动运动
    hstepper.busy = true;

    // 初始化 current_speed 为一个小值以渐加速（避免除0）
    hstepper.current_per = STEPS_PER_S_MIN;

    // 计算首次 timer interval 并启动定时器中断
    step_interval_ticks = stepsps_to_interval_ticks(hstepper.current_per);

    // 确保 timer 产生中断并启动，但不立即启用计数。
    atmr_init(TMR_PSC, step_interval_ticks);
    atmr_ctrl(TMR_PERIOD_MODE, TMR_IR_UI_BIT);
    NVIC_EnableIRQ(ATMR_IRQn);
}

// 立即停止（粗暴）
void stepperStop(void)
{
    atmr_deinit();
    hstepper.busy = false;
    hstepper.remaining = 0;
    // 断电线圈以免发热（如果需要）
    apply_step_pins(0);
}

bool stepperIsBusy(void)
{
    return hstepper.busy;
}

// 中断处理：将由 HAL 在 TIM2 IRQ 中调用（或在 HAL TIM 回调中转接）
void Stepper_TIM_Callback(void) // 请在 HAL_TIM_PeriodElapsedCallback 内调用此函数
{
    if (!hstepper.busy) return;

    // 产生一步（更新序列）
    // 判断方向
    // step index
    if (hstepper.forward) {
        cur_step_index++;
        if (cur_step_index >= 8) cur_step_index = 0;
    } else {
        if (cur_step_index == 0) cur_step_index = 7;
        else cur_step_index--;
    }

    // 输出到 GPIO（半步/八拍）
    apply_step_pins(halfstep_seq[cur_step_index]);

    // 完成一步
    if (hstepper.remaining) hstepper.remaining--;

    // 速度递推：简单线性递推每步增加 delta_v = accel * dt
    // dt ≈ 1 / current_speed (s per step) ; 但为避免浮点，这里用近似更新：
    uint16_t delta = hstepper.accel_per / 100;
    if (delta == 0) delta = 1;
    
    if (hstepper.current_per < hstepper.target_per) {
        // 增速： current_speed += accel * dt  => 近似： current_speed += accel / current_speed
        // 为避免除0，保证 current_speed>=1
        hstepper.current_per += delta;
        if (hstepper.current_per > hstepper.target_per) hstepper.current_per = hstepper.target_per;
    } else if (hstepper.current_per > hstepper.target_per) {
        // 减速（若目标小于当前）
        if ((hstepper.current_per > STEPS_PER_S_MIN) && (hstepper.current_per > delta)) hstepper.current_per -= delta;
        else hstepper.current_per = STEPS_PER_S_MIN;
    }
    
    // 减速阶段
    if (hstepper.remaining == hstepper.acceleration) hstepper.target_per = STEPS_PER_S_MIN;
    
    // 重新计算 timer interval
    ATMR->ARR = stepsps_to_interval_ticks(hstepper.current_per);
    
    // 结束条件
    if (hstepper.remaining == 0) {
        // 停止 timer
        atmr_deinit();
        hstepper.busy = false;
        // 可选断电线圈
        apply_step_pins(0);
    }
}

//中断回调
void ATMR_IRQHandler(void)
{
    uint32_t irq_stat = ATMR->RIF.Word;
    
    // Each  tick Interrupt is 1ms
    if (irq_stat & 0x01/*TIMER_UI_BIT*/)
    {
        ATMR->IDR.UI = 1;  // Disable UI Interrupt

        Stepper_TIM_Callback();
        
        ATMR->ICR.UI = 1; //Clear Interrupt Flag
        ATMR->IER.UI = 1; // Enable UI Interrupt
    }
}
