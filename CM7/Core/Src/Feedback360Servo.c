//#include "Feedback360Servo.h"
//#include "tim.h"
//#include <math.h>
//
///* ------------------------------------------------------------------------- */
///* User-tunable installation parameters                                      */
///* ------------------------------------------------------------------------- */
//
///* TIM2_CH2 is connected to the servo white control wire. */
//#define FEEDBACK360_CONTROL_TIM              htim2
//#define FEEDBACK360_CONTROL_CHANNEL          TIM_CHANNEL_2
//
///* TIM3_CH1 receives the servo yellow feedback wire. TIM3_CH2 is configured
// * as indirect falling-edge capture from the same TI1 signal by this driver. */
//#define FEEDBACK360_FEEDBACK_TIM             htim3
//#define FEEDBACK360_FEEDBACK_PERIOD_CHANNEL  TIM_CHANNEL_1
//#define FEEDBACK360_FEEDBACK_HIGH_CHANNEL    TIM_CHANNEL_2
//
///* Parallax Feedback 360 control PWM. */
//#define FEEDBACK360_PWM_MIN_US               1280U
//#define FEEDBACK360_PWM_STOP_US              1500U
//#define FEEDBACK360_PWM_MAX_US               1720U
//#define FEEDBACK360_PWM_UPDATE_MS            20U
//
///* The feedback signal is about 910 Hz, approximately 1099 us period. */
//#define FEEDBACK360_PERIOD_MIN_US            800U
//#define FEEDBACK360_PERIOD_MAX_US            1400U
//#define FEEDBACK360_FEEDBACK_TIMEOUT_MS      150U
//
///* Feedback duty-cycle to absolute angle. */
//#define FEEDBACK360_DUTY_MIN                 0.027f
//#define FEEDBACK360_DUTY_MAX                 0.971f
//
///* Position controller. This is intentionally conservative for a first bring-up. */
//#define FEEDBACK360_DEADBAND_DEG             2.0f
//#define FEEDBACK360_KP_US_PER_DEG            2.2f
//#define FEEDBACK360_MIN_DRIVE_US             35.0f
//
///*
// * Mounting calibration.
// * - FEEDBACK360_ANGLE_OFFSET_DEG: add mechanical zero offset here.
// * - FEEDBACK360_FEEDBACK_INVERT: flip measured angle if feedback angle increases
// *   opposite to the RobotEar azimuth frame.
// * - FEEDBACK360_CONTROL_INVERT: flip motor command direction if the servo moves
// *   away from the target.
// */
//#define FEEDBACK360_ANGLE_OFFSET_DEG         0.0f
///*
// * Mirror the Feedback360 azimuth frame so the physical 90/270 deg positions
// * are exchanged while 0/180 deg stay unchanged:
// *   logical 0   -> physical 0
// *   logical 90  -> physical 270
// *   logical 180 -> physical 180
// *   logical 270 -> physical 90
// *
// * Feedback and control direction must be inverted together; otherwise the
// * position loop would drive away from the requested target.
// */
//#define FEEDBACK360_FEEDBACK_INVERT          1U
//#define FEEDBACK360_CONTROL_INVERT           1U
//
//volatile uint32_t g_feedback360_capture_count = 0U;
//volatile uint32_t g_feedback360_invalid_capture_count = 0U;
//volatile uint32_t g_feedback360_feedback_valid = 0U;
//volatile uint32_t g_feedback360_target_valid = 0U;
//volatile uint32_t g_feedback360_period_us = 0U;
//volatile uint32_t g_feedback360_high_us = 0U;
//volatile uint16_t g_feedback360_pulse_us = FEEDBACK360_PWM_STOP_US;
//volatile float g_feedback360_raw_angle_deg = 0.0f;
//volatile float g_feedback360_angle_deg = 0.0f;
//volatile float g_feedback360_target_deg = 0.0f;
//volatile float g_feedback360_error_deg = 0.0f;
//volatile float g_feedback360_command_us = 0.0f;
//
//static uint32_t s_last_capture_tick = 0U;
//static uint32_t s_last_control_tick = 0U;
//
//static float Feedback360_Wrap360(float deg)
//{
//    while (deg < 0.0f)
//    {
//        deg += 360.0f;
//    }
//
//    while (deg >= 360.0f)
//    {
//        deg -= 360.0f;
//    }
//
//    return deg;
//}
//
//static float Feedback360_AngleErrorDeg(float target_deg, float current_deg)
//{
//    float err;
//
//    target_deg = Feedback360_Wrap360(target_deg);
//    current_deg = Feedback360_Wrap360(current_deg);
//
//    err = target_deg - current_deg;
//
//    while (err > 180.0f)
//    {
//        err -= 360.0f;
//    }
//
//    while (err < -180.0f)
//    {
//        err += 360.0f;
//    }
//
//    return err;
//}
//
//static uint16_t Feedback360_ClampPulseUs(int32_t pulse_us)
//{
//    if (pulse_us < (int32_t)FEEDBACK360_PWM_MIN_US)
//    {
//        return FEEDBACK360_PWM_MIN_US;
//    }
//
//    if (pulse_us > (int32_t)FEEDBACK360_PWM_MAX_US)
//    {
//        return FEEDBACK360_PWM_MAX_US;
//    }
//
//    return (uint16_t)pulse_us;
//}
//
//static void Feedback360_SetPulseUs(uint16_t pulse_us)
//{
//    pulse_us = Feedback360_ClampPulseUs((int32_t)pulse_us);
//    g_feedback360_pulse_us = pulse_us;
//    __HAL_TIM_SET_COMPARE(&FEEDBACK360_CONTROL_TIM,
//                          FEEDBACK360_CONTROL_CHANNEL,
//                          pulse_us);
//}
//
//static float Feedback360_DutyToRawAngleDeg(uint32_t high_us, uint32_t period_us)
//{
//    uint32_t dc_x1000;
//    int32_t angle_i;
//
//    if (period_us == 0U)
//    {
//        return 0.0f;
//    }
//
//    /*
//     * Parallax Feedback 360 datasheet formula.
//     * dc_x1000: duty cycle scaled by 1000
//     * valid nominal duty range: 2.9%~97.1% -> 29~971
//     * angle = 359 - ((dc - dcMin) * 360) / (dcMax - dcMin + 1)
//     */
//    dc_x1000 = ((1000U * high_us) + (period_us / 2U)) / period_us;
//
//    if (dc_x1000 < 29U)
//    {
//        dc_x1000 = 29U;
//    }
//
//    if (dc_x1000 > 971U)
//    {
//        dc_x1000 = 971U;
//    }
//
//    angle_i = 359 - ((((int32_t)dc_x1000 - 29) * 360) / (971 - 29 + 1));
//
//    if (angle_i < 0)
//    {
//        angle_i = 0;
//    }
//    else if (angle_i > 359)
//    {
//        angle_i = 359;
//    }
//
//    return Feedback360_Wrap360((float)angle_i);
//}
//
//static float Feedback360_RawAngleToMotorAngleDeg(float raw_angle_deg)
//{
//    float angle = raw_angle_deg;
//
//#if (FEEDBACK360_FEEDBACK_INVERT != 0U)
//    angle = 360.0f - angle;
//#endif
//
//    angle += FEEDBACK360_ANGLE_OFFSET_DEG;
//
//    return Feedback360_Wrap360(angle);
//}
//
//static HAL_StatusTypeDef Feedback360_ConfigFeedbackPwmInput(void)
//{
//    TIM_IC_InitTypeDef sConfigIC = {0};
//    TIM_SlaveConfigTypeDef sSlaveConfig = {0};
//
//    (void)HAL_TIM_IC_Stop_IT(&FEEDBACK360_FEEDBACK_TIM,
//                             FEEDBACK360_FEEDBACK_PERIOD_CHANNEL);
//    (void)HAL_TIM_IC_Stop_IT(&FEEDBACK360_FEEDBACK_TIM,
//                             FEEDBACK360_FEEDBACK_HIGH_CHANNEL);
//
//    sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
//    sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
//    sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
//    sConfigIC.ICFilter = 0;
//    if (HAL_TIM_IC_ConfigChannel(&FEEDBACK360_FEEDBACK_TIM,
//                                 &sConfigIC,
//                                 FEEDBACK360_FEEDBACK_PERIOD_CHANNEL) != HAL_OK)
//    {
//        return HAL_ERROR;
//    }
//
//    sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_FALLING;
//    sConfigIC.ICSelection = TIM_ICSELECTION_INDIRECTTI;
//    sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
//    sConfigIC.ICFilter = 0;
//    if (HAL_TIM_IC_ConfigChannel(&FEEDBACK360_FEEDBACK_TIM,
//                                 &sConfigIC,
//                                 FEEDBACK360_FEEDBACK_HIGH_CHANNEL) != HAL_OK)
//    {
//        return HAL_ERROR;
//    }
//
//    sSlaveConfig.SlaveMode = TIM_SLAVEMODE_RESET;
//    sSlaveConfig.InputTrigger = TIM_TS_TI1FP1;
//    sSlaveConfig.TriggerPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
//    sSlaveConfig.TriggerPrescaler = TIM_ICPSC_DIV1;
//    sSlaveConfig.TriggerFilter = 0;
//    if (HAL_TIM_SlaveConfigSynchro(&FEEDBACK360_FEEDBACK_TIM,
//                                   &sSlaveConfig) != HAL_OK)
//    {
//        return HAL_ERROR;
//    }
//
//    return HAL_OK;
//}
//
//HAL_StatusTypeDef Feedback360Servo_Init(void)
//{
//    HAL_StatusTypeDef ret;
//
//    g_feedback360_target_valid = 0U;
//    g_feedback360_feedback_valid = 0U;
//    g_feedback360_error_deg = 0.0f;
//    g_feedback360_command_us = 0.0f;
//
//    ret = HAL_TIM_PWM_Start(&FEEDBACK360_CONTROL_TIM,
//                            FEEDBACK360_CONTROL_CHANNEL);
//    if (ret != HAL_OK)
//    {
//        return ret;
//    }
//
//    Feedback360_SetPulseUs(FEEDBACK360_PWM_STOP_US);
//
//    ret = Feedback360_ConfigFeedbackPwmInput();
//    if (ret != HAL_OK)
//    {
//        return ret;
//    }
//
//    __HAL_TIM_SET_COUNTER(&FEEDBACK360_FEEDBACK_TIM, 0U);
//
//    ret = HAL_TIM_IC_Start_IT(&FEEDBACK360_FEEDBACK_TIM,
//                              FEEDBACK360_FEEDBACK_HIGH_CHANNEL);
//    if (ret != HAL_OK)
//    {
//        return ret;
//    }
//
//    ret = HAL_TIM_IC_Start_IT(&FEEDBACK360_FEEDBACK_TIM,
//                              FEEDBACK360_FEEDBACK_PERIOD_CHANNEL);
//    if (ret != HAL_OK)
//    {
//        return ret;
//    }
//
//    s_last_capture_tick = HAL_GetTick();
//    s_last_control_tick = HAL_GetTick();
//
//    return HAL_OK;
//}
//
//void Feedback360Servo_Stop(void)
//{
//    g_feedback360_command_us = 0.0f;
//    Feedback360_SetPulseUs(FEEDBACK360_PWM_STOP_US);
//}
//
//void Feedback360Servo_ResetTarget(void)
//{
//    g_feedback360_target_valid = 0U;
//    g_feedback360_error_deg = 0.0f;
//    Feedback360Servo_Stop();
//}
//
//float Feedback360Servo_GetAngleDeg(void)
//{
//    return g_feedback360_angle_deg;
//}
//
//float Feedback360Servo_GetRawAngleDeg(void)
//{
//    return g_feedback360_raw_angle_deg;
//}
//
//uint8_t Feedback360Servo_IsFeedbackValid(void)
//{
//    return (uint8_t)g_feedback360_feedback_valid;
//}
//
//uint16_t Feedback360Servo_GetLastPulseUs(void)
//{
//    return g_feedback360_pulse_us;
//}
//
//void Feedback360Servo_SetRawPulseUs(uint16_t pulse_us)
//{
//    g_feedback360_target_valid = 0U;
//    g_feedback360_error_deg = 0.0f;
//    g_feedback360_command_us = 0.0f;
//    Feedback360_SetPulseUs(pulse_us);
//}
//
//HAL_StatusTypeDef Feedback360Servo_SetTargetAngle0To359(float target_deg)
//{
//    return Feedback360Servo_SetTargetAzimuthRealtime(target_deg);
//}
//
//HAL_StatusTypeDef Feedback360Servo_SetTargetAbsAngle(float target_abs_deg)
//{
//    /*
//     * This project uses the 360-degree feedback as an azimuth position sensor.
//     * Absolute multi-turn control is therefore folded to the closest 0~359 deg target.
//     */
//    return Feedback360Servo_SetTargetAzimuthRealtime(Feedback360_Wrap360(target_abs_deg));
//}
//
//HAL_StatusTypeDef Feedback360Servo_SetTargetAzimuthRealtime(float az_deg)
//{
//    uint32_t now;
//    float target_deg;
//    float current_deg;
//    float err_deg;
//    float command_us;
//    int32_t pulse_us;
//
//    now = HAL_GetTick();
//    target_deg = Feedback360_Wrap360(az_deg);
//
//    g_feedback360_target_deg = target_deg;
//    g_feedback360_target_valid = 1U;
//
//    if ((now - s_last_capture_tick) > FEEDBACK360_FEEDBACK_TIMEOUT_MS)
//    {
//        g_feedback360_feedback_valid = 0U;
//        Feedback360Servo_Stop();
//        return HAL_ERROR;
//    }
//
//    if (g_feedback360_feedback_valid == 0U)
//    {
//        Feedback360Servo_Stop();
//        return HAL_BUSY;
//    }
//
//    if ((now - s_last_control_tick) < FEEDBACK360_PWM_UPDATE_MS)
//    {
//        return HAL_BUSY;
//    }
//
//    current_deg = g_feedback360_angle_deg;
//    err_deg = Feedback360_AngleErrorDeg(target_deg, current_deg);
//    g_feedback360_error_deg = err_deg;
//
//    if (fabsf(err_deg) <= FEEDBACK360_DEADBAND_DEG)
//    {
//        g_feedback360_command_us = 0.0f;
//        Feedback360_SetPulseUs(FEEDBACK360_PWM_STOP_US);
//        s_last_control_tick = now;
//        return HAL_OK;
//    }
//
//    command_us = err_deg * FEEDBACK360_KP_US_PER_DEG;
//
//    if ((command_us > 0.0f) && (command_us < FEEDBACK360_MIN_DRIVE_US))
//    {
//        command_us = FEEDBACK360_MIN_DRIVE_US;
//    }
//    else if ((command_us < 0.0f) && (command_us > -FEEDBACK360_MIN_DRIVE_US))
//    {
//        command_us = -FEEDBACK360_MIN_DRIVE_US;
//    }
//
//#if (FEEDBACK360_CONTROL_INVERT != 0U)
//    command_us = -command_us;
//#endif
//
//    g_feedback360_command_us = command_us;
//    pulse_us = (int32_t)((float)FEEDBACK360_PWM_STOP_US + command_us);
//
//    Feedback360_SetPulseUs(Feedback360_ClampPulseUs(pulse_us));
//    s_last_control_tick = now;
//
//    return HAL_OK;
//}
//
//void Feedback360Servo_Task(void)
//{
//    uint32_t now = HAL_GetTick();
//
//    if ((now - s_last_capture_tick) > FEEDBACK360_FEEDBACK_TIMEOUT_MS)
//    {
//        g_feedback360_feedback_valid = 0U;
//        Feedback360Servo_Stop();
//        return;
//    }
//
//    if (g_feedback360_target_valid != 0U)
//    {
//        (void)Feedback360Servo_SetTargetAzimuthRealtime(g_feedback360_target_deg);
//    }
//}
//
//void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
//{
//    uint32_t period_us;
//    uint32_t high_us;
//    float raw_angle_deg;
//
//    if (htim->Instance != FEEDBACK360_FEEDBACK_TIM.Instance)
//    {
//        return;
//    }
//
//    if (htim->Channel != HAL_TIM_ACTIVE_CHANNEL_1)
//    {
//        return;
//    }
//
//    period_us = HAL_TIM_ReadCapturedValue(htim,
//                                          FEEDBACK360_FEEDBACK_PERIOD_CHANNEL);
//    high_us = HAL_TIM_ReadCapturedValue(htim,
//                                        FEEDBACK360_FEEDBACK_HIGH_CHANNEL);
//
//    if ((period_us < FEEDBACK360_PERIOD_MIN_US) ||
//        (period_us > FEEDBACK360_PERIOD_MAX_US) ||
//        (high_us == 0U) ||
//        (high_us >= period_us))
//    {
//        g_feedback360_invalid_capture_count++;
//        return;
//    }
//
//    raw_angle_deg = Feedback360_DutyToRawAngleDeg(high_us, period_us);
//
//    g_feedback360_period_us = period_us;
//    g_feedback360_high_us = high_us;
//    g_feedback360_raw_angle_deg = raw_angle_deg;
//    g_feedback360_angle_deg = Feedback360_RawAngleToMotorAngleDeg(raw_angle_deg);
//    g_feedback360_feedback_valid = 1U;
//    g_feedback360_capture_count++;
//
//    s_last_capture_tick = HAL_GetTick();
//}






































