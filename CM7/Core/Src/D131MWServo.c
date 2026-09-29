//#include "D131MWServo.h"
//#include "tim.h"
//#include <math.h>
//
///* ------------------------------------------------------------------------- */
///* User-tunable installation parameters                                      */
///* ------------------------------------------------------------------------- */
//
///* TIM2_CH1 is connected to the Hitec D131MW control wire. */
//#define HITEC_D131MW_TIM                         htim2
//#define HITEC_D131MW_CHANNEL                     TIM_CHANNEL_1
//
///*
// * Hitec D131MW PWM travel.
// *
// * Datasheet nominal travel:
// *   - one-side pulse travel: 400 us
// *   - one-side operating travel: 40 deg
// *
// * Tested extended travel used here:
// *   - center pulse: 1500 us
// *   - one-side pulse travel: 450 us
// *   - one-side operating travel: 45 deg
// *
// * Therefore:
// *   -45 deg -> 1050 us
// *     0 deg -> 1500 us
// *   +45 deg -> 1950 us
// */
//#define HITEC_D131MW_CENTER_US                   1500U
//#define HITEC_D131MW_MIN_US                      1050U
//#define HITEC_D131MW_MAX_US                      1950U
//#define HITEC_D131MW_ONE_SIDE_US                 450.0f
//
//#define HITEC_D131MW_MIN_DEG                     (-45.0f)
//#define HITEC_D131MW_MAX_DEG                     (45.0f)
//#define HITEC_D131MW_ONE_SIDE_DEG                45.0f
//
///* TDOA LS elevation mapping. */
//#define TDOA_LS_EL_MIN_DEG                       0.0f
//#define TDOA_LS_EL_MAX_DEG                       90.0f
//#define TDOA_LS_EL_CENTER_DEG                    45.0f
//#define TDOA_LS_EL_HALF_RANGE_DEG                45.0f
//
//#define HITEC_D131MW_ELEVATION_UPDATE_MS         20U
//#define HITEC_D131MW_ELEVATION_DEADBAND_DEG      1.0f
//
//volatile uint16_t g_d131mw_pulse_us = HITEC_D131MW_CENTER_US;
//volatile float g_d131mw_angle_deg = 0.0f;
//volatile float g_d131mw_tdoa_el_deg = TDOA_LS_EL_CENTER_DEG;
//
//static uint8_t s_el_initialized = 0U;
//static float s_last_tdoa_el_deg = 0.0f;
//static uint32_t s_last_update_tick = 0U;
//
//static float HitecD131MW_ClampFloat(float x, float min_v, float max_v)
//{
//    if (x < min_v)
//    {
//        return min_v;
//    }
//
//    if (x > max_v)
//    {
//        return max_v;
//    }
//
//    return x;
//}
//
//static float HitecD131MW_TdoaLsElevationToAngle(float tdoa_el_deg)
//{
//    float normalized;
//
//    tdoa_el_deg = HitecD131MW_ClampFloat(tdoa_el_deg,
//                                          TDOA_LS_EL_MIN_DEG,
//                                          TDOA_LS_EL_MAX_DEG);
//
//    /*
//     * TDOA LS 0 deg  -> D131MW -45 deg
//     * TDOA LS 45 deg -> D131MW   0 deg
//     * TDOA LS 90 deg -> D131MW +45 deg
//     */
//    normalized = (tdoa_el_deg - TDOA_LS_EL_CENTER_DEG) /
//                 TDOA_LS_EL_HALF_RANGE_DEG;
//
//    return normalized * HITEC_D131MW_ONE_SIDE_DEG;
//}
//
//void HitecD131MWServo_Init(void)
//{
//    (void)HAL_TIM_PWM_Start(&HITEC_D131MW_TIM, HITEC_D131MW_CHANNEL);
//
//    s_el_initialized = 0U;
//    s_last_tdoa_el_deg = 0.0f;
//    s_last_update_tick = HAL_GetTick();
//
//    HitecD131MWServo_SetPulseUs(HITEC_D131MW_CENTER_US);
//}
//
//void HitecD131MWServo_SetPulseUs(uint16_t pulse_us)
//{
//    if (pulse_us < HITEC_D131MW_MIN_US)
//    {
//        pulse_us = HITEC_D131MW_MIN_US;
//    }
//
//    if (pulse_us > HITEC_D131MW_MAX_US)
//    {
//        pulse_us = HITEC_D131MW_MAX_US;
//    }
//
//    g_d131mw_pulse_us = pulse_us;
//    __HAL_TIM_SET_COMPARE(&HITEC_D131MW_TIM, HITEC_D131MW_CHANNEL, pulse_us);
//}
//
//void HitecD131MWServo_SetAngle(float angle_deg)
//{
//    float pulse_f;
//    uint16_t pulse_us;
//
//    angle_deg = HitecD131MW_ClampFloat(angle_deg,
//                                        HITEC_D131MW_MIN_DEG,
//                                        HITEC_D131MW_MAX_DEG);
//
//    pulse_f = (float)HITEC_D131MW_CENTER_US +
//              ((angle_deg / HITEC_D131MW_ONE_SIDE_DEG) *
//               HITEC_D131MW_ONE_SIDE_US);
//    pulse_us = (uint16_t)(pulse_f + 0.5f);
//
//    g_d131mw_angle_deg = angle_deg;
//    HitecD131MWServo_SetPulseUs(pulse_us);
//}
//
//void HitecD131MWServo_SetTdoaLsElevation(float tdoa_el_deg)
//{
//    float servo_angle;
//
//    tdoa_el_deg = HitecD131MW_ClampFloat(tdoa_el_deg,
//                                          TDOA_LS_EL_MIN_DEG,
//                                          TDOA_LS_EL_MAX_DEG);
//    servo_angle = HitecD131MW_TdoaLsElevationToAngle(tdoa_el_deg);
//
//    g_d131mw_tdoa_el_deg = tdoa_el_deg;
//    HitecD131MWServo_SetAngle(servo_angle);
//}
//
//void HitecD131MWServo_SetTdoaLsElevationRealtime(float tdoa_el_deg)
//{
//    uint32_t now;
//    float diff;
//
//    now = HAL_GetTick();
//    tdoa_el_deg = HitecD131MW_ClampFloat(tdoa_el_deg,
//                                          TDOA_LS_EL_MIN_DEG,
//                                          TDOA_LS_EL_MAX_DEG);
//
//    if ((now - s_last_update_tick) < HITEC_D131MW_ELEVATION_UPDATE_MS)
//    {
//        return;
//    }
//
//    if (s_el_initialized != 0U)
//    {
//        diff = fabsf(tdoa_el_deg - s_last_tdoa_el_deg);
//        if (diff < HITEC_D131MW_ELEVATION_DEADBAND_DEG)
//        {
//            return;
//        }
//    }
//
//    HitecD131MWServo_SetTdoaLsElevation(tdoa_el_deg);
//
//    s_last_tdoa_el_deg = tdoa_el_deg;
//    s_last_update_tick = now;
//    s_el_initialized = 1U;
//}























































