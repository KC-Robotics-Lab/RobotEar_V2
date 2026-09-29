//#ifndef INC_SOUNDMOTORCONTROLLER_H_
//#define INC_SOUNDMOTORCONTROLLER_H_
//
//#include "main.h"
//#include "SoundMotorConfig.h"
//#include <stdint.h>
//
///*
// * Sound azimuth motor controller.
// *
// * This layer keeps main.c independent from the concrete motor drivers.
// * It reads the currently selected BF axis and sends the same motor-frame
// * azimuth to both supported azimuth motors:
// *   - MX28AR Dynamixel motor
// *   - Parallax / Adafruit Feedback 360 servo
// *
// * Enable/disable each motor at the top of SoundMotorController.c.
// */
//
//HAL_StatusTypeDef SoundMotorController_Init(void);
//void SoundMotorController_Update(void);
//void SoundMotorController_ResetTarget(void);
//HAL_StatusTypeDef SoundMotorController_SetTargetAzimuthRealtime(float motor_az_deg);
//
//extern volatile uint32_t g_sound_motor_target_valid;
//extern volatile uint32_t g_sound_motor_stream;
//extern volatile uint32_t g_sound_motor_update_count;
//extern volatile uint32_t g_sound_motor_reset_count;
//extern volatile uint32_t g_sound_motor_mx28ar_enabled;
//extern volatile uint32_t g_sound_motor_feedback360_enabled;
//extern volatile uint32_t g_sound_motor_mx28ar_status;
//extern volatile uint32_t g_sound_motor_feedback360_status;
//extern volatile float g_sound_motor_srp_motor_az_deg;
//extern volatile float g_sound_motor_bf_board_az_deg;
//extern volatile float g_sound_motor_motor_az_deg;
//
///* Backward-compatible debug/watch names from the Feedback360-only version. */
//extern volatile float g_feedback360_srp_motor_az_deg;
//extern volatile float g_feedback360_bf_board_az_deg;
//extern volatile float g_feedback360_bf_motor_az_deg;
//extern volatile uint32_t g_feedback360_bf_motor_valid;
//extern volatile uint32_t g_feedback360_bf_motor_stream;
//
//#endif /* INC_SOUNDMOTORCONTROLLER_H_ */


















#ifndef INC_SOUNDMOTORCONTROLLER_H_
#define INC_SOUNDMOTORCONTROLLER_H_

#include "main.h"
#include "SoundMotorConfig.h"
#include <stdint.h>

/*
 * Sound azimuth motor controller.
 *
 * This layer keeps main.c independent from the concrete motor drivers.
 * It reads the currently selected BF axis and sends the same motor-frame
 * azimuth to both supported azimuth motors:
 *   - MX28AR Dynamixel motor
 *   - Parallax / Adafruit Feedback 360 servo
 *
 * Enable/disable each motor at the top of SoundMotorController.c.
 */

HAL_StatusTypeDef SoundMotorController_Init(void);
void SoundMotorController_Task(void);
void SoundMotorController_Update(void);
void SoundMotorController_ClearSourceTarget(void);
void SoundMotorController_ResetTarget(void);
HAL_StatusTypeDef SoundMotorController_SetTargetAzimuthRealtime(float motor_az_deg);

extern volatile uint32_t g_sound_motor_target_valid;
extern volatile uint32_t g_sound_motor_stream;
extern volatile uint32_t g_sound_motor_update_count;
extern volatile uint32_t g_sound_motor_reset_count;
extern volatile uint32_t g_sound_motor_mx28ar_enabled;
extern volatile uint32_t g_sound_motor_feedback360_enabled;
extern volatile uint32_t g_sound_motor_mx28ar_status;
extern volatile uint32_t g_sound_motor_feedback360_status;
extern volatile float g_sound_motor_srp_motor_az_deg;
extern volatile float g_sound_motor_bf_board_az_deg;
extern volatile float g_sound_motor_motor_az_deg;

extern volatile uint32_t g_sound_motor_startup_home_success;
extern volatile uint32_t g_sound_motor_startup_home_timeout_count;
extern volatile uint32_t g_sound_motor_startup_home_elapsed_ms;
extern volatile uint32_t g_sound_motor_startup_home_stable_ms;
extern volatile float g_sound_motor_startup_home_angle_deg;
extern volatile float g_sound_motor_startup_home_error_deg;

/* Backward-compatible debug/watch names from the Feedback360-only version. */
extern volatile float g_feedback360_srp_motor_az_deg;
extern volatile float g_feedback360_bf_board_az_deg;
extern volatile float g_feedback360_bf_motor_az_deg;
extern volatile uint32_t g_feedback360_bf_motor_valid;
extern volatile uint32_t g_feedback360_bf_motor_stream;

#endif /* INC_SOUNDMOTORCONTROLLER_H_ */