//#include "Feedback360Servo.h"
//#include "tim.h"
//#include <math.h>
//
///* ------------------------------------------------------------------------- */
///* User-tunable installation parameters                                      */
///* ------------------------------------------------------------------------- */
//
///* TIM2_CH2 is connected to the servo white control wire. */
//#define FEEDBACK360_CONTROL_TIM              htim2
//#define FEEDBACK360_CONTROL_CHANNEL          TIM_CHANNEL_2
//
///* TIM3_CH1 receives the servo yellow feedback wire. TIM3_CH2 is configured
// * as indirect falling-edge capture from the same TI1 signal by this driver. */
//#define FEEDBACK360_FEEDBACK_TIM             htim3
//#define FEEDBACK360_FEEDBACK_PERIOD_CHANNEL  TIM_CHANNEL_1
//#define FEEDBACK360_FEEDBACK_HIGH_CHANNEL    TIM_CHANNEL_2
//
///* Parallax Feedback 360 control PWM. */
//#define FEEDBACK360_PWM_MIN_US               1280U
//#define FEEDBACK360_PWM_STOP_US              1500U
//#define FEEDBACK360_PWM_MAX_US               1720U
//#define FEEDBACK360_PWM_UPDATE_MS            20U
//
///* The feedback signal is about 910 Hz, approximately 1099 us period. */
//#define FEEDBACK360_PERIOD_MIN_US            800U
//#define FEEDBACK360_PERIOD_MAX_US            1400U
//#define FEEDBACK360_FEEDBACK_TIMEOUT_MS      150U
//
///* Feedback duty-cycle to absolute angle. */
//#define FEEDBACK360_DUTY_MIN                 0.027f
//#define FEEDBACK360_DUTY_MAX                 0.971f
//
///* ------------------------------------------------------------------------- */
///* Position + velocity (PD) controller                                       */
///* ------------------------------------------------------------------------- */
///*
// * The control PWM is a SPEED command, not a position command.  Therefore,
// * position-only P control can reach the target with residual angular speed and
// * overshoot it.  The D term estimates angular velocity from the absolute
// * feedback angle and starts reducing/reversing the speed command before the
// * target is crossed.
// *
// * Initial tuning values for the current RobotEar mechanics:
// *   P = position pull toward the target
// *   D = velocity braking
// */
//#define FEEDBACK360_KP_US_PER_DEG            2.20f
//#define FEEDBACK360_KD_US_PER_DEG_S          0.18f
//
///* Velocity estimate is low-pass filtered because the feedback angle is
// * quantized to approximately 1 degree by the current duty conversion. */
//#define FEEDBACK360_VELOCITY_LPF_ALPHA       0.25f
//#define FEEDBACK360_VELOCITY_RESET_MS        200U
//
///* A target is considered physically settled only when BOTH position and
// * angular velocity are small. */
//#define FEEDBACK360_SETTLE_POS_DEG           1.0f
//#define FEEDBACK360_SETTLE_VEL_DEG_S         8.0f
//
///* Continuous approach profile.  The servo receives one continuous command;
// * there is no ON/OFF pulse sequence.  Near the target the maximum command is
// * reduced, while a small effective minimum is retained so that static friction
// * and the servo neutral dead zone do not leave several degrees of error. */
//#define FEEDBACK360_ZONE_FAR_DEG             30.0f
//#define FEEDBACK360_ZONE_MID_DEG             15.0f
//#define FEEDBACK360_ZONE_NEAR_DEG             7.0f
//
//#define FEEDBACK360_MIN_DRIVE_FAR_US         55.0f
//#define FEEDBACK360_MIN_DRIVE_MID_US         45.0f
//#define FEEDBACK360_MIN_DRIVE_NEAR_US        38.0f
//#define FEEDBACK360_MIN_DRIVE_FINE_US        32.0f
//
//#define FEEDBACK360_MAX_DRIVE_FAR_US        180.0f
//#define FEEDBACK360_MAX_DRIVE_MID_US        110.0f
//#define FEEDBACK360_MAX_DRIVE_NEAR_US        70.0f
//#define FEEDBACK360_MAX_DRIVE_FINE_US        50.0f
//
///* When the shaft is already inside the position tolerance but still moving,
// * command a short opposite-direction braking torque instead of declaring the
// * move complete merely because it passed through the target. */
//#define FEEDBACK360_BRAKE_MIN_US             32.0f
//#define FEEDBACK360_BRAKE_MAX_US             45.0f
//
///*
// * Mounting calibration.
// * - FEEDBACK360_ANGLE_OFFSET_DEG: add mechanical zero offset here.
// * - FEEDBACK360_FEEDBACK_INVERT: flip measured angle if feedback angle increases
// *   opposite to the RobotEar azimuth frame.
// * - FEEDBACK360_CONTROL_INVERT: flip motor command direction if the servo moves
// *   away from the target.
// */
//#define FEEDBACK360_ANGLE_OFFSET_DEG         0.0f
///*
// * Mirror the Feedback360 azimuth frame so the physical 90/270 deg positions
// * are exchanged while 0/180 deg stay unchanged:
// *   logical 0   -> physical 0
// *   logical 90  -> physical 270
// *   logical 180 -> physical 180
// *   logical 270 -> physical 90
// *
// * Feedback and control direction must be inverted together; otherwise the
// * position loop would drive away from the requested target.
// */
//#define FEEDBACK360_FEEDBACK_INVERT          1U
//#define FEEDBACK360_CONTROL_INVERT           1U
//
//volatile uint32_t g_feedback360_capture_count = 0U;
//volatile uint32_t g_feedback360_invalid_capture_count = 0U;
//volatile uint32_t g_feedback360_feedback_valid = 0U;
//volatile uint32_t g_feedback360_target_valid = 0U;
//volatile uint32_t g_feedback360_period_us = 0U;
//volatile uint32_t g_feedback360_high_us = 0U;
//volatile uint16_t g_feedback360_pulse_us = FEEDBACK360_PWM_STOP_US;
//volatile float g_feedback360_raw_angle_deg = 0.0f;
//volatile float g_feedback360_angle_deg = 0.0f;
//volatile float g_feedback360_target_deg = 0.0f;
//volatile float g_feedback360_error_deg = 0.0f;
//volatile float g_feedback360_command_us = 0.0f;
//
///* PD controller debug/watch variables. */
//volatile float g_feedback360_velocity_raw_deg_s = 0.0f;
//volatile float g_feedback360_velocity_deg_s = 0.0f;
//volatile float g_feedback360_pd_p_us = 0.0f;
//volatile float g_feedback360_pd_d_us = 0.0f;
//volatile uint32_t g_feedback360_brake_active = 0U;
//volatile uint32_t g_feedback360_settled = 0U;
//
//static uint32_t s_last_capture_tick = 0U;
//static uint32_t s_last_control_tick = 0U;
//static uint32_t s_velocity_prev_tick = 0U;
//static float s_velocity_prev_angle_deg = 0.0f;
//static uint8_t s_velocity_initialized = 0U;
//
//static float Feedback360_Wrap360(float deg)
//{
//    while (deg < 0.0f)
//    {
//        deg += 360.0f;
//    }
//
//    while (deg >= 360.0f)
//    {
//        deg -= 360.0f;
//    }
//
//    return deg;
//}
//
//static float Feedback360_AngleErrorDeg(float target_deg, float current_deg)
//{
//    float err;
//
//    target_deg = Feedback360_Wrap360(target_deg);
//    current_deg = Feedback360_Wrap360(current_deg);
//
//    err = target_deg - current_deg;
//
//    while (err > 180.0f)
//    {
//        err -= 360.0f;
//    }
//
//    while (err < -180.0f)
//    {
//        err += 360.0f;
//    }
//
//    return err;
//}
//
//static void Feedback360_ResetVelocityEstimator(void)
//{
//    s_velocity_prev_tick = 0U;
//    s_velocity_prev_angle_deg = g_feedback360_angle_deg;
//    s_velocity_initialized = 0U;
//    g_feedback360_velocity_raw_deg_s = 0.0f;
//    g_feedback360_velocity_deg_s = 0.0f;
//}
//
//static float Feedback360_UpdateVelocityDegPerSec(float current_deg, uint32_t now)
//{
//    uint32_t dt_ms;
//    float delta_deg;
//    float raw_velocity_deg_s;
//
//    if (s_velocity_initialized == 0U)
//    {
//        s_velocity_initialized = 1U;
//        s_velocity_prev_tick = now;
//        s_velocity_prev_angle_deg = current_deg;
//        g_feedback360_velocity_raw_deg_s = 0.0f;
//        g_feedback360_velocity_deg_s = 0.0f;
//        return 0.0f;
//    }
//
//    dt_ms = now - s_velocity_prev_tick;
//
//    if ((dt_ms == 0U) || (dt_ms > FEEDBACK360_VELOCITY_RESET_MS))
//    {
//        s_velocity_prev_tick = now;
//        s_velocity_prev_angle_deg = current_deg;
//        g_feedback360_velocity_raw_deg_s = 0.0f;
//        g_feedback360_velocity_deg_s = 0.0f;
//        return 0.0f;
//    }
//
//    /* current - previous, wrapped to the shortest signed angular motion. */
//    delta_deg = Feedback360_AngleErrorDeg(current_deg,
//                                           s_velocity_prev_angle_deg);
//    raw_velocity_deg_s = delta_deg * (1000.0f / (float)dt_ms);
//
//    g_feedback360_velocity_raw_deg_s = raw_velocity_deg_s;
//    g_feedback360_velocity_deg_s +=
//        FEEDBACK360_VELOCITY_LPF_ALPHA *
//        (raw_velocity_deg_s - g_feedback360_velocity_deg_s);
//
//    s_velocity_prev_tick = now;
//    s_velocity_prev_angle_deg = current_deg;
//
//    return g_feedback360_velocity_deg_s;
//}
//
//static float Feedback360_LimitSignedCommandUs(float command_us,
//                                               float min_drive_us,
//                                               float max_drive_us)
//{
//    if (command_us > 0.0f)
//    {
//        if ((min_drive_us > 0.0f) && (command_us < min_drive_us))
//        {
//            command_us = min_drive_us;
//        }
//        if (command_us > max_drive_us)
//        {
//            command_us = max_drive_us;
//        }
//    }
//    else if (command_us < 0.0f)
//    {
//        if ((min_drive_us > 0.0f) && (command_us > -min_drive_us))
//        {
//            command_us = -min_drive_us;
//        }
//        if (command_us < -max_drive_us)
//        {
//            command_us = -max_drive_us;
//        }
//    }
//
//    return command_us;
//}
//
//static uint16_t Feedback360_ClampPulseUs(int32_t pulse_us)
//{
//    if (pulse_us < (int32_t)FEEDBACK360_PWM_MIN_US)
//    {
//        return FEEDBACK360_PWM_MIN_US;
//    }
//
//    if (pulse_us > (int32_t)FEEDBACK360_PWM_MAX_US)
//    {
//        return FEEDBACK360_PWM_MAX_US;
//    }
//
//    return (uint16_t)pulse_us;
//}
//
//static void Feedback360_SetPulseUs(uint16_t pulse_us)
//{
//    pulse_us = Feedback360_ClampPulseUs((int32_t)pulse_us);
//    g_feedback360_pulse_us = pulse_us;
//    __HAL_TIM_SET_COMPARE(&FEEDBACK360_CONTROL_TIM,
//                          FEEDBACK360_CONTROL_CHANNEL,
//                          pulse_us);
//}
//
//static float Feedback360_DutyToRawAngleDeg(uint32_t high_us, uint32_t period_us)
//{
//    uint32_t dc_x1000;
//    int32_t angle_i;
//
//    if (period_us == 0U)
//    {
//        return 0.0f;
//    }
//
//    /*
//     * Parallax Feedback 360 datasheet formula.
//     * dc_x1000: duty cycle scaled by 1000
//     * valid nominal duty range: 2.9%~97.1% -> 29~971
//     * angle = 359 - ((dc - dcMin) * 360) / (dcMax - dcMin + 1)
//     */
//    dc_x1000 = ((1000U * high_us) + (period_us / 2U)) / period_us;
//
//    if (dc_x1000 < 29U)
//    {
//        dc_x1000 = 29U;
//    }
//
//    if (dc_x1000 > 971U)
//    {
//        dc_x1000 = 971U;
//    }
//
//    angle_i = 359 - ((((int32_t)dc_x1000 - 29) * 360) / (971 - 29 + 1));
//
//    if (angle_i < 0)
//    {
//        angle_i = 0;
//    }
//    else if (angle_i > 359)
//    {
//        angle_i = 359;
//    }
//
//    return Feedback360_Wrap360((float)angle_i);
//}
//
//static float Feedback360_RawAngleToMotorAngleDeg(float raw_angle_deg)
//{
//    float angle = raw_angle_deg;
//
//#if (FEEDBACK360_FEEDBACK_INVERT != 0U)
//    angle = 360.0f - angle;
//#endif
//
//    angle += FEEDBACK360_ANGLE_OFFSET_DEG;
//
//    return Feedback360_Wrap360(angle);
//}
//
//static HAL_StatusTypeDef Feedback360_ConfigFeedbackPwmInput(void)
//{
//    TIM_IC_InitTypeDef sConfigIC = {0};
//    TIM_SlaveConfigTypeDef sSlaveConfig = {0};
//
//    (void)HAL_TIM_IC_Stop_IT(&FEEDBACK360_FEEDBACK_TIM,
//                             FEEDBACK360_FEEDBACK_PERIOD_CHANNEL);
//    (void)HAL_TIM_IC_Stop_IT(&FEEDBACK360_FEEDBACK_TIM,
//                             FEEDBACK360_FEEDBACK_HIGH_CHANNEL);
//
//    sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
//    sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
//    sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
//    sConfigIC.ICFilter = 0;
//    if (HAL_TIM_IC_ConfigChannel(&FEEDBACK360_FEEDBACK_TIM,
//                                 &sConfigIC,
//                                 FEEDBACK360_FEEDBACK_PERIOD_CHANNEL) != HAL_OK)
//    {
//        return HAL_ERROR;
//    }
//
//    sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_FALLING;
//    sConfigIC.ICSelection = TIM_ICSELECTION_INDIRECTTI;
//    sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
//    sConfigIC.ICFilter = 0;
//    if (HAL_TIM_IC_ConfigChannel(&FEEDBACK360_FEEDBACK_TIM,
//                                 &sConfigIC,
//                                 FEEDBACK360_FEEDBACK_HIGH_CHANNEL) != HAL_OK)
//    {
//        return HAL_ERROR;
//    }
//
//    sSlaveConfig.SlaveMode = TIM_SLAVEMODE_RESET;
//    sSlaveConfig.InputTrigger = TIM_TS_TI1FP1;
//    sSlaveConfig.TriggerPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
//    sSlaveConfig.TriggerPrescaler = TIM_ICPSC_DIV1;
//    sSlaveConfig.TriggerFilter = 0;
//    if (HAL_TIM_SlaveConfigSynchro(&FEEDBACK360_FEEDBACK_TIM,
//                                   &sSlaveConfig) != HAL_OK)
//    {
//        return HAL_ERROR;
//    }
//
//    return HAL_OK;
//}
//
//HAL_StatusTypeDef Feedback360Servo_Init(void)
//{
//    HAL_StatusTypeDef ret;
//
//    g_feedback360_target_valid = 0U;
//    g_feedback360_feedback_valid = 0U;
//    g_feedback360_error_deg = 0.0f;
//    g_feedback360_command_us = 0.0f;
//    g_feedback360_pd_p_us = 0.0f;
//    g_feedback360_pd_d_us = 0.0f;
//    g_feedback360_brake_active = 0U;
//    g_feedback360_settled = 0U;
//    Feedback360_ResetVelocityEstimator();
//
//    ret = HAL_TIM_PWM_Start(&FEEDBACK360_CONTROL_TIM,
//                            FEEDBACK360_CONTROL_CHANNEL);
//    if (ret != HAL_OK)
//    {
//        return ret;
//    }
//
//    Feedback360_SetPulseUs(FEEDBACK360_PWM_STOP_US);
//
//    ret = Feedback360_ConfigFeedbackPwmInput();
//    if (ret != HAL_OK)
//    {
//        return ret;
//    }
//
//    __HAL_TIM_SET_COUNTER(&FEEDBACK360_FEEDBACK_TIM, 0U);
//
//    ret = HAL_TIM_IC_Start_IT(&FEEDBACK360_FEEDBACK_TIM,
//                              FEEDBACK360_FEEDBACK_HIGH_CHANNEL);
//    if (ret != HAL_OK)
//    {
//        return ret;
//    }
//
//    ret = HAL_TIM_IC_Start_IT(&FEEDBACK360_FEEDBACK_TIM,
//                              FEEDBACK360_FEEDBACK_PERIOD_CHANNEL);
//    if (ret != HAL_OK)
//    {
//        return ret;
//    }
//
//    s_last_capture_tick = HAL_GetTick();
//    s_last_control_tick = HAL_GetTick();
//
//    return HAL_OK;
//}
//
//void Feedback360Servo_Stop(void)
//{
//    g_feedback360_command_us = 0.0f;
//    Feedback360_SetPulseUs(FEEDBACK360_PWM_STOP_US);
//}
//
//void Feedback360Servo_ResetTarget(void)
//{
//    g_feedback360_target_valid = 0U;
//    g_feedback360_error_deg = 0.0f;
//    g_feedback360_pd_p_us = 0.0f;
//    g_feedback360_pd_d_us = 0.0f;
//    g_feedback360_brake_active = 0U;
//    g_feedback360_settled = 0U;
//    Feedback360_ResetVelocityEstimator();
//    Feedback360Servo_Stop();
//}
//
//float Feedback360Servo_GetAngleDeg(void)
//{
//    return g_feedback360_angle_deg;
//}
//
//float Feedback360Servo_GetRawAngleDeg(void)
//{
//    return g_feedback360_raw_angle_deg;
//}
//
//uint8_t Feedback360Servo_IsFeedbackValid(void)
//{
//    return (uint8_t)g_feedback360_feedback_valid;
//}
//
//uint16_t Feedback360Servo_GetLastPulseUs(void)
//{
//    return g_feedback360_pulse_us;
//}
//
//void Feedback360Servo_SetRawPulseUs(uint16_t pulse_us)
//{
//    g_feedback360_target_valid = 0U;
//    g_feedback360_error_deg = 0.0f;
//    g_feedback360_command_us = 0.0f;
//    g_feedback360_pd_p_us = 0.0f;
//    g_feedback360_pd_d_us = 0.0f;
//    g_feedback360_brake_active = 0U;
//    g_feedback360_settled = 0U;
//    Feedback360_ResetVelocityEstimator();
//    Feedback360_SetPulseUs(pulse_us);
//}
//
//HAL_StatusTypeDef Feedback360Servo_SetTargetAngle0To359(float target_deg)
//{
//    return Feedback360Servo_SetTargetAzimuthRealtime(target_deg);
//}
//
//HAL_StatusTypeDef Feedback360Servo_SetTargetAbsAngle(float target_abs_deg)
//{
//    /*
//     * This project uses the 360-degree feedback as an azimuth position sensor.
//     * Absolute multi-turn control is therefore folded to the closest 0~359 deg target.
//     */
//    return Feedback360Servo_SetTargetAzimuthRealtime(Feedback360_Wrap360(target_abs_deg));
//}
//
//HAL_StatusTypeDef Feedback360Servo_SetTargetAzimuthRealtime(float az_deg)
//{
//    uint32_t now;
//    float target_deg;
//    float current_deg;
//    float err_deg;
//    float abs_err_deg;
//    float velocity_deg_s;
//    float abs_velocity_deg_s;
//    float p_us;
//    float d_us;
//    float command_us;
//    float min_drive_us;
//    float max_drive_us;
//    int32_t pulse_us;
//
//    now = HAL_GetTick();
//    target_deg = Feedback360_Wrap360(az_deg);
//
//    g_feedback360_target_deg = target_deg;
//    g_feedback360_target_valid = 1U;
//
//    if ((now - s_last_capture_tick) > FEEDBACK360_FEEDBACK_TIMEOUT_MS)
//    {
//        g_feedback360_feedback_valid = 0U;
//        g_feedback360_settled = 0U;
//        Feedback360Servo_Stop();
//        return HAL_ERROR;
//    }
//
//    if (g_feedback360_feedback_valid == 0U)
//    {
//        g_feedback360_settled = 0U;
//        Feedback360Servo_Stop();
//        return HAL_BUSY;
//    }
//
//    if ((now - s_last_control_tick) < FEEDBACK360_PWM_UPDATE_MS)
//    {
//        return HAL_BUSY;
//    }
//
//    current_deg = g_feedback360_angle_deg;
//    err_deg = Feedback360_AngleErrorDeg(target_deg, current_deg);
//    abs_err_deg = fabsf(err_deg);
//    velocity_deg_s = Feedback360_UpdateVelocityDegPerSec(current_deg, now);
//    abs_velocity_deg_s = fabsf(velocity_deg_s);
//
//    g_feedback360_error_deg = err_deg;
//    g_feedback360_brake_active = 0U;
//    g_feedback360_settled = 0U;
//
//    /*
//     * Do not call a pass-through of the target "arrived".  A move is complete
//     * only after the shaft is both close to the requested angle and slow.
//     */
//    if ((abs_err_deg <= FEEDBACK360_SETTLE_POS_DEG) &&
//        (abs_velocity_deg_s <= FEEDBACK360_SETTLE_VEL_DEG_S))
//    {
//        g_feedback360_pd_p_us = 0.0f;
//        g_feedback360_pd_d_us = 0.0f;
//        g_feedback360_command_us = 0.0f;
//        g_feedback360_settled = 1U;
//        Feedback360_SetPulseUs(FEEDBACK360_PWM_STOP_US);
//        s_last_control_tick = now;
//        return HAL_OK;
//    }
//
//    /* PD in the logical angle frame.  The control inversion is applied later. */
//    p_us = FEEDBACK360_KP_US_PER_DEG * err_deg;
//    d_us = -FEEDBACK360_KD_US_PER_DEG_S * velocity_deg_s;
//    command_us = p_us + d_us;
//
//    g_feedback360_pd_p_us = p_us;
//    g_feedback360_pd_d_us = d_us;
//
//    if (abs_err_deg > FEEDBACK360_ZONE_FAR_DEG)
//    {
//        min_drive_us = FEEDBACK360_MIN_DRIVE_FAR_US;
//        max_drive_us = FEEDBACK360_MAX_DRIVE_FAR_US;
//    }
//    else if (abs_err_deg > FEEDBACK360_ZONE_MID_DEG)
//    {
//        min_drive_us = FEEDBACK360_MIN_DRIVE_MID_US;
//        max_drive_us = FEEDBACK360_MAX_DRIVE_MID_US;
//    }
//    else if (abs_err_deg > FEEDBACK360_ZONE_NEAR_DEG)
//    {
//        min_drive_us = FEEDBACK360_MIN_DRIVE_NEAR_US;
//        max_drive_us = FEEDBACK360_MAX_DRIVE_NEAR_US;
//    }
//    else
//    {
//        min_drive_us = FEEDBACK360_MIN_DRIVE_FINE_US;
//        max_drive_us = FEEDBACK360_MAX_DRIVE_FINE_US;
//    }
//
//    /*
//     * Inside the final +/-1 deg window, position is already good.  If angular
//     * speed is still high, actively command the opposite direction so the
//     * shaft loses speed before it crosses far beyond the target.
//     */
//    if ((abs_err_deg <= FEEDBACK360_SETTLE_POS_DEG) &&
//        (abs_velocity_deg_s > FEEDBACK360_SETTLE_VEL_DEG_S))
//    {
//        g_feedback360_brake_active = 1U;
//        min_drive_us = FEEDBACK360_BRAKE_MIN_US;
//        max_drive_us = FEEDBACK360_BRAKE_MAX_US;
//
//        if (fabsf(command_us) < FEEDBACK360_BRAKE_MIN_US)
//        {
//            if (velocity_deg_s > 0.0f)
//            {
//                command_us = -FEEDBACK360_BRAKE_MIN_US;
//            }
//            else if (velocity_deg_s < 0.0f)
//            {
//                command_us = FEEDBACK360_BRAKE_MIN_US;
//            }
//        }
//    }
//
//    command_us = Feedback360_LimitSignedCommandUs(command_us,
//                                                   min_drive_us,
//                                                   max_drive_us);
//
//#if (FEEDBACK360_CONTROL_INVERT != 0U)
//    command_us = -command_us;
//#endif
//
//    g_feedback360_command_us = command_us;
//    pulse_us = (int32_t)((float)FEEDBACK360_PWM_STOP_US + command_us);
//
//    Feedback360_SetPulseUs(Feedback360_ClampPulseUs(pulse_us));
//    s_last_control_tick = now;
//
//    return HAL_OK;
//}
//
//void Feedback360Servo_Task(void)
//{
//    uint32_t now = HAL_GetTick();
//
//    if ((now - s_last_capture_tick) > FEEDBACK360_FEEDBACK_TIMEOUT_MS)
//    {
//        g_feedback360_feedback_valid = 0U;
//        Feedback360Servo_Stop();
//        return;
//    }
//
//    if (g_feedback360_target_valid != 0U)
//    {
//        (void)Feedback360Servo_SetTargetAzimuthRealtime(g_feedback360_target_deg);
//    }
//}
//
//void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
//{
//    uint32_t period_us;
//    uint32_t high_us;
//    float raw_angle_deg;
//
//    if (htim->Instance != FEEDBACK360_FEEDBACK_TIM.Instance)
//    {
//        return;
//    }
//
//    if (htim->Channel != HAL_TIM_ACTIVE_CHANNEL_1)
//    {
//        return;
//    }
//
//    period_us = HAL_TIM_ReadCapturedValue(htim,
//                                          FEEDBACK360_FEEDBACK_PERIOD_CHANNEL);
//    high_us = HAL_TIM_ReadCapturedValue(htim,
//                                        FEEDBACK360_FEEDBACK_HIGH_CHANNEL);
//
//    if ((period_us < FEEDBACK360_PERIOD_MIN_US) ||
//        (period_us > FEEDBACK360_PERIOD_MAX_US) ||
//        (high_us == 0U) ||
//        (high_us >= period_us))
//    {
//        g_feedback360_invalid_capture_count++;
//        return;
//    }
//
//    raw_angle_deg = Feedback360_DutyToRawAngleDeg(high_us, period_us);
//
//    g_feedback360_period_us = period_us;
//    g_feedback360_high_us = high_us;
//    g_feedback360_raw_angle_deg = raw_angle_deg;
//    g_feedback360_angle_deg = Feedback360_RawAngleToMotorAngleDeg(raw_angle_deg);
//    g_feedback360_feedback_valid = 1U;
//    g_feedback360_capture_count++;
//
//    s_last_capture_tick = HAL_GetTick();
//}
























































