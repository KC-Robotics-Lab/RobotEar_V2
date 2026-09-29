//#ifndef INC_FEEDBACK360SERVO_H_
//#define INC_FEEDBACK360SERVO_H_
//
//#include "main.h"
//#include <stdint.h>
//
///*
// * Parallax / Adafruit Feedback 360 degree servo driver.
// *
// * Hardware used in this project:
// *   - TIM2_CH2 : 50 Hz control PWM output, 1280~1720 us, 1500 us stop
// *   - TIM3_CH1 : feedback PWM input, rising edge / period capture
// *   - TIM3_CH2 : feedback PWM input, falling edge / high-time capture
// *
// * TIM2 and TIM3 should be configured with 1 us counter ticks:
// *   Timer clock 200 MHz -> Prescaler 199
// */
//
//HAL_StatusTypeDef Feedback360Servo_Init(void);
//void Feedback360Servo_Stop(void);
//void Feedback360Servo_ResetTarget(void);
//
//HAL_StatusTypeDef Feedback360Servo_SetTargetAzimuthRealtime(float az_deg);
//HAL_StatusTypeDef Feedback360Servo_SetTargetAngle0To359(float target_deg);
//HAL_StatusTypeDef Feedback360Servo_SetTargetAbsAngle(float target_abs_deg);
//void Feedback360Servo_SetRawPulseUs(uint16_t pulse_us);
//void Feedback360Servo_Task(void);
//
//float Feedback360Servo_GetAngleDeg(void);
//float Feedback360Servo_GetRawAngleDeg(void);
//uint8_t Feedback360Servo_IsFeedbackValid(void);
//uint16_t Feedback360Servo_GetLastPulseUs(void);
//
///* Debug/watch variables */
//extern volatile uint32_t g_feedback360_capture_count;
//extern volatile uint32_t g_feedback360_invalid_capture_count;
//extern volatile uint32_t g_feedback360_feedback_valid;
//extern volatile uint32_t g_feedback360_target_valid;
//extern volatile uint32_t g_feedback360_period_us;
//extern volatile uint32_t g_feedback360_high_us;
//extern volatile uint16_t g_feedback360_pulse_us;
//extern volatile float g_feedback360_raw_angle_deg;
//extern volatile float g_feedback360_angle_deg;
//extern volatile float g_feedback360_target_deg;
//extern volatile float g_feedback360_error_deg;
//extern volatile float g_feedback360_command_us;
//
//#endif /* INC_FEEDBACK360SERVO_H_ */



























#ifndef INC_FEEDBACK360SERVO_H_
#define INC_FEEDBACK360SERVO_H_

#include "main.h"
#include <stdint.h>

/*
 * Parallax / Adafruit Feedback 360 degree servo driver.
 *
 * Hardware used in this project:
 *   - TIM2_CH2 : 50 Hz control PWM output, 1280~1720 us, 1500 us stop
 *   - TIM3_CH1 : feedback PWM input, rising edge / period capture
 *   - TIM3_CH2 : feedback PWM input, falling edge / high-time capture
 *
 * TIM2 and TIM3 should be configured with 1 us counter ticks:
 *   Timer clock 200 MHz -> Prescaler 199
 */

HAL_StatusTypeDef Feedback360Servo_Init(void);
void Feedback360Servo_Stop(void);
void Feedback360Servo_ResetTarget(void);

HAL_StatusTypeDef Feedback360Servo_SetTargetAzimuthRealtime(float az_deg);
HAL_StatusTypeDef Feedback360Servo_SetTargetAngle0To359(float target_deg);
HAL_StatusTypeDef Feedback360Servo_SetTargetAbsAngle(float target_abs_deg);
void Feedback360Servo_SetRawPulseUs(uint16_t pulse_us);
void Feedback360Servo_Task(void);

float Feedback360Servo_GetAngleDeg(void);
float Feedback360Servo_GetRawAngleDeg(void);
float Feedback360Servo_GetUnwrappedAngleDeg(void);
void Feedback360Servo_ResetCableReferenceAtCurrentAngle(void);
uint8_t Feedback360Servo_IsFeedbackValid(void);
uint16_t Feedback360Servo_GetLastPulseUs(void);

/* Debug/watch variables */
extern volatile uint32_t g_feedback360_capture_count;
extern volatile uint32_t g_feedback360_invalid_capture_count;
extern volatile uint32_t g_feedback360_feedback_valid;
extern volatile uint32_t g_feedback360_target_valid;
extern volatile uint32_t g_feedback360_period_us;
extern volatile uint32_t g_feedback360_high_us;
extern volatile uint16_t g_feedback360_pulse_us;
extern volatile float g_feedback360_raw_angle_deg;
extern volatile float g_feedback360_angle_deg;
extern volatile float g_feedback360_target_deg;
extern volatile float g_feedback360_error_deg;
extern volatile float g_feedback360_command_us;
extern volatile float g_feedback360_velocity_raw_deg_s;
extern volatile float g_feedback360_velocity_deg_s;
extern volatile float g_feedback360_pd_p_us;
extern volatile float g_feedback360_pd_d_us;
extern volatile uint32_t g_feedback360_brake_active;
extern volatile uint32_t g_feedback360_settled;

extern volatile uint32_t g_feedback360_hold_active;

/* Cable-safe debug/watch variables. */
extern volatile float g_feedback360_unwrapped_deg;
extern volatile float g_feedback360_cable_target_unwrapped_deg;
extern volatile float g_feedback360_cable_twist_deg;
extern volatile uint32_t g_feedback360_cable_warning_active;
extern volatile uint32_t g_feedback360_cable_hard_limit_active;
extern volatile int32_t g_feedback360_cable_last_180_direction;

#endif /* INC_FEEDBACK360SERVO_H_ */