#include "D131MWServo.h"
#include "tim.h"
#include <math.h>

/* ------------------------------------------------------------------------- */
/* User-tunable installation parameters                                      */
/* ------------------------------------------------------------------------- */

/* TIM2_CH1 is connected to the Hitec D131MW control wire. */
#define HITEC_D131MW_TIM                         htim2
#define HITEC_D131MW_CHANNEL                     TIM_CHANNEL_1

/*
 * Hitec D131MW PWM travel.
 *
 * New operating range:
 *
 *   -25 deg -> 1250 us
 *     0 deg -> 1500 us
 *   +20 deg -> 1700 us
 *
 * Servo scale:
 *   10 us / deg
 */

#define HITEC_D131MW_CENTER_US                   1500U
#define HITEC_D131MW_MIN_US                      1250U
#define HITEC_D131MW_MAX_US                      1700U

#define HITEC_D131MW_US_PER_DEG                    10.0f

#define HITEC_D131MW_MIN_DEG                     (-25.0f)
#define HITEC_D131MW_MAX_DEG                      (20.0f)

/* TDOA LS elevation mapping. */
#define TDOA_LS_EL_MIN_DEG                         0.0f
#define TDOA_LS_EL_MAX_DEG                        90.0f
#define TDOA_LS_EL_CENTER_DEG                     45.0f

#define HITEC_D131MW_ELEVATION_UPDATE_MS          20U
#define HITEC_D131MW_ELEVATION_DEADBAND_DEG        0.5f
#define HITEC_D131MW_ELEVATION_STEP_DEG             1.5f