//#include "Feedback360Servo.h"
//#include "tim.h"
//#include <math.h>
//
///* ------------------------------------------------------------------------- */
///* User-tunable installation parameters                                      */
///* ------------------------------------------------------------------------- */
//
///* TIM2_CH2 is connected to the servo white control wire. */
//#define FEEDBACK360_CONTROL_TIM              htim2
//#define FEEDBACK360_CONTROL_CHANNEL          TIM_CHANNEL_2
//
///* TIM3_CH1 receives the servo yellow feedback wire. TIM3_CH2 is configured
// * as indirect falling-edge capture from the same TI1 signal by this driver. */
//#define FEEDBACK360_FEEDBACK_TIM             htim3
//#define FEEDBACK360_FEEDBACK_PERIOD_CHANNEL  TIM_CHANNEL_1
//#define FEEDBACK360_FEEDBACK_HIGH_CHANNEL    TIM_CHANNEL_2
//
///* Parallax Feedback 360 control PWM. */
//#define FEEDBACK360_PWM_MIN_US               1280U
//#define FEEDBACK360_PWM_STOP_US              1500U
//#define FEEDBACK360_PWM_MAX_US               1720U
//#define FEEDBACK360_PWM_UPDATE_MS            20U
//
///* The feedback signal is about 910 Hz, approximately 1099 us period. */
//#define FEEDBACK360_PERIOD_MIN_US            800U
//#define FEEDBACK360_PERIOD_MAX_US            1400U
//#define FEEDBACK360_FEEDBACK_TIMEOUT_MS      150U
//
///* Feedback duty-cycle to absolute angle. */
//#define FEEDBACK360_DUTY_MIN                 0.027f
//#define FEEDBACK360_DUTY_MAX                 0.971f
//
///* ------------------------------------------------------------------------- */
///* Position + velocity (PD) controller                                       */
///* ------------------------------------------------------------------------- */
///*
// * The control PWM is a SPEED command, not a position command.  Therefore,
// * position-only P control can reach the target with residual angular speed and
// * overshoot it.  The D term estimates angular velocity from the absolute
// * feedback angle and starts reducing/reversing the speed command before the
// * target is crossed.
// *
// * Initial tuning values for the current RobotEar mechanics:
// *   P = position pull toward the target
// *   D = velocity braking
// */
//#define FEEDBACK360_KP_US_PER_DEG            2.20f
//#define FEEDBACK360_KD_US_PER_DEG_S          0.18f
//
///* Velocity estimate is low-pass filtered because the feedback angle is
// * quantized to approximately 1 degree by the current duty conversion. */
//#define FEEDBACK360_VELOCITY_LPF_ALPHA       0.25f
//#define FEEDBACK360_VELOCITY_RESET_MS        200U
//
///*
// * Anti-jitter stop hysteresis.
// *
// * Once the shaft enters HOLD_ENTER, force the exact neutral PWM and latch the
// * stopped state.  Do not re-apply drive for 1-degree feedback quantization or
// * small mechanical bounce.  Motion resumes only after the error leaves the
// * larger HOLD_EXIT window.
// */
//#define FEEDBACK360_HOLD_ENTER_DEG            1.5f
//#define FEEDBACK360_HOLD_EXIT_DEG             3.0f
//
///* Continuous approach profile.  The servo receives one continuous command;
// * there is no ON/OFF pulse sequence.  Near the target the maximum command is
// * reduced, while a small effective minimum is retained so that static friction
// * and the servo neutral dead zone do not leave several degrees of error. */
//#define FEEDBACK360_ZONE_FAR_DEG             30.0f
//#define FEEDBACK360_ZONE_MID_DEG             15.0f
//#define FEEDBACK360_ZONE_NEAR_DEG             7.0f
//
//#define FEEDBACK360_MIN_DRIVE_FAR_US         55.0f
//#define FEEDBACK360_MIN_DRIVE_MID_US         45.0f
//#define FEEDBACK360_MIN_DRIVE_NEAR_US        38.0f
//#define FEEDBACK360_MIN_DRIVE_FINE_US        32.0f
//
//#define FEEDBACK360_MAX_DRIVE_FAR_US        180.0f
//#define FEEDBACK360_MAX_DRIVE_MID_US        110.0f
//#define FEEDBACK360_MAX_DRIVE_NEAR_US        70.0f
//#define FEEDBACK360_MAX_DRIVE_FINE_US        50.0f
//
///*
// * Do not use an active reverse-brake in the final stop window.  On this
// * continuous-rotation servo, reverse braking plus quantized feedback can make
// * the command alternate around neutral and show up as visible shaking.
// */
//
///*
// * Mounting calibration.
// * - FEEDBACK360_ANGLE_OFFSET_DEG: add mechanical zero offset here.
// * - FEEDBACK360_FEEDBACK_INVERT: flip measured angle if feedback angle increases
// *   opposite to the RobotEar azimuth frame.
// * - FEEDBACK360_CONTROL_INVERT: flip motor command direction if the servo moves
// *   away from the target.
// */
//#define FEEDBACK360_ANGLE_OFFSET_DEG         0.0f
///*
// * Mirror the Feedback360 azimuth frame so the physical 90/270 deg positions
// * are exchanged while 0/180 deg stay unchanged:
// *   logical 0   -> physical 0
// *   logical 90  -> physical 270
// *   logical 180 -> physical 180
// *   logical 270 -> physical 90
// *
// * Feedback and control direction must be inverted together; otherwise the
// * position loop would drive away from the requested target.
// */
//#define FEEDBACK360_FEEDBACK_INVERT          0U
//#define FEEDBACK360_CONTROL_INVERT           0U
//
//volatile uint32_t g_feedback360_capture_count = 0U;
//volatile uint32_t g_feedback360_invalid_capture_count = 0U;
//volatile uint32_t g_feedback360_feedback_valid = 0U;
//volatile uint32_t g_feedback360_target_valid = 0U;
//volatile uint32_t g_feedback360_period_us = 0U;
//volatile uint32_t g_feedback360_high_us = 0U;
//volatile uint16_t g_feedback360_pulse_us = FEEDBACK360_PWM_STOP_US;
//volatile float g_feedback360_raw_angle_deg = 0.0f;
//volatile float g_feedback360_angle_deg = 0.0f;
//volatile float g_feedback360_target_deg = 0.0f;
//volatile float g_feedback360_error_deg = 0.0f;
//volatile float g_feedback360_command_us = 0.0f;
//
///* PD controller debug/watch variables. */
//volatile float g_feedback360_velocity_raw_deg_s = 0.0f;
//volatile float g_feedback360_velocity_deg_s = 0.0f;
//volatile float g_feedback360_pd_p_us = 0.0f;
//volatile float g_feedback360_pd_d_us = 0.0f;
//volatile uint32_t g_feedback360_brake_active = 0U;
//volatile uint32_t g_feedback360_settled = 0U;
//volatile uint32_t g_feedback360_hold_active = 0U;
//
//static uint32_t s_last_capture_tick = 0U;
//static uint32_t s_last_control_tick = 0U;
//static uint32_t s_velocity_prev_tick = 0U;
//static float s_velocity_prev_angle_deg = 0.0f;
//static uint8_t s_velocity_initialized = 0U;
//static uint8_t s_hold_active = 0U;
//
//static float Feedback360_Wrap360(float deg)
//{
//    while (deg < 0.0f)
//    {
//        deg += 360.0f;
//    }
//
//    while (deg >= 360.0f)
//    {
//        deg -= 360.0f;
//    }
//
//    return deg;
//}
//
//static float Feedback360_AngleErrorDeg(float target_deg, float current_deg)
//{
//    float err;
//
//    target_deg = Feedback360_Wrap360(target_deg);
//    current_deg = Feedback360_Wrap360(current_deg);
//
//    err = target_deg - current_deg;
//
//    while (err > 180.0f)
//    {
//        err -= 360.0f;
//    }
//
//    while (err < -180.0f)
//    {
//        err += 360.0f;
//    }
//
//    return err;
//}
//
//static void Feedback360_ResetVelocityEstimator(void)
//{
//    s_velocity_prev_tick = 0U;
//    s_velocity_prev_angle_deg = g_feedback360_angle_deg;
//    s_velocity_initialized = 0U;
//    g_feedback360_velocity_raw_deg_s = 0.0f;
//    g_feedback360_velocity_deg_s = 0.0f;
//}
//
//static float Feedback360_UpdateVelocityDegPerSec(float current_deg, uint32_t now)
//{
//    uint32_t dt_ms;
//    float delta_deg;
//    float raw_velocity_deg_s;
//
//    if (s_velocity_initialized == 0U)
//    {
//        s_velocity_initialized = 1U;
//        s_velocity_prev_tick = now;
//        s_velocity_prev_angle_deg = current_deg;
//        g_feedback360_velocity_raw_deg_s = 0.0f;
//        g_feedback360_velocity_deg_s = 0.0f;
//        return 0.0f;
//    }
//
//    dt_ms = now - s_velocity_prev_tick;
//
//    if ((dt_ms == 0U) || (dt_ms > FEEDBACK360_VELOCITY_RESET_MS))
//    {
//        s_velocity_prev_tick = now;
//        s_velocity_prev_angle_deg = current_deg;
//        g_feedback360_velocity_raw_deg_s = 0.0f;
//        g_feedback360_velocity_deg_s = 0.0f;
//        return 0.0f;
//    }
//
//    /* current - previous, wrapped to the shortest signed angular motion. */
//    delta_deg = Feedback360_AngleErrorDeg(current_deg,
//                                           s_velocity_prev_angle_deg);
//    raw_velocity_deg_s = delta_deg * (1000.0f / (float)dt_ms);
//
//    g_feedback360_velocity_raw_deg_s = raw_velocity_deg_s;
//    g_feedback360_velocity_deg_s +=
//        FEEDBACK360_VELOCITY_LPF_ALPHA *
//        (raw_velocity_deg_s - g_feedback360_velocity_deg_s);
//
//    s_velocity_prev_tick = now;
//    s_velocity_prev_angle_deg = current_deg;
//
//    return g_feedback360_velocity_deg_s;
//}
//
//static float Feedback360_LimitSignedCommandUs(float command_us,
//                                               float min_drive_us,
//                                               float max_drive_us)
//{
//    if (command_us > 0.0f)
//    {
//        if ((min_drive_us > 0.0f) && (command_us < min_drive_us))
//        {
//            command_us = min_drive_us;
//        }
//        if (command_us > max_drive_us)
//        {
//            command_us = max_drive_us;
//        }
//    }
//    else if (command_us < 0.0f)
//    {
//        if ((min_drive_us > 0.0f) && (command_us > -min_drive_us))
//        {
//            command_us = -min_drive_us;
//        }
//        if (command_us < -max_drive_us)
//        {
//            command_us = -max_drive_us;
//        }
//    }
//
//    return command_us;
//}
//
//static uint16_t Feedback360_ClampPulseUs(int32_t pulse_us)
//{
//    if (pulse_us < (int32_t)FEEDBACK360_PWM_MIN_US)
//    {
//        return FEEDBACK360_PWM_MIN_US;
//    }
//
//    if (pulse_us > (int32_t)FEEDBACK360_PWM_MAX_US)
//    {
//        return FEEDBACK360_PWM_MAX_US;
//    }
//
//    return (uint16_t)pulse_us;
//}
//
//static void Feedback360_SetPulseUs(uint16_t pulse_us)
//{
//    pulse_us = Feedback360_ClampPulseUs((int32_t)pulse_us);
//    g_feedback360_pulse_us = pulse_us;
//    __HAL_TIM_SET_COMPARE(&FEEDBACK360_CONTROL_TIM,
//                          FEEDBACK360_CONTROL_CHANNEL,
//                          pulse_us);
//}
//
//static float Feedback360_DutyToRawAngleDeg(uint32_t high_us, uint32_t period_us)
//{
//    uint32_t dc_x1000;
//    int32_t angle_i;
//
//    if (period_us == 0U)
//    {
//        return 0.0f;
//    }
//
//    /*
//     * Parallax Feedback 360 datasheet formula.
//     * dc_x1000: duty cycle scaled by 1000
//     * valid nominal duty range: 2.9%~97.1% -> 29~971
//     * angle = 359 - ((dc - dcMin) * 360) / (dcMax - dcMin + 1)
//     */
//    dc_x1000 = ((1000U * high_us) + (period_us / 2U)) / period_us;
//
//    if (dc_x1000 < 29U)
//    {
//        dc_x1000 = 29U;
//    }
//
//    if (dc_x1000 > 971U)
//    {
//        dc_x1000 = 971U;
//    }
//
//    angle_i = 359 - ((((int32_t)dc_x1000 - 29) * 360) / (971 - 29 + 1));
//
//    if (angle_i < 0)
//    {
//        angle_i = 0;
//    }
//    else if (angle_i > 359)
//    {
//        angle_i = 359;
//    }
//
//    return Feedback360_Wrap360((float)angle_i);
//}
//
//static float Feedback360_RawAngleToMotorAngleDeg(float raw_angle_deg)
//{
//    float angle = raw_angle_deg;
//
//#if (FEEDBACK360_FEEDBACK_INVERT != 0U)
//    angle = 360.0f - angle;
//#endif
//
//    angle += FEEDBACK360_ANGLE_OFFSET_DEG;
//
//    return Feedback360_Wrap360(angle);
//}
//
//static HAL_StatusTypeDef Feedback360_ConfigFeedbackPwmInput(void)
//{
//    TIM_IC_InitTypeDef sConfigIC = {0};
//    TIM_SlaveConfigTypeDef sSlaveConfig = {0};
//
//    (void)HAL_TIM_IC_Stop_IT(&FEEDBACK360_FEEDBACK_TIM,
//                             FEEDBACK360_FEEDBACK_PERIOD_CHANNEL);
//    (void)HAL_TIM_IC_Stop_IT(&FEEDBACK360_FEEDBACK_TIM,
//                             FEEDBACK360_FEEDBACK_HIGH_CHANNEL);
//
//    sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
//    sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
//    sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
//    sConfigIC.ICFilter = 0;
//    if (HAL_TIM_IC_ConfigChannel(&FEEDBACK360_FEEDBACK_TIM,
//                                 &sConfigIC,
//                                 FEEDBACK360_FEEDBACK_PERIOD_CHANNEL) != HAL_OK)
//    {
//        return HAL_ERROR;
//    }
//
//    sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_FALLING;
//    sConfigIC.ICSelection = TIM_ICSELECTION_INDIRECTTI;
//    sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
//    sConfigIC.ICFilter = 0;
//    if (HAL_TIM_IC_ConfigChannel(&FEEDBACK360_FEEDBACK_TIM,
//                                 &sConfigIC,
//                                 FEEDBACK360_FEEDBACK_HIGH_CHANNEL) != HAL_OK)
//    {
//        return HAL_ERROR;
//    }
//
//    sSlaveConfig.SlaveMode = TIM_SLAVEMODE_RESET;
//    sSlaveConfig.InputTrigger = TIM_TS_TI1FP1;
//    sSlaveConfig.TriggerPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
//    sSlaveConfig.TriggerPrescaler = TIM_ICPSC_DIV1;
//    sSlaveConfig.TriggerFilter = 0;
//    if (HAL_TIM_SlaveConfigSynchro(&FEEDBACK360_FEEDBACK_TIM,
//                                   &sSlaveConfig) != HAL_OK)
//    {
//        return HAL_ERROR;
//    }
//
//    return HAL_OK;
//}
//
//HAL_StatusTypeDef Feedback360Servo_Init(void)
//{
//    HAL_StatusTypeDef ret;
//
//    g_feedback360_target_valid = 0U;
//    g_feedback360_feedback_valid = 0U;
//    g_feedback360_error_deg = 0.0f;
//    g_feedback360_command_us = 0.0f;
//    g_feedback360_pd_p_us = 0.0f;
//    g_feedback360_pd_d_us = 0.0f;
//    g_feedback360_brake_active = 0U;
//    g_feedback360_settled = 0U;
//    g_feedback360_hold_active = 0U;
//    s_hold_active = 0U;
//    Feedback360_ResetVelocityEstimator();
//
//    ret = HAL_TIM_PWM_Start(&FEEDBACK360_CONTROL_TIM,
//                            FEEDBACK360_CONTROL_CHANNEL);
//    if (ret != HAL_OK)
//    {
//        return ret;
//    }
//
//    Feedback360_SetPulseUs(FEEDBACK360_PWM_STOP_US);
//
//    ret = Feedback360_ConfigFeedbackPwmInput();
//    if (ret != HAL_OK)
//    {
//        return ret;
//    }
//
//    __HAL_TIM_SET_COUNTER(&FEEDBACK360_FEEDBACK_TIM, 0U);
//
//    ret = HAL_TIM_IC_Start_IT(&FEEDBACK360_FEEDBACK_TIM,
//                              FEEDBACK360_FEEDBACK_HIGH_CHANNEL);
//    if (ret != HAL_OK)
//    {
//        return ret;
//    }
//
//    ret = HAL_TIM_IC_Start_IT(&FEEDBACK360_FEEDBACK_TIM,
//                              FEEDBACK360_FEEDBACK_PERIOD_CHANNEL);
//    if (ret != HAL_OK)
//    {
//        return ret;
//    }
//
//    s_last_capture_tick = HAL_GetTick();
//    s_last_control_tick = HAL_GetTick();
//
//    return HAL_OK;
//}
//
//void Feedback360Servo_Stop(void)
//{
//    g_feedback360_command_us = 0.0f;
//    Feedback360_SetPulseUs(FEEDBACK360_PWM_STOP_US);
//}
//
//void Feedback360Servo_ResetTarget(void)
//{
//    g_feedback360_target_valid = 0U;
//    g_feedback360_error_deg = 0.0f;
//    g_feedback360_pd_p_us = 0.0f;
//    g_feedback360_pd_d_us = 0.0f;
//    g_feedback360_brake_active = 0U;
//    g_feedback360_settled = 0U;
//    g_feedback360_hold_active = 0U;
//    s_hold_active = 0U;
//    Feedback360_ResetVelocityEstimator();
//    Feedback360Servo_Stop();
//}
//
//float Feedback360Servo_GetAngleDeg(void)
//{
//    return g_feedback360_angle_deg;
//}
//
//float Feedback360Servo_GetRawAngleDeg(void)
//{
//    return g_feedback360_raw_angle_deg;
//}
//
//uint8_t Feedback360Servo_IsFeedbackValid(void)
//{
//    return (uint8_t)g_feedback360_feedback_valid;
//}
//
//uint16_t Feedback360Servo_GetLastPulseUs(void)
//{
//    return g_feedback360_pulse_us;
//}
//
//void Feedback360Servo_SetRawPulseUs(uint16_t pulse_us)
//{
//    g_feedback360_target_valid = 0U;
//    g_feedback360_error_deg = 0.0f;
//    g_feedback360_command_us = 0.0f;
//    g_feedback360_pd_p_us = 0.0f;
//    g_feedback360_pd_d_us = 0.0f;
//    g_feedback360_brake_active = 0U;
//    g_feedback360_settled = 0U;
//    g_feedback360_hold_active = 0U;
//    s_hold_active = 0U;
//    Feedback360_ResetVelocityEstimator();
//    Feedback360_SetPulseUs(pulse_us);
//}
//
//HAL_StatusTypeDef Feedback360Servo_SetTargetAngle0To359(float target_deg)
//{
//    return Feedback360Servo_SetTargetAzimuthRealtime(target_deg);
//}
//
//HAL_StatusTypeDef Feedback360Servo_SetTargetAbsAngle(float target_abs_deg)
//{
//    /*
//     * This project uses the 360-degree feedback as an azimuth position sensor.
//     * Absolute multi-turn control is therefore folded to the closest 0~359 deg target.
//     */
//    return Feedback360Servo_SetTargetAzimuthRealtime(Feedback360_Wrap360(target_abs_deg));
//}
//
//HAL_StatusTypeDef Feedback360Servo_SetTargetAzimuthRealtime(float az_deg)
//{
//    uint32_t now;
//    float target_deg;
//    float current_deg;
//    float err_deg;
//    float abs_err_deg;
//    float velocity_deg_s;
//    float p_us;
//    float d_us;
//    float command_us;
//    float min_drive_us;
//    float max_drive_us;
//    int32_t pulse_us;
//
//    now = HAL_GetTick();
//    target_deg = Feedback360_Wrap360(az_deg);
//
//    g_feedback360_target_deg = target_deg;
//    g_feedback360_target_valid = 1U;
//
//    if ((now - s_last_capture_tick) > FEEDBACK360_FEEDBACK_TIMEOUT_MS)
//    {
//        g_feedback360_feedback_valid = 0U;
//        g_feedback360_settled = 0U;
//        Feedback360Servo_Stop();
//        return HAL_ERROR;
//    }
//
//    if (g_feedback360_feedback_valid == 0U)
//    {
//        g_feedback360_settled = 0U;
//        Feedback360Servo_Stop();
//        return HAL_BUSY;
//    }
//
//    if ((now - s_last_control_tick) < FEEDBACK360_PWM_UPDATE_MS)
//    {
//        return HAL_BUSY;
//    }
//
//    current_deg = g_feedback360_angle_deg;
//    err_deg = Feedback360_AngleErrorDeg(target_deg, current_deg);
//    abs_err_deg = fabsf(err_deg);
//    velocity_deg_s = Feedback360_UpdateVelocityDegPerSec(current_deg, now);
//
//    g_feedback360_error_deg = err_deg;
//    g_feedback360_brake_active = 0U;
//    g_feedback360_settled = 0U;
//
//    /*
//     * Anti-jitter HOLD (Schmitt-trigger style hysteresis).
//     *
//     * - Enter stop/hold at <= 1.5 deg.
//     * - Once stopped, stay at the exact neutral PWM while error <= 3.0 deg.
//     * - Re-drive only when a real target/mechanical change exceeds 3.0 deg.
//     *
//     * This prevents feedback quantization and tiny shaft bounce from repeatedly
//     * generating +/- minimum-drive commands around the target.
//     */
//    if (s_hold_active != 0U)
//    {
//        if (abs_err_deg <= FEEDBACK360_HOLD_EXIT_DEG)
//        {
//            g_feedback360_pd_p_us = 0.0f;
//            g_feedback360_pd_d_us = 0.0f;
//            g_feedback360_command_us = 0.0f;
//            g_feedback360_brake_active = 0U;
//            g_feedback360_settled = 1U;
//            g_feedback360_hold_active = 1U;
//            Feedback360_SetPulseUs(FEEDBACK360_PWM_STOP_US);
//            s_last_control_tick = now;
//            return HAL_OK;
//        }
//
//        s_hold_active = 0U;
//        g_feedback360_hold_active = 0U;
//        g_feedback360_settled = 0U;
//        Feedback360_ResetVelocityEstimator();
//        velocity_deg_s = 0.0f;
//    }
//
//    if (abs_err_deg <= FEEDBACK360_HOLD_ENTER_DEG)
//    {
//        s_hold_active = 1U;
//        g_feedback360_hold_active = 1U;
//        g_feedback360_pd_p_us = 0.0f;
//        g_feedback360_pd_d_us = 0.0f;
//        g_feedback360_command_us = 0.0f;
//        g_feedback360_brake_active = 0U;
//        g_feedback360_settled = 1U;
//        Feedback360_SetPulseUs(FEEDBACK360_PWM_STOP_US);
//        s_last_control_tick = now;
//        return HAL_OK;
//    }
//
//    /* PD in the logical angle frame.  The control inversion is applied later. */
//    p_us = FEEDBACK360_KP_US_PER_DEG * err_deg;
//    d_us = -FEEDBACK360_KD_US_PER_DEG_S * velocity_deg_s;
//    command_us = p_us + d_us;
//
//    g_feedback360_pd_p_us = p_us;
//    g_feedback360_pd_d_us = d_us;
//
//    if (abs_err_deg > FEEDBACK360_ZONE_FAR_DEG)
//    {
//        min_drive_us = FEEDBACK360_MIN_DRIVE_FAR_US;
//        max_drive_us = FEEDBACK360_MAX_DRIVE_FAR_US;
//    }
//    else if (abs_err_deg > FEEDBACK360_ZONE_MID_DEG)
//    {
//        min_drive_us = FEEDBACK360_MIN_DRIVE_MID_US;
//        max_drive_us = FEEDBACK360_MAX_DRIVE_MID_US;
//    }
//    else if (abs_err_deg > FEEDBACK360_ZONE_NEAR_DEG)
//    {
//        min_drive_us = FEEDBACK360_MIN_DRIVE_NEAR_US;
//        max_drive_us = FEEDBACK360_MAX_DRIVE_NEAR_US;
//    }
//    else
//    {
//        min_drive_us = FEEDBACK360_MIN_DRIVE_FINE_US;
//        max_drive_us = FEEDBACK360_MAX_DRIVE_FINE_US;
//    }
//
//    command_us = Feedback360_LimitSignedCommandUs(command_us,
//                                                   min_drive_us,
//                                                   max_drive_us);
//
//#if (FEEDBACK360_CONTROL_INVERT != 0U)
//    command_us = -command_us;
//#endif
//
//    g_feedback360_command_us = command_us;
//    pulse_us = (int32_t)((float)FEEDBACK360_PWM_STOP_US + command_us);
//
//    Feedback360_SetPulseUs(Feedback360_ClampPulseUs(pulse_us));
//    s_last_control_tick = now;
//
//    return HAL_OK;
//}
//
//void Feedback360Servo_Task(void)
//{
//    uint32_t now = HAL_GetTick();
//
//    if ((now - s_last_capture_tick) > FEEDBACK360_FEEDBACK_TIMEOUT_MS)
//    {
//        g_feedback360_feedback_valid = 0U;
//        Feedback360Servo_Stop();
//        return;
//    }
//
//    if (g_feedback360_target_valid != 0U)
//    {
//        (void)Feedback360Servo_SetTargetAzimuthRealtime(g_feedback360_target_deg);
//    }
//}
//
//void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
//{
//    uint32_t period_us;
//    uint32_t high_us;
//    float raw_angle_deg;
//
//    if (htim->Instance != FEEDBACK360_FEEDBACK_TIM.Instance)
//    {
//        return;
//    }
//
//    if (htim->Channel != HAL_TIM_ACTIVE_CHANNEL_1)
//    {
//        return;
//    }
//
//    period_us = HAL_TIM_ReadCapturedValue(htim,
//                                          FEEDBACK360_FEEDBACK_PERIOD_CHANNEL);
//    high_us = HAL_TIM_ReadCapturedValue(htim,
//                                        FEEDBACK360_FEEDBACK_HIGH_CHANNEL);
//
//    if ((period_us < FEEDBACK360_PERIOD_MIN_US) ||
//        (period_us > FEEDBACK360_PERIOD_MAX_US) ||
//        (high_us == 0U) ||
//        (high_us >= period_us))
//    {
//        g_feedback360_invalid_capture_count++;
//        return;
//    }
//
//    raw_angle_deg = Feedback360_DutyToRawAngleDeg(high_us, period_us);
//
//    g_feedback360_period_us = period_us;
//    g_feedback360_high_us = high_us;
//    g_feedback360_raw_angle_deg = raw_angle_deg;
//    g_feedback360_angle_deg = Feedback360_RawAngleToMotorAngleDeg(raw_angle_deg);
//    g_feedback360_feedback_valid = 1U;
//    g_feedback360_capture_count++;
//
//    s_last_capture_tick = HAL_GetTick();
//}






















































