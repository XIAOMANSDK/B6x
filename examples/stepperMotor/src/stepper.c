/**
 ****************************************************************************************
 *
 * @file stepper.c
 *
 * @brief Stepper motor driver for 24BYJ48 using ATMR timer with acceleration.
 *
 ****************************************************************************************
 */

#include "drvs.h"
#include "stepper.h"

/*
 * DEFINES
 ****************************************************************************************
 */

// Timer prescaler for 1us tick based on SYS_CLK
#if (SYS_CLK == 1)
    #define TMR_PSC             (32 - 1)
#elif (SYS_CLK == 2)
    #define TMR_PSC             (48 - 1)
#elif (SYS_CLK == 3)
    #define TMR_PSC             (64 - 1)
#else
    #define TMR_PSC             (16 - 1)
#endif

#define TMR_ARR_MAX         0xFFFFu     // Max 16-bit timer value
#define TICKS_PER_SECOND    1000000u    // 1us tick resolution
#define HALFSTEP_SEQ_SIZE   8           // Half-step sequence length
#define ACCEL_DIVISOR       100         // Acceleration scaling factor

// 24BYJ48 stepper motor constants
#define STEPS_PER_REV       4096        // Steps per revolution
#define REVOL_PER_MIN_MAX   15          // Max revolutions per minute
#define STEPS_PER_S_MAX     1000        // Max steps per second
#define STEPS_PER_S_MIN     38          // Min startup steps per second

typedef struct {
    // Motion state
    uint32_t remaining;     // Remaining steps
    uint16_t acceleration;  // Deceleration threshold
    bool     forward;       // Direction: true=CW, false=CCW
    bool     busy;          // Active state

    // Speed control (in steps/s)
    uint16_t current_per;   // Current speed
    uint16_t target_per;    // Target speed
    uint16_t accel_per;     // Acceleration rate
} stepper_t;

// 8-step half-step sequence {A,B,C,D}
static const uint8_t halfstep_seq[HALFSTEP_SEQ_SIZE] = {
    0x08,  // {1,0,0,0}
    0x0C,  // {1,1,0,0}
    0x04,  // {0,1,0,0}
    0x06,  // {0,1,1,0}
    0x02,  // {0,0,1,0}
    0x03,  // {0,0,1,1}
    0x01,  // {0,0,0,1}
    0x09,  // {1,0,0,1}
};

static volatile stepper_t hstepper;
static volatile uint8_t cur_step_index = 0;
static volatile uint32_t step_interval_ticks = 0;

/*
 * FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Apply step pattern to motor GPIO pins.
 *
 * @param[in] abcd  4-bit pattern {A,B,C,D}
 ****************************************************************************************
 */
static void apply_step_pins(uint8_t abcd)
{
    uint32_t setPinBits = 0;
    uint32_t clrPinBits = 0;

    if (abcd & 0x08) setPinBits |= BIT(PA_MOTOR_A);
    else             clrPinBits |= BIT(PA_MOTOR_A);

    if (abcd & 0x04) setPinBits |= BIT(PA_MOTOR_B);
    else             clrPinBits |= BIT(PA_MOTOR_B);

    if (abcd & 0x02) setPinBits |= BIT(PA_MOTOR_C);
    else             clrPinBits |= BIT(PA_MOTOR_C);

    if (abcd & 0x01) setPinBits |= BIT(PA_MOTOR_D);
    else             clrPinBits |= BIT(PA_MOTOR_D);

    GPIO_DAT_CLR(clrPinBits);
    GPIO_DAT_SET(setPinBits);
}

/**
 ****************************************************************************************
 * @brief Convert RPM to steps per second (integer math, no float).
 *
 * @param[in] rpm  Revolutions per minute
 *
 * @return Steps per second
 ****************************************************************************************
 */
static uint16_t rpm_to_steps_per_s(uint8_t rpm)
{
    uint32_t tmp = (uint32_t)rpm * STEPS_PER_REV;
    tmp /= 60;

    return (uint16_t)tmp;
}

/**
 ****************************************************************************************
 * @brief Convert steps/s to timer interval ticks (ARR value).
 *
 * @param[in] steps_per_s  Steps per second
 *
 * @return Timer ARR value (16-bit)
 ****************************************************************************************
 */
static uint16_t stepsps_to_interval_ticks(uint16_t steps_per_s)
{
    if (steps_per_s == 0)
    {
        return TMR_ARR_MAX;
    }

    uint32_t interval = TICKS_PER_SECOND / steps_per_s;
    if (interval == 0) interval = 1;
    if (interval > TMR_ARR_MAX) interval = TMR_ARR_MAX;

    return (uint16_t)interval;
}

/**
 ****************************************************************************************
 * @brief Initialize stepper motor GPIO and state.
 ****************************************************************************************
 */
void stepper_init(void)
{
    GPIO_DAT_CLR(BIT(PA_MOTOR_A) | BIT(PA_MOTOR_B) | BIT(PA_MOTOR_C) | BIT(PA_MOTOR_D));
    GPIO_DIR_SET(BIT(PA_MOTOR_A) | BIT(PA_MOTOR_B) | BIT(PA_MOTOR_C) | BIT(PA_MOTOR_D));

    hstepper.remaining = 0;
    hstepper.busy = false;
}