volatile uint16_t g_d131mw_pulse_us = HITEC_D131MW_CENTER_US;
volatile float g_d131mw_angle_deg = 0.0f;
volatile float g_d131mw_tdoa_el_deg = TDOA_LS_EL_CENTER_DEG;
volatile float g_d131mw_target_tdoa_el_deg = TDOA_LS_EL_CENTER_DEG;
volatile float g_d131mw_target_error_deg = 0.0f;
volatile uint32_t g_d131mw_target_valid = 0U;
volatile uint32_t g_d131mw_target_reached = 1U;
volatile uint32_t g_d131mw_task_update_count = 0U;

static uint32_t s_last_update_tick = 0U;

static float HitecD131MW_ClampFloat(float x, float min_v, float max_v)
{
    if (x < min_v)
    {
        return min_v;
    }

    if (x > max_v)
    {
        return max_v;
    }

    return x;
}

static float HitecD131MW_TdoaLsElevationToAngle(float tdoa_el_deg)
{
    float servo_angle_deg;

    tdoa_el_deg = HitecD131MW_ClampFloat(tdoa_el_deg,
                                         TDOA_LS_EL_MIN_DEG,
                                         TDOA_LS_EL_MAX_DEG);

    /*
     * TDOA LS elevation -> D131MW angle
     *
     *    0 deg -> -25 deg
     *   45 deg ->   0 deg
     *   90 deg -> +20 deg
     *
     * 45 deg를 기준으로 아래/위쪽 범위를 각각 별도로 스케일링한다.
     */
    if (tdoa_el_deg <= TDOA_LS_EL_CENTER_DEG)
    {
        /*
         * 0 ~ 45 deg
         *
         * 0  -> -25
         * 45 ->   0
         */
        servo_angle_deg =
            HITEC_D131MW_MIN_DEG +
            ((tdoa_el_deg - TDOA_LS_EL_MIN_DEG) /
             (TDOA_LS_EL_CENTER_DEG - TDOA_LS_EL_MIN_DEG)) *
            (0.0f - HITEC_D131MW_MIN_DEG);
    }
    else
    {
        /*
         * 45 ~ 90 deg
         *
         * 45 ->  0
         * 90 -> +20
         */
        servo_angle_deg =
            ((tdoa_el_deg - TDOA_LS_EL_CENTER_DEG) /
             (TDOA_LS_EL_MAX_DEG - TDOA_LS_EL_CENTER_DEG)) *
            HITEC_D131MW_MAX_DEG;
    }

    return HitecD131MW_ClampFloat(servo_angle_deg,
                                  HITEC_D131MW_MIN_DEG,
                                  HITEC_D131MW_MAX_DEG);
}

void HitecD131MWServo_Init(void)
{
    (void)HAL_TIM_PWM_Start(&HITEC_D131MW_TIM, HITEC_D131MW_CHANNEL);

    s_last_update_tick = HAL_GetTick();
    g_d131mw_target_tdoa_el_deg = TDOA_LS_EL_CENTER_DEG;
    g_d131mw_target_error_deg = 0.0f;
    g_d131mw_target_valid = 0U;
    g_d131mw_target_reached = 1U;
    g_d131mw_task_update_count = 0U;

    HitecD131MWServo_SetTdoaLsElevation(TDOA_LS_EL_CENTER_DEG);
}

void HitecD131MWServo_SetPulseUs(uint16_t pulse_us)
{
    if (pulse_us < HITEC_D131MW_MIN_US)
    {
        pulse_us = HITEC_D131MW_MIN_US;
    }

    if (pulse_us > HITEC_D131MW_MAX_US)
    {
        pulse_us = HITEC_D131MW_MAX_US;
    }

    g_d131mw_pulse_us = pulse_us;
    __HAL_TIM_SET_COMPARE(&HITEC_D131MW_TIM, HITEC_D131MW_CHANNEL, pulse_us);
}

void HitecD131MWServo_SetAngle(float angle_deg)
{
    float pulse_f;
    uint16_t pulse_us;

    angle_deg = HitecD131MW_ClampFloat(angle_deg,
                                       HITEC_D131MW_MIN_DEG,
                                       HITEC_D131MW_MAX_DEG);

    /*
     * 1 deg = 10 us
     *
     * -25 deg -> 1250 us
     *   0 deg -> 1500 us
     * +20 deg -> 1700 us
     */
    pulse_f =
        (float)HITEC_D131MW_CENTER_US +
        (angle_deg * HITEC_D131MW_US_PER_DEG);

    pulse_us = (uint16_t)(pulse_f + 0.5f);

    g_d131mw_angle_deg = angle_deg;

    HitecD131MWServo_SetPulseUs(pulse_us);
}