#include "Feedback360Servo.h"
#include "tim.h"
#include <math.h>

/* ------------------------------------------------------------------------- */
/* User-tunable installation parameters                                      */
/* ------------------------------------------------------------------------- */

/* TIM2_CH2 is connected to the servo white control wire. */
#define FEEDBACK360_CONTROL_TIM              htim2
#define FEEDBACK360_CONTROL_CHANNEL          TIM_CHANNEL_2

/* TIM3_CH1 receives the servo yellow feedback wire. TIM3_CH2 is configured
 * as indirect falling-edge capture from the same TI1 signal by this driver. */
#define FEEDBACK360_FEEDBACK_TIM             htim3
#define FEEDBACK360_FEEDBACK_PERIOD_CHANNEL  TIM_CHANNEL_1
#define FEEDBACK360_FEEDBACK_HIGH_CHANNEL    TIM_CHANNEL_2

/* Parallax Feedback 360 control PWM. */
#define FEEDBACK360_PWM_MIN_US               1280U
#define FEEDBACK360_PWM_STOP_US              1500U
#define FEEDBACK360_PWM_MAX_US               1720U
#define FEEDBACK360_PWM_UPDATE_MS            20U

/* The feedback signal is about 910 Hz, approximately 1099 us period. */
#define FEEDBACK360_PERIOD_MIN_US            800U
#define FEEDBACK360_PERIOD_MAX_US            1400U
#define FEEDBACK360_FEEDBACK_TIMEOUT_MS      150U

