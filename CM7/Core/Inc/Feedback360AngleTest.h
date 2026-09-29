#ifndef INC_FEEDBACK360ANGLETEST_H_
#define INC_FEEDBACK360ANGLETEST_H_

#include "main.h"
#include <stdint.h>

/*
 * Feedback 360 motor standalone angle test.
 *
 * 1U: normal sound motor logic is bypassed and the motor repeats
 *     0 -> 90 -> 180 -> 270 deg.
 * 0U: normal RobotEar sound tracking logic is used.
 */
#define FEEDBACK360_ANGLE_TEST_ENABLE          0U

/* Auto sequence hold time for each angle. */
#define FEEDBACK360_ANGLE_TEST_HOLD_MS         3000U

HAL_StatusTypeDef Feedback360AngleTest_Init(void);
void Feedback360AngleTest_Task(void);
void Feedback360AngleTest_SetManualIndex(uint32_t index);
void Feedback360AngleTest_SetAutoMode(void);

/*
 * Debug/watch variables.
 * Manual test:
 *   g_feedback360_test_manual_enable = 1
 *   g_feedback360_test_manual_index  = 0,1,2,3
 *     0 -> 0 deg, 1 -> 90 deg, 2 -> 180 deg, 3 -> 270 deg
 */
extern volatile uint32_t g_feedback360_test_state;
extern volatile uint32_t g_feedback360_test_index;
extern volatile uint32_t g_feedback360_test_manual_enable;
extern volatile uint32_t g_feedback360_test_manual_index;
extern volatile uint32_t g_feedback360_test_step_count;
extern volatile float g_feedback360_test_target_deg;
extern volatile float g_feedback360_test_current_deg;
extern volatile float g_feedback360_test_error_deg;

#endif /* INC_FEEDBACK360ANGLETEST_H_ */
