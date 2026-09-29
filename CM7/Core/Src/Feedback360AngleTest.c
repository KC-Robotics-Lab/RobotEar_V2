#include "Feedback360AngleTest.h"
#include "Feedback360Servo.h"
#include <math.h>

#define FEEDBACK360_TEST_TARGET_COUNT    4U

static const float s_feedback360_test_targets[FEEDBACK360_TEST_TARGET_COUNT] =
{
    0.0f,
    90.0f,
    180.0f,
    270.0f
};

volatile uint32_t g_feedback360_test_state = 0U;
volatile uint32_t g_feedback360_test_index = 0U;
volatile uint32_t g_feedback360_test_manual_enable = 0U;
volatile uint32_t g_feedback360_test_manual_index = 0U;
volatile uint32_t g_feedback360_test_step_count = 0U;
volatile float g_feedback360_test_target_deg = 0.0f;
volatile float g_feedback360_test_current_deg = 0.0f;
volatile float g_feedback360_test_error_deg = 0.0f;

static uint32_t s_last_step_tick = 0U;
static uint32_t s_applied_index = 0xFFFFFFFFU;

static float Feedback360Test_Wrap360(float deg)
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

static float Feedback360Test_AngleErrorDeg(float target_deg, float current_deg)
{
    float err;

    target_deg = Feedback360Test_Wrap360(target_deg);
    current_deg = Feedback360Test_Wrap360(current_deg);

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

static void Feedback360Test_ApplyIndex(uint32_t index)
{
    float target_deg;

    index %= FEEDBACK360_TEST_TARGET_COUNT;
    target_deg = s_feedback360_test_targets[index];

    g_feedback360_test_index = index;
    g_feedback360_test_target_deg = target_deg;
    s_applied_index = index;

    (void)Feedback360Servo_SetTargetAngle0To359(target_deg);
}

HAL_StatusTypeDef Feedback360AngleTest_Init(void)
{
    HAL_StatusTypeDef ret;

    g_feedback360_test_state = 0U;
    g_feedback360_test_index = 0U;
    g_feedback360_test_manual_enable = 0U;
    g_feedback360_test_manual_index = 0U;
    g_feedback360_test_step_count = 0U;
    g_feedback360_test_target_deg = 0.0f;
    g_feedback360_test_current_deg = 0.0f;
    g_feedback360_test_error_deg = 0.0f;

    ret = Feedback360Servo_Init();
    if (ret != HAL_OK)
    {
        g_feedback360_test_state = 9U;
        return ret;
    }

    s_last_step_tick = HAL_GetTick();
    s_applied_index = 0xFFFFFFFFU;

    Feedback360Test_ApplyIndex(0U);
    g_feedback360_test_state = 1U;

    return HAL_OK;
}

void Feedback360AngleTest_SetManualIndex(uint32_t index)
{
    g_feedback360_test_manual_enable = 1U;
    g_feedback360_test_manual_index = index % FEEDBACK360_TEST_TARGET_COUNT;
    Feedback360Test_ApplyIndex(g_feedback360_test_manual_index);
}

void Feedback360AngleTest_SetAutoMode(void)
{
    g_feedback360_test_manual_enable = 0U;
    s_last_step_tick = HAL_GetTick();
}

void Feedback360AngleTest_Task(void)
{
    uint32_t now;
    uint32_t next_index;
    float current_deg;

    now = HAL_GetTick();

    if (g_feedback360_test_manual_enable != 0U)
    {
        next_index = g_feedback360_test_manual_index % FEEDBACK360_TEST_TARGET_COUNT;
        if (next_index != s_applied_index)
        {
            Feedback360Test_ApplyIndex(next_index);
            g_feedback360_test_step_count++;
        }
    }
    else if ((now - s_last_step_tick) >= FEEDBACK360_ANGLE_TEST_HOLD_MS)
    {
        next_index = (g_feedback360_test_index + 1U) % FEEDBACK360_TEST_TARGET_COUNT;
        Feedback360Test_ApplyIndex(next_index);
        g_feedback360_test_step_count++;
        s_last_step_tick = now;
    }

    Feedback360Servo_Task();

    current_deg = Feedback360Servo_GetAngleDeg();
    g_feedback360_test_current_deg = current_deg;
    g_feedback360_test_error_deg = Feedback360Test_AngleErrorDeg(g_feedback360_test_target_deg,
                                                                 current_deg);

    if (Feedback360Servo_IsFeedbackValid() == 0U)
    {
        g_feedback360_test_state = 2U; /* waiting for feedback */
    }
    else if (fabsf(g_feedback360_test_error_deg) <= 2.0f)
    {
        g_feedback360_test_state = 3U; /* target reached */
    }
    else
    {
        g_feedback360_test_state = 1U; /* moving */
    }
}