/* Feedback duty-cycle to absolute angle. */
#define FEEDBACK360_DUTY_MIN                 0.027f
#define FEEDBACK360_DUTY_MAX                 0.971f

/* ------------------------------------------------------------------------- */
/* Position + velocity (PD) controller                                       */
/* ------------------------------------------------------------------------- */
/*
 * The control PWM is a SPEED command, not a position command.  Therefore,
 * position-only P control can reach the target with residual angular speed and
 * overshoot it.  The D term estimates angular velocity from the absolute
 * feedback angle and starts reducing/reversing the speed command before the
 * target is crossed.
 *
 * Initial tuning values for the current RobotEar mechanics:
 *   P = position pull toward the target
 *   D = velocity braking
 */
#define FEEDBACK360_KP_US_PER_DEG            2.20f
#define FEEDBACK360_KD_US_PER_DEG_S          0.18f

/* Velocity estimate is low-pass filtered because the feedback angle is
 * quantized to approximately 1 degree by the current duty conversion. */
#define FEEDBACK360_VELOCITY_LPF_ALPHA       0.25f
#define FEEDBACK360_VELOCITY_RESET_MS        200U

/*
 * Anti-jitter stop hysteresis.
 *
 * Once the shaft enters HOLD_ENTER, force the exact neutral PWM and latch the
 * stopped state.  Do not re-apply drive for 1-degree feedback quantization or
 * small mechanical bounce.  Motion resumes only after the error leaves the
 * larger HOLD_EXIT window.
 */