void HitecD131MWServo_SetTdoaLsElevation(float tdoa_el_deg)
{
    float servo_angle;

    tdoa_el_deg = HitecD131MW_ClampFloat(tdoa_el_deg,
                                          TDOA_LS_EL_MIN_DEG,
                                          TDOA_LS_EL_MAX_DEG);
    servo_angle = HitecD131MW_TdoaLsElevationToAngle(tdoa_el_deg);

    g_d131mw_tdoa_el_deg = tdoa_el_deg;
    HitecD131MWServo_SetAngle(servo_angle);
}

void HitecD131MWServo_SetTargetTdoaLsElevation(float tdoa_el_deg)
{
    tdoa_el_deg = HitecD131MW_ClampFloat(tdoa_el_deg,
                                          TDOA_LS_EL_MIN_DEG,
                                          TDOA_LS_EL_MAX_DEG);

    g_d131mw_target_tdoa_el_deg = tdoa_el_deg;
    g_d131mw_target_error_deg = tdoa_el_deg - g_d131mw_tdoa_el_deg;
    g_d131mw_target_valid = 1U;
    g_d131mw_target_reached =
        (fabsf(g_d131mw_target_error_deg) <= HITEC_D131MW_ELEVATION_DEADBAND_DEG) ? 1U : 0U;
}

void HitecD131MWServo_Task(void)
{
    uint32_t now;
    float current_el_deg;
    float target_el_deg;
    float error_deg;
    float next_el_deg;

    if (g_d131mw_target_valid == 0U)
    {
        return;
    }

    now = HAL_GetTick();
    if ((now - s_last_update_tick) < HITEC_D131MW_ELEVATION_UPDATE_MS)
    {
        return;
    }
    s_last_update_tick = now;

    current_el_deg = g_d131mw_tdoa_el_deg;
    target_el_deg = g_d131mw_target_tdoa_el_deg;
    error_deg = target_el_deg - current_el_deg;
    g_d131mw_target_error_deg = error_deg;

    if (fabsf(error_deg) <= HITEC_D131MW_ELEVATION_DEADBAND_DEG)
    {
        /* Send the exact final target once and then hold it. */
        if (current_el_deg != target_el_deg)
        {
            HitecD131MWServo_SetTdoaLsElevation(target_el_deg);
            g_d131mw_task_update_count++;
        }
        g_d131mw_target_error_deg = 0.0f;
        g_d131mw_target_reached = 1U;
        return;
    }

    /*
     * Move the command from the current commanded elevation toward the
     * latched target.  The source latch never changes here; only the motor
     * command progresses until it reaches that fixed target.
     */
    if (error_deg > HITEC_D131MW_ELEVATION_STEP_DEG)
    {
        next_el_deg = current_el_deg + HITEC_D131MW_ELEVATION_STEP_DEG;
    }
    else if (error_deg < -HITEC_D131MW_ELEVATION_STEP_DEG)
    {
        next_el_deg = current_el_deg - HITEC_D131MW_ELEVATION_STEP_DEG;
    }
    else
    {
        next_el_deg = target_el_deg;
    }

    HitecD131MWServo_SetTdoaLsElevation(next_el_deg);
    g_d131mw_target_error_deg = target_el_deg - g_d131mw_tdoa_el_deg;
    g_d131mw_target_reached =
        (fabsf(g_d131mw_target_error_deg) <= HITEC_D131MW_ELEVATION_DEADBAND_DEG) ? 1U : 0U;
    g_d131mw_task_update_count++;
}

void HitecD131MWServo_SetTdoaLsElevationRealtime(float tdoa_el_deg)
{
    /*
     * Compatibility API: do not drop a one-shot latch command because of the
     * 20 ms PWM period. Store it as a persistent target and let Task() finish
     * the move from the current commanded value.
     */
    HitecD131MWServo_SetTargetTdoaLsElevation(tdoa_el_deg);
    HitecD131MWServo_Task();
}