/**
 ****************************************************************************************
 * @brief Start stepper motion with acceleration.
 *
 * @param[in] steps           Step count (positive=CW, negative=CCW)
 * @param[in] rpm             Target speed in RPM
 * @param[in] accel_sps2      Acceleration in steps/s^2
 ****************************************************************************************
 */
void stepper_move(int32_t steps, uint8_t rpm, uint32_t accel_sps2)
{
    hstepper.accel_per = accel_sps2;
    hstepper.target_per = rpm_to_steps_per_s(rpm);

    if (steps == 0) return;

    if (hstepper.busy)
    {
        if (steps > 0)
        {
            if (hstepper.forward)
                hstepper.remaining += steps;     // Same direction: add steps
            else if (hstepper.remaining > (uint32_t)(steps + hstepper.acceleration))
                hstepper.remaining -= steps;     // Opposite: subtract
            /* else: wait for current motion to complete */
        }
        else
        {
            if (!hstepper.forward)
                hstepper.remaining -= steps;     // Same direction: add steps
            else if (hstepper.remaining > (uint32_t)(-steps + hstepper.acceleration))
                hstepper.remaining += steps;     // Opposite: subtract
            /* else: wait for current motion to complete */
        }
    }
    else
    {
        hstepper.remaining = (steps > 0) ? (uint32_t)steps : (uint32_t)(-steps);
    }

    // Calculate deceleration threshold, guard against division by zero
    if (hstepper.remaining > hstepper.target_per)
    {
        uint32_t divisor = (accel_sps2 >= ACCEL_DIVISOR) ? (accel_sps2 / ACCEL_DIVISOR) : 1;
        hstepper.acceleration = (hstepper.target_per - STEPS_PER_S_MIN) / divisor;
    }
    else
    {
        hstepper.acceleration = hstepper.remaining / 2;
    }

    if (hstepper.busy) return;

    // Critical section: set motion parameters atomically
    GLOBAL_INT_STOP();

    // Set direction
    hstepper.forward = (steps > 0);

    // Start motion
    hstepper.busy = true;
    hstepper.current_per = STEPS_PER_S_MIN;
    step_interval_ticks = stepsps_to_interval_ticks(hstepper.current_per);

    GLOBAL_INT_START();

    atmr_init(TMR_PSC, step_interval_ticks);
    atmr_ctrl(TMR_PERIOD_MODE, TMR_IR_UI_BIT);
    NVIC_EnableIRQ(ATMR_IRQn);
}

/**
 ****************************************************************************************
 * @brief Stop stepper motor immediately.
 ****************************************************************************************
 */
void stepper_stop(void)
{
    atmr_deinit();
    hstepper.busy = false;
    hstepper.remaining = 0;
    apply_step_pins(0);
}

/**
 ****************************************************************************************
 * @brief Check if stepper motor is currently moving.
 *
 * @return true if busy, false if idle
 ****************************************************************************************
 */
bool stepper_is_busy(void)
{
    return hstepper.busy;
}

/**
 ****************************************************************************************
 * @brief Timer ISR callback for stepper step generation.
 *
 * Handles step output, acceleration/deceleration, and timer interval updates.
 ****************************************************************************************
 */
static void stepper_timer_callback(void)
{
    if (!hstepper.busy) return;

    // Advance step index
    if (hstepper.forward)
    {
        cur_step_index++;
        if (cur_step_index >= HALFSTEP_SEQ_SIZE) cur_step_index = 0;
    }
    else
    {
        if (cur_step_index == 0) cur_step_index = HALFSTEP_SEQ_SIZE - 1;
        else cur_step_index--;
    }

    // Apply GPIO pattern
    apply_step_pins(halfstep_seq[cur_step_index]);

    // Decrement remaining count
    if (hstepper.remaining) hstepper.remaining--;

    // Speed ramp
    uint16_t delta = hstepper.accel_per / ACCEL_DIVISOR;
    if (delta == 0) delta = 1;

    if (hstepper.current_per < hstepper.target_per)
    {
        // Accelerating
        hstepper.current_per += delta;
        if (hstepper.current_per > hstepper.target_per)
            hstepper.current_per = hstepper.target_per;
    }
    else if (hstepper.current_per > hstepper.target_per)
    {
        // Decelerating
        if ((hstepper.current_per > STEPS_PER_S_MIN) && (hstepper.current_per > delta))
            hstepper.current_per -= delta;
        else
            hstepper.current_per = STEPS_PER_S_MIN;
    }

    // Start deceleration when approaching end
    if (hstepper.remaining == hstepper.acceleration)
        hstepper.target_per = STEPS_PER_S_MIN;

    // Update timer interval
    ATMR->ARR = stepsps_to_interval_ticks(hstepper.current_per);

    // Check completion
    if (hstepper.remaining == 0)
    {
        atmr_deinit();
        hstepper.busy = false;
        apply_step_pins(0);
    }
}

/**
 ****************************************************************************************
 * @brief ATMR interrupt handler.
 *
 * Clears interrupt flag first, then processes callback.
 * This ensures safe peripheral teardown from within the callback.
 ****************************************************************************************
 */
void ATMR_IRQHandler(void)
{
    if (ATMR->RIF.Word & TMR_IR_UI_BIT)
    {
        ATMR->ICR.Word = TMR_IR_UI_BIT;
        stepper_timer_callback();
    }
}