#define FEEDBACK360_HOLD_ENTER_DEG            1.5f
#define FEEDBACK360_HOLD_EXIT_DEG             3.0f

/* Continuous approach profile.  The servo receives one continuous command;
 * there is no ON/OFF pulse sequence.  Near the target the maximum command is
 * reduced, while a small effective minimum is retained so that static friction
 * and the servo neutral dead zone do not leave several degrees of error. */
#define FEEDBACK360_ZONE_FAR_DEG             30.0f
#define FEEDBACK360_ZONE_MID_DEG             15.0f
#define FEEDBACK360_ZONE_NEAR_DEG             7.0f

#define FEEDBACK360_MIN_DRIVE_FAR_US         55.0f
#define FEEDBACK360_MIN_DRIVE_MID_US         45.0f
#define FEEDBACK360_MIN_DRIVE_NEAR_US        38.0f
#define FEEDBACK360_MIN_DRIVE_FINE_US        32.0f

#define FEEDBACK360_MAX_DRIVE_FAR_US        180.0f
#define FEEDBACK360_MAX_DRIVE_MID_US        110.0f
#define FEEDBACK360_MAX_DRIVE_NEAR_US        70.0f
#define FEEDBACK360_MAX_DRIVE_FINE_US        50.0f

/*
 * Do not use an active reverse-brake in the final stop window.  On this
 * continuous-rotation servo, reverse braking plus quantized feedback can make
 * the command alternate around neutral and show up as visible shaking.
 */

/*
 * Mounting calibration.
 * - FEEDBACK360_ANGLE_OFFSET_DEG: add mechanical zero offset here.
 * - FEEDBACK360_FEEDBACK_INVERT: flip measured angle if feedback angle increases
 *   opposite to the RobotEar azimuth frame.
 * - FEEDBACK360_CONTROL_INVERT: flip motor command direction if the servo moves
 *   away from the target.
 */
#define FEEDBACK360_ANGLE_OFFSET_DEG         0.0f
/*
 * Mirror the Feedback360 azimuth frame so the physical 90/270 deg positions
 * are exchanged while 0/180 deg stay unchanged:
 *   logical 0   -> physical 0
 *   logical 90  -> physical 270
 *   logical 180 -> physical 180
 *   logical 270 -> physical 90
 *
 * Feedback and control direction must be inverted together; otherwise the
 * position loop would drive away from the requested target.
 */
#define FEEDBACK360_FEEDBACK_INVERT          0U
#define FEEDBACK360_CONTROL_INVERT           0U

/* ------------------------------------------------------------------------- */
/* Cable-safe multi-turn management                                           */
/* ------------------------------------------------------------------------- */
/*
 * The feedback sensor itself reports only 0~359 deg, but the cable can still
 * accumulate real turns.  Keep an unwrapped angle and always represent a new
 * azimuth target inside the cable-neutral target window.
 *
 * Examples with TARGET_LIMIT = 220 deg:
 *   270 deg -> -90 deg  (same physical orientation, only 90 deg travel)
 *   350 deg -> -10 deg
 *   180 deg -> +180 or -180; exact tie alternates direction.
 *
 * TARGET_LIMIT is the normal cable-safe command window.  WARNING/HARD are
 * additional safety margins for overshoot or an unexpected state.  Normal
 * selected targets never intentionally exceed +/- TARGET_LIMIT.
 */
#define FEEDBACK360_CABLE_TARGET_LIMIT_DEG   220.0f
#define FEEDBACK360_CABLE_WARNING_DEG        220.0f
#define FEEDBACK360_CABLE_HARD_LIMIT_DEG     270.0f
#define FEEDBACK360_CABLE_TARGET_EPS_DEG       0.25f
#define FEEDBACK360_CABLE_SAME_TARGET_EPS_DEG  0.10f

/* Logical direction used for the first exact 180-deg tie.
 * -1 = decreasing logical angle, +1 = increasing logical angle. */
#define FEEDBACK360_CABLE_FIRST_180_DIR       (-1)

volatile uint32_t g_feedback360_capture_count = 0U;
volatile uint32_t g_feedback360_invalid_capture_count = 0U;
volatile uint32_t g_feedback360_feedback_valid = 0U;
volatile uint32_t g_feedback360_target_valid = 0U;
volatile uint32_t g_feedback360_period_us = 0U;
volatile uint32_t g_feedback360_high_us = 0U;
volatile uint16_t g_feedback360_pulse_us = FEEDBACK360_PWM_STOP_US;
volatile float g_feedback360_raw_angle_deg = 0.0f;
volatile float g_feedback360_angle_deg = 0.0f;
volatile float g_feedback360_target_deg = 0.0f;
volatile float g_feedback360_error_deg = 0.0f;
volatile float g_feedback360_command_us = 0.0f;

/* PD controller debug/watch variables. */
volatile float g_feedback360_velocity_raw_deg_s = 0.0f;
volatile float g_feedback360_velocity_deg_s = 0.0f;
volatile float g_feedback360_pd_p_us = 0.0f;
volatile float g_feedback360_pd_d_us = 0.0f;
volatile uint32_t g_feedback360_brake_active = 0U;
volatile uint32_t g_feedback360_settled = 0U;
volatile uint32_t g_feedback360_hold_active = 0U;

/* Cable-safe debug/watch variables. */
volatile float g_feedback360_unwrapped_deg = 0.0f;
volatile float g_feedback360_cable_target_unwrapped_deg = 0.0f;
volatile float g_feedback360_cable_twist_deg = 0.0f;
volatile uint32_t g_feedback360_cable_warning_active = 0U;
volatile uint32_t g_feedback360_cable_hard_limit_active = 0U;
volatile int32_t g_feedback360_cable_last_180_direction = 0;

static uint32_t s_last_capture_tick = 0U;
static uint32_t s_last_control_tick = 0U;
static uint32_t s_velocity_prev_tick = 0U;
static float s_velocity_prev_angle_deg = 0.0f;
static uint8_t s_velocity_initialized = 0U;
static uint8_t s_hold_active = 0U;

/* Cable-safe internal state. */
static uint8_t s_cable_unwrapped_initialized = 0U;
static float s_cable_prev_wrapped_deg = 0.0f;
static uint8_t s_cable_target_valid = 0U;
static float s_cable_command_wrapped_deg = 0.0f;
static float s_cable_target_unwrapped_deg = 0.0f;
static int32_t s_cable_last_180_direction = 0;

static float Feedback360_Wrap360(float deg)
{
    while (deg < 0.0f)
    {
        deg += 360.0f;
    }

    while (deg >= 360.0f)
    {
        deg -= 360.0f;
    }

    return deg;
}

