//#ifndef INC_D131MWSERVO_H_
//#define INC_D131MWSERVO_H_
//
//#include "main.h"
//#include <stdint.h>
//
///*
// * Hitec D131MW elevation servo driver.
// *
// * Hardware used in this project:
// *   - TIM2_CH1 : 50 Hz PWM output
// *
// * TIM2 should be configured with 1 us counter ticks:
// *   Timer clock 200 MHz -> Prescaler 199
// *   Counter Period 19999 -> 20 ms / 50 Hz
// */
//
//void HitecD131MWServo_Init(void);
//void HitecD131MWServo_SetPulseUs(uint16_t pulse_us);
//void HitecD131MWServo_SetAngle(float angle_deg);
//void HitecD131MWServo_SetTdoaLsElevation(float tdoa_el_deg);
//void HitecD131MWServo_SetTdoaLsElevationRealtime(float tdoa_el_deg);
//
//extern volatile uint16_t g_d131mw_pulse_us;
//extern volatile float g_d131mw_angle_deg;
//extern volatile float g_d131mw_tdoa_el_deg;
//
//#endif /* INC_D131MWSERVO_H_ */













#ifndef INC_D131MWSERVO_H_
#define INC_D131MWSERVO_H_

#include "main.h"
#include <stdint.h>

/*
 * Hitec D131MW elevation servo driver.
 *
 * Hardware used in this project:
 *   - TIM2_CH1 : 50 Hz PWM output
 *
 * TIM2 should be configured with 1 us counter ticks:
 *   Timer clock 200 MHz -> Prescaler 199
 *   Counter Period 19999 -> 20 ms / 50 Hz
 */

void HitecD131MWServo_Init(void);
void HitecD131MWServo_SetPulseUs(uint16_t pulse_us);
void HitecD131MWServo_SetAngle(float angle_deg);
void HitecD131MWServo_SetTdoaLsElevation(float tdoa_el_deg);
void HitecD131MWServo_SetTdoaLsElevationRealtime(float tdoa_el_deg);
void HitecD131MWServo_SetTargetTdoaLsElevation(float tdoa_el_deg);
void HitecD131MWServo_Task(void);

extern volatile uint16_t g_d131mw_pulse_us;
extern volatile float g_d131mw_angle_deg;
extern volatile float g_d131mw_tdoa_el_deg;
extern volatile float g_d131mw_target_tdoa_el_deg;
extern volatile float g_d131mw_target_error_deg;
extern volatile uint32_t g_d131mw_target_valid;
extern volatile uint32_t g_d131mw_target_reached;
extern volatile uint32_t g_d131mw_task_update_count;

#endif /* INC_D131MWSERVO_H_ */