static float Feedback360_AngleErrorDeg(float target_deg, float current_deg)
{
    float err;

    target_deg = Feedback360_Wrap360(target_deg);
    current_deg = Feedback360_Wrap360(current_deg);

    err = target_deg - current_deg;

    while (err > 180.0f)
    {
        err -= 360.0f;
    }

    while (err < -180.0f)
    {
        err += 360.0f;
    }

    return err;
}

static float Feedback360_WrappedToCableNeutralDeg(float wrapped_deg)
{
    float d = Feedback360_Wrap360(wrapped_deg);

    /* Map the single-turn sensor into the cable-neutral representation. */
    if (d >= 180.0f)
    {
        d -= 360.0f;
    }

    return d;
}

static void Feedback360_UpdateCableDebugFlags(void)
{
    float a = fabsf(g_feedback360_unwrapped_deg);

    g_feedback360_cable_twist_deg = g_feedback360_unwrapped_deg;
    g_feedback360_cable_warning_active =
        (a >= FEEDBACK360_CABLE_WARNING_DEG) ? 1U : 0U;
    g_feedback360_cable_hard_limit_active =
        (a >= FEEDBACK360_CABLE_HARD_LIMIT_DEG) ? 1U : 0U;
}

static void Feedback360_UpdateCableUnwrappedFromWrapped(float current_wrapped_deg)
{
    float wrapped = Feedback360_Wrap360(current_wrapped_deg);

    if (s_cable_unwrapped_initialized == 0U)
    {
        /*
         * Before startup homing we cannot know historical turns from the
         * single-turn sensor.  Assume power-up starts within one half-turn of
         * the cable-neutral position.  Startup homing later establishes an
         * exact zero reference.
         */
        g_feedback360_unwrapped_deg =
            Feedback360_WrappedToCableNeutralDeg(wrapped);
        s_cable_prev_wrapped_deg = wrapped;
        s_cable_unwrapped_initialized = 1U;
        Feedback360_UpdateCableDebugFlags();
        return;
    }

    {
        float delta = Feedback360_AngleErrorDeg(wrapped,
                                                s_cable_prev_wrapped_deg);
        g_feedback360_unwrapped_deg += delta;
        s_cable_prev_wrapped_deg = wrapped;
    }

    Feedback360_UpdateCableDebugFlags();
}

static float Feedback360_SelectCableSafeTarget(float target_wrapped_deg,
                                                float current_unwrapped_deg)
{
    float best = 0.0f;
    float best_move = 1e30f;
    uint8_t found = 0U;
    float target = Feedback360_Wrap360(target_wrapped_deg);

    /*
     * Check equivalent multi-turn targets.  With a limit >= 180 deg, every
     * 0~359 deg azimuth has a cable-safe equivalent and 180 deg has two.
     */
    for (int32_t k = -2; k <= 2; k++)
    {
        float candidate = target + (360.0f * (float)k);
        float move;

        if (candidate < (-FEEDBACK360_CABLE_TARGET_LIMIT_DEG -
                         FEEDBACK360_CABLE_TARGET_EPS_DEG))
        {
            continue;
        }
        if (candidate > (FEEDBACK360_CABLE_TARGET_LIMIT_DEG +
                         FEEDBACK360_CABLE_TARGET_EPS_DEG))
        {
            continue;
        }

        move = fabsf(candidate - current_unwrapped_deg);

        if ((found == 0U) ||
            (move < (best_move - FEEDBACK360_CABLE_TARGET_EPS_DEG)))
        {
            best = candidate;
            best_move = move;
            found = 1U;
        }
        else if (fabsf(move - best_move) <= FEEDBACK360_CABLE_TARGET_EPS_DEG)
        {
            /*
             * Exact 180-deg tie: alternate only the true +/-180 choice.
             * This avoids repeatedly winding the same side when starting from
             * cable-neutral 0 deg.
             */
            if (fabsf(target - 180.0f) <= FEEDBACK360_CABLE_TARGET_EPS_DEG)
            {
                int32_t preferred_dir;
                float candidate_delta = candidate - current_unwrapped_deg;
                int32_t candidate_dir = (candidate_delta >= 0.0f) ? 1 : -1;

                if (s_cable_last_180_direction == 0)
                {
                    preferred_dir = FEEDBACK360_CABLE_FIRST_180_DIR;
                }
                else
                {
                    preferred_dir = -s_cable_last_180_direction;
                }

                if (candidate_dir == preferred_dir)
                {
                    best = candidate;
                    best_move = move;
                }
            }
            else if (fabsf(candidate) < fabsf(best))
            {
                /* General tie: prefer the cable-neutral side. */
                best = candidate;
                best_move = move;
            }
        }
    }

    if (found == 0U)
    {
        /* Defensive fallback; normal 0~359 deg targets always find a candidate. */
        best = Feedback360_WrappedToCableNeutralDeg(target);
    }

    /* Remember only a real 180-deg tie-sized move for the next alternation. */
    if ((fabsf(target - 180.0f) <= FEEDBACK360_CABLE_TARGET_EPS_DEG) &&
        (fabsf(fabsf(best - current_unwrapped_deg) - 180.0f) <=
         FEEDBACK360_CABLE_TARGET_EPS_DEG))
    {
        s_cable_last_180_direction =
            ((best - current_unwrapped_deg) >= 0.0f) ? 1 : -1;
        g_feedback360_cable_last_180_direction =
            s_cable_last_180_direction;
    }

    return best;
}

static void Feedback360_ResetVelocityEstimator(void)
{
    s_velocity_prev_tick = 0U;
    s_velocity_prev_angle_deg = g_feedback360_angle_deg;
    s_velocity_initialized = 0U;
    g_feedback360_velocity_raw_deg_s = 0.0f;
    g_feedback360_velocity_deg_s = 0.0f;
}

static float Feedback360_UpdateVelocityDegPerSec(float current_deg, uint32_t now)
{
    uint32_t dt_ms;
    float delta_deg;
    float raw_velocity_deg_s;

    if (s_velocity_initialized == 0U)
    {
        s_velocity_initialized = 1U;
        s_velocity_prev_tick = now;
        s_velocity_prev_angle_deg = current_deg;
        g_feedback360_velocity_raw_deg_s = 0.0f;
        g_feedback360_velocity_deg_s = 0.0f;
        return 0.0f;
    }

    dt_ms = now - s_velocity_prev_tick;

    if ((dt_ms == 0U) || (dt_ms > FEEDBACK360_VELOCITY_RESET_MS))
    {
        s_velocity_prev_tick = now;
        s_velocity_prev_angle_deg = current_deg;
        g_feedback360_velocity_raw_deg_s = 0.0f;
        g_feedback360_velocity_deg_s = 0.0f;
        return 0.0f;
    }

    /* current - previous, wrapped to the shortest signed angular motion. */
    delta_deg = Feedback360_AngleErrorDeg(current_deg,
                                           s_velocity_prev_angle_deg);
    raw_velocity_deg_s = delta_deg * (1000.0f / (float)dt_ms);

    g_feedback360_velocity_raw_deg_s = raw_velocity_deg_s;
    g_feedback360_velocity_deg_s +=
        FEEDBACK360_VELOCITY_LPF_ALPHA *
        (raw_velocity_deg_s - g_feedback360_velocity_deg_s);

    s_velocity_prev_tick = now;
    s_velocity_prev_angle_deg = current_deg;

    return g_feedback360_velocity_deg_s;
}

static float Feedback360_LimitSignedCommandUs(float command_us,
                                               float min_drive_us,
                                               float max_drive_us)
{
    if (command_us > 0.0f)
    {
        if ((min_drive_us > 0.0f) && (command_us < min_drive_us))
        {
            command_us = min_drive_us;
        }
        if (command_us > max_drive_us)
        {
            command_us = max_drive_us;
        }
    }
    else if (command_us < 0.0f)
    {
        if ((min_drive_us > 0.0f) && (command_us > -min_drive_us))
        {
            command_us = -min_drive_us;
        }
        if (command_us < -max_drive_us)
        {
            command_us = -max_drive_us;
        }
    }

    return command_us;
}

static uint16_t Feedback360_ClampPulseUs(int32_t pulse_us)
{
    if (pulse_us < (int32_t)FEEDBACK360_PWM_MIN_US)
    {
        return FEEDBACK360_PWM_MIN_US;
    }

    if (pulse_us > (int32_t)FEEDBACK360_PWM_MAX_US)
    {
        return FEEDBACK360_PWM_MAX_US;
    }

    return (uint16_t)pulse_us;
}

static void Feedback360_SetPulseUs(uint16_t pulse_us)
{
    pulse_us = Feedback360_ClampPulseUs((int32_t)pulse_us);
    g_feedback360_pulse_us = pulse_us;
    __HAL_TIM_SET_COMPARE(&FEEDBACK360_CONTROL_TIM,
                          FEEDBACK360_CONTROL_CHANNEL,
                          pulse_us);
}

static float Feedback360_DutyToRawAngleDeg(uint32_t high_us, uint32_t period_us)
{
    uint32_t dc_x1000;
    int32_t angle_i;

    if (period_us == 0U)
    {
        return 0.0f;
    }

    /*
     * Parallax Feedback 360 datasheet formula.
     * dc_x1000: duty cycle scaled by 1000
     * valid nominal duty range: 2.9%~97.1% -> 29~971
     * angle = 359 - ((dc - dcMin) * 360) / (dcMax - dcMin + 1)
     */
    dc_x1000 = ((1000U * high_us) + (period_us / 2U)) / period_us;

    if (dc_x1000 < 29U)
    {
        dc_x1000 = 29U;
    }

    if (dc_x1000 > 971U)
    {
        dc_x1000 = 971U;
    }

    angle_i = 359 - ((((int32_t)dc_x1000 - 29) * 360) / (971 - 29 + 1));

    if (angle_i < 0)
    {
        angle_i = 0;
    }
    else if (angle_i > 359)
    {
        angle_i = 359;
    }

    return Feedback360_Wrap360((float)angle_i);
}

static float Feedback360_RawAngleToMotorAngleDeg(float raw_angle_deg)
{
    float angle = raw_angle_deg;

#if (FEEDBACK360_FEEDBACK_INVERT != 0U)
    angle = 360.0f - angle;
#endif

    angle += FEEDBACK360_ANGLE_OFFSET_DEG;

    return Feedback360_Wrap360(angle);
}

static HAL_StatusTypeDef Feedback360_ConfigFeedbackPwmInput(void)
{
    TIM_IC_InitTypeDef sConfigIC = {0};
    TIM_SlaveConfigTypeDef sSlaveConfig = {0};

    (void)HAL_TIM_IC_Stop_IT(&FEEDBACK360_FEEDBACK_TIM,
                             FEEDBACK360_FEEDBACK_PERIOD_CHANNEL);
    (void)HAL_TIM_IC_Stop_IT(&FEEDBACK360_FEEDBACK_TIM,
                             FEEDBACK360_FEEDBACK_HIGH_CHANNEL);

    sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
    sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
    sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
    sConfigIC.ICFilter = 0;
    if (HAL_TIM_IC_ConfigChannel(&FEEDBACK360_FEEDBACK_TIM,
                                 &sConfigIC,
                                 FEEDBACK360_FEEDBACK_PERIOD_CHANNEL) != HAL_OK)
    {
        return HAL_ERROR;
    }

    sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_FALLING;
    sConfigIC.ICSelection = TIM_ICSELECTION_INDIRECTTI;
    sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
    sConfigIC.ICFilter = 0;
    if (HAL_TIM_IC_ConfigChannel(&FEEDBACK360_FEEDBACK_TIM,
                                 &sConfigIC,
                                 FEEDBACK360_FEEDBACK_HIGH_CHANNEL) != HAL_OK)
    {
        return HAL_ERROR;
    }

    sSlaveConfig.SlaveMode = TIM_SLAVEMODE_RESET;
    sSlaveConfig.InputTrigger = TIM_TS_TI1FP1;
    sSlaveConfig.TriggerPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
    sSlaveConfig.TriggerPrescaler = TIM_ICPSC_DIV1;
    sSlaveConfig.TriggerFilter = 0;
    if (HAL_TIM_SlaveConfigSynchro(&FEEDBACK360_FEEDBACK_TIM,
                                   &sSlaveConfig) != HAL_OK)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}

HAL_StatusTypeDef Feedback360Servo_Init(void)
{
    HAL_StatusTypeDef ret;

    g_feedback360_target_valid = 0U;
    g_feedback360_feedback_valid = 0U;
    g_feedback360_error_deg = 0.0f;
    g_feedback360_command_us = 0.0f;
    g_feedback360_pd_p_us = 0.0f;
    g_feedback360_pd_d_us = 0.0f;
    g_feedback360_brake_active = 0U;
    g_feedback360_settled = 0U;
    g_feedback360_hold_active = 0U;
    s_hold_active = 0U;

    g_feedback360_unwrapped_deg = 0.0f;
    g_feedback360_cable_target_unwrapped_deg = 0.0f;
    g_feedback360_cable_twist_deg = 0.0f;
    g_feedback360_cable_warning_active = 0U;
    g_feedback360_cable_hard_limit_active = 0U;
    g_feedback360_cable_last_180_direction = 0;
    s_cable_unwrapped_initialized = 0U;
    s_cable_prev_wrapped_deg = 0.0f;
    s_cable_target_valid = 0U;
    s_cable_command_wrapped_deg = 0.0f;
    s_cable_target_unwrapped_deg = 0.0f;
    s_cable_last_180_direction = 0;

    Feedback360_ResetVelocityEstimator();

    ret = HAL_TIM_PWM_Start(&FEEDBACK360_CONTROL_TIM,
                            FEEDBACK360_CONTROL_CHANNEL);
    if (ret != HAL_OK)
    {
        return ret;
    }

    Feedback360_SetPulseUs(FEEDBACK360_PWM_STOP_US);

    ret = Feedback360_ConfigFeedbackPwmInput();
    if (ret != HAL_OK)
    {
        return ret;
    }

    __HAL_TIM_SET_COUNTER(&FEEDBACK360_FEEDBACK_TIM, 0U);

    ret = HAL_TIM_IC_Start_IT(&FEEDBACK360_FEEDBACK_TIM,
                              FEEDBACK360_FEEDBACK_HIGH_CHANNEL);
    if (ret != HAL_OK)
    {
        return ret;
    }

    ret = HAL_TIM_IC_Start_IT(&FEEDBACK360_FEEDBACK_TIM,
                              FEEDBACK360_FEEDBACK_PERIOD_CHANNEL);
    if (ret != HAL_OK)
    {
        return ret;
    }

    s_last_capture_tick = HAL_GetTick();
    s_last_control_tick = HAL_GetTick();

    return HAL_OK;
}

void Feedback360Servo_Stop(void)
{
    g_feedback360_command_us = 0.0f;
    Feedback360_SetPulseUs(FEEDBACK360_PWM_STOP_US);
}

void Feedback360Servo_ResetTarget(void)
{
    g_feedback360_target_valid = 0U;
    s_cable_target_valid = 0U;
    g_feedback360_error_deg = 0.0f;
    g_feedback360_pd_p_us = 0.0f;
    g_feedback360_pd_d_us = 0.0f;
    g_feedback360_brake_active = 0U;
    g_feedback360_settled = 0U;
    g_feedback360_hold_active = 0U;
    s_hold_active = 0U;
    Feedback360_ResetVelocityEstimator();
    Feedback360Servo_Stop();
}

float Feedback360Servo_GetAngleDeg(void)
{
    return g_feedback360_angle_deg;
}

float Feedback360Servo_GetRawAngleDeg(void)
{
    return g_feedback360_raw_angle_deg;
}

float Feedback360Servo_GetUnwrappedAngleDeg(void)
{
    return g_feedback360_unwrapped_deg;
}

void Feedback360Servo_ResetCableReferenceAtCurrentAngle(void)
{
    /*
     * Call this only when the mechanism is physically at the known cable
     * neutral/home orientation.  Startup homing in SoundMotorController does
     * exactly that at 0 deg.
     */
    s_cable_prev_wrapped_deg = Feedback360_Wrap360(g_feedback360_angle_deg);
    g_feedback360_unwrapped_deg = 0.0f;
    s_cable_unwrapped_initialized = 1U;

    s_cable_target_valid = 0U;
    s_cable_command_wrapped_deg = 0.0f;
    s_cable_target_unwrapped_deg = 0.0f;
    g_feedback360_cable_target_unwrapped_deg = 0.0f;

    Feedback360_UpdateCableDebugFlags();
}

uint8_t Feedback360Servo_IsFeedbackValid(void)
{
    return (uint8_t)g_feedback360_feedback_valid;
}

uint16_t Feedback360Servo_GetLastPulseUs(void)
{
    return g_feedback360_pulse_us;
}

void Feedback360Servo_SetRawPulseUs(uint16_t pulse_us)
{
    g_feedback360_target_valid = 0U;
    g_feedback360_error_deg = 0.0f;
    g_feedback360_command_us = 0.0f;
    g_feedback360_pd_p_us = 0.0f;
    g_feedback360_pd_d_us = 0.0f;
    g_feedback360_brake_active = 0U;
    g_feedback360_settled = 0U;
    g_feedback360_hold_active = 0U;
    s_hold_active = 0U;
    Feedback360_ResetVelocityEstimator();
    Feedback360_SetPulseUs(pulse_us);
}

HAL_StatusTypeDef Feedback360Servo_SetTargetAngle0To359(float target_deg)
{
    return Feedback360Servo_SetTargetAzimuthRealtime(target_deg);
}

HAL_StatusTypeDef Feedback360Servo_SetTargetAbsAngle(float target_abs_deg)
{
    /*
     * This project uses the 360-degree feedback as an azimuth position sensor.
     * Absolute multi-turn control is therefore folded to the closest 0~359 deg target.
     */
    return Feedback360Servo_SetTargetAzimuthRealtime(Feedback360_Wrap360(target_abs_deg));
}

HAL_StatusTypeDef Feedback360Servo_SetTargetAzimuthRealtime(float az_deg)
{
    uint32_t now;
    float target_deg;
    float current_deg;
    float current_unwrapped_deg;
    float target_unwrapped_deg;
    float err_deg;
    float abs_err_deg;
    float velocity_deg_s;
    float p_us;
    float d_us;
    float command_us;
    float min_drive_us;
    float max_drive_us;
    int32_t pulse_us;

    now = HAL_GetTick();
    target_deg = Feedback360_Wrap360(az_deg);

    g_feedback360_target_deg = target_deg;
    g_feedback360_target_valid = 1U;

    if ((now - s_last_capture_tick) > FEEDBACK360_FEEDBACK_TIMEOUT_MS)
    {
        g_feedback360_feedback_valid = 0U;
        g_feedback360_settled = 0U;
        Feedback360Servo_Stop();
        return HAL_ERROR;
    }

    if (g_feedback360_feedback_valid == 0U)
    {
        g_feedback360_settled = 0U;
        Feedback360Servo_Stop();
        return HAL_BUSY;
    }

    if ((now - s_last_control_tick) < FEEDBACK360_PWM_UPDATE_MS)
    {
        return HAL_BUSY;
    }

    current_deg = g_feedback360_angle_deg;
    current_unwrapped_deg = g_feedback360_unwrapped_deg;

    /*
     * Select the equivalent target only when the requested wrapped azimuth
     * changes.  Feedback360Servo_Task() repeatedly re-issues the same target,
     * so keeping the selected unwrapped target prevents mid-motion reversal.
     */
    if ((s_cable_target_valid == 0U) ||
        (fabsf(Feedback360_AngleErrorDeg(target_deg,
                                         s_cable_command_wrapped_deg)) >
         FEEDBACK360_CABLE_SAME_TARGET_EPS_DEG))
    {
        target_unwrapped_deg =
            Feedback360_SelectCableSafeTarget(target_deg,
                                              current_unwrapped_deg);
        s_cable_command_wrapped_deg = target_deg;
        s_cable_target_unwrapped_deg = target_unwrapped_deg;
        s_cable_target_valid = 1U;
    }
    else
    {
        target_unwrapped_deg = s_cable_target_unwrapped_deg;
    }

    g_feedback360_cable_target_unwrapped_deg = target_unwrapped_deg;

    /*
     * IMPORTANT: do not wrap this error back to +/-180 deg.  A cable-safe
     * target can intentionally request the longer angular path to unwind the
     * cable, e.g. current +170 -> target -170 means -340 deg through neutral.
     */
    err_deg = target_unwrapped_deg - current_unwrapped_deg;
    abs_err_deg = fabsf(err_deg);
    velocity_deg_s = Feedback360_UpdateVelocityDegPerSec(current_deg, now);

    g_feedback360_error_deg = err_deg;
    g_feedback360_brake_active = 0U;
    g_feedback360_settled = 0U;

    /*
     * Anti-jitter HOLD (Schmitt-trigger style hysteresis).
     *
     * - Enter stop/hold at <= 1.5 deg.
     * - Once stopped, stay at the exact neutral PWM while error <= 3.0 deg.
     * - Re-drive only when a real target/mechanical change exceeds 3.0 deg.
     *
     * This prevents feedback quantization and tiny shaft bounce from repeatedly
     * generating +/- minimum-drive commands around the target.
     */
    if (s_hold_active != 0U)
    {
        if (abs_err_deg <= FEEDBACK360_HOLD_EXIT_DEG)
        {
            g_feedback360_pd_p_us = 0.0f;
            g_feedback360_pd_d_us = 0.0f;
            g_feedback360_command_us = 0.0f;
            g_feedback360_brake_active = 0U;
            g_feedback360_settled = 1U;
            g_feedback360_hold_active = 1U;
            Feedback360_SetPulseUs(FEEDBACK360_PWM_STOP_US);
            s_last_control_tick = now;
            return HAL_OK;
        }

        s_hold_active = 0U;
        g_feedback360_hold_active = 0U;
        g_feedback360_settled = 0U;
        Feedback360_ResetVelocityEstimator();
        velocity_deg_s = 0.0f;
    }

    if (abs_err_deg <= FEEDBACK360_HOLD_ENTER_DEG)
    {
        s_hold_active = 1U;
        g_feedback360_hold_active = 1U;
        g_feedback360_pd_p_us = 0.0f;
        g_feedback360_pd_d_us = 0.0f;
        g_feedback360_command_us = 0.0f;
        g_feedback360_brake_active = 0U;
        g_feedback360_settled = 1U;
        Feedback360_SetPulseUs(FEEDBACK360_PWM_STOP_US);
        s_last_control_tick = now;
        return HAL_OK;
    }

    /* PD in the logical angle frame.  The control inversion is applied later. */
    p_us = FEEDBACK360_KP_US_PER_DEG * err_deg;
    d_us = -FEEDBACK360_KD_US_PER_DEG_S * velocity_deg_s;
    command_us = p_us + d_us;

    g_feedback360_pd_p_us = p_us;
    g_feedback360_pd_d_us = d_us;

    if (abs_err_deg > FEEDBACK360_ZONE_FAR_DEG)
    {
        min_drive_us = FEEDBACK360_MIN_DRIVE_FAR_US;
        max_drive_us = FEEDBACK360_MAX_DRIVE_FAR_US;
    }
    else if (abs_err_deg > FEEDBACK360_ZONE_MID_DEG)
    {
        min_drive_us = FEEDBACK360_MIN_DRIVE_MID_US;
        max_drive_us = FEEDBACK360_MAX_DRIVE_MID_US;
    }
    else if (abs_err_deg > FEEDBACK360_ZONE_NEAR_DEG)
    {
        min_drive_us = FEEDBACK360_MIN_DRIVE_NEAR_US;
        max_drive_us = FEEDBACK360_MAX_DRIVE_NEAR_US;
    }
    else
    {
        min_drive_us = FEEDBACK360_MIN_DRIVE_FINE_US;
        max_drive_us = FEEDBACK360_MAX_DRIVE_FINE_US;
    }

    command_us = Feedback360_LimitSignedCommandUs(command_us,
                                                   min_drive_us,
                                                   max_drive_us);

    /*
     * Hard cable guard.  Normal target selection is already confined to
     * +/-TARGET_LIMIT.  This is a final fail-safe against further outward motion
     * if overshoot or an unexpected state reaches the hard boundary.
     */
    if ((current_unwrapped_deg >= FEEDBACK360_CABLE_HARD_LIMIT_DEG) &&
        (command_us > 0.0f))
    {
        command_us = 0.0f;
        g_feedback360_cable_hard_limit_active = 1U;
    }
    else if ((current_unwrapped_deg <= -FEEDBACK360_CABLE_HARD_LIMIT_DEG) &&
             (command_us < 0.0f))
    {
        command_us = 0.0f;
        g_feedback360_cable_hard_limit_active = 1U;
    }

#if (FEEDBACK360_CONTROL_INVERT != 0U)
    command_us = -command_us;
#endif

    g_feedback360_command_us = command_us;
    pulse_us = (int32_t)((float)FEEDBACK360_PWM_STOP_US + command_us);

    Feedback360_SetPulseUs(Feedback360_ClampPulseUs(pulse_us));
    s_last_control_tick = now;

    return HAL_OK;
}

void Feedback360Servo_Task(void)
{
    uint32_t now = HAL_GetTick();

    if ((now - s_last_capture_tick) > FEEDBACK360_FEEDBACK_TIMEOUT_MS)
    {
        g_feedback360_feedback_valid = 0U;
        Feedback360Servo_Stop();
        return;
    }

    if (g_feedback360_target_valid != 0U)
    {
        (void)Feedback360Servo_SetTargetAzimuthRealtime(g_feedback360_target_deg);
    }
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    uint32_t period_us;
    uint32_t high_us;
    float raw_angle_deg;

    if (htim->Instance != FEEDBACK360_FEEDBACK_TIM.Instance)
    {
        return;
    }

    if (htim->Channel != HAL_TIM_ACTIVE_CHANNEL_1)
    {
        return;
    }

    period_us = HAL_TIM_ReadCapturedValue(htim,
                                          FEEDBACK360_FEEDBACK_PERIOD_CHANNEL);
    high_us = HAL_TIM_ReadCapturedValue(htim,
                                        FEEDBACK360_FEEDBACK_HIGH_CHANNEL);

    if ((period_us < FEEDBACK360_PERIOD_MIN_US) ||
        (period_us > FEEDBACK360_PERIOD_MAX_US) ||
        (high_us == 0U) ||
        (high_us >= period_us))
    {
        g_feedback360_invalid_capture_count++;
        return;
    }

    raw_angle_deg = Feedback360_DutyToRawAngleDeg(high_us, period_us);

    g_feedback360_period_us = period_us;
    g_feedback360_high_us = high_us;
    g_feedback360_raw_angle_deg = raw_angle_deg;
    g_feedback360_angle_deg = Feedback360_RawAngleToMotorAngleDeg(raw_angle_deg);
    Feedback360_UpdateCableUnwrappedFromWrapped(g_feedback360_angle_deg);
    g_feedback360_feedback_valid = 1U;
    g_feedback360_capture_count++;

    s_last_capture_tick = HAL_GetTick();
}

