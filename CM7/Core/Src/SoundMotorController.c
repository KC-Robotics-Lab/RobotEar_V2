//#include "SoundMotorController.h"
//#include "SoundMotorConfig.h"
//#include "SRP_PHAT.h"
//#include "audio_aec_bf_pcm.h"
//#include <math.h>
//
//#if (SOUND_MOTOR_ENABLE_MX28AR != 0U)
//#include "MX28AR.h"
//#endif
//
//#if (SOUND_MOTOR_ENABLE_FEEDBACK360 != 0U)
//#include "Feedback360Servo.h"
//#endif
//
///* ------------------------------------------------------------------------- */
///* Debug/watch variables                                                     */
///* ------------------------------------------------------------------------- */
//
//volatile uint32_t g_sound_motor_target_valid = 0U;
//volatile uint32_t g_sound_motor_stream = 0U;
//volatile uint32_t g_sound_motor_update_count = 0U;
//volatile uint32_t g_sound_motor_reset_count = 0U;
//volatile uint32_t g_sound_motor_mx28ar_enabled = SOUND_MOTOR_ENABLE_MX28AR;
//volatile uint32_t g_sound_motor_feedback360_enabled = SOUND_MOTOR_ENABLE_FEEDBACK360;
//volatile uint32_t g_sound_motor_mx28ar_status = HAL_OK;
//volatile uint32_t g_sound_motor_feedback360_status = HAL_OK;
//volatile float g_sound_motor_srp_motor_az_deg = 0.0f;
//volatile float g_sound_motor_bf_board_az_deg = 0.0f;
//volatile float g_sound_motor_motor_az_deg = 0.0f;
//
///* Backward-compatible debug/watch names from the Feedback360-only version. */
//volatile float g_feedback360_srp_motor_az_deg = 0.0f;
//volatile float g_feedback360_bf_board_az_deg = 0.0f;
//volatile float g_feedback360_bf_motor_az_deg = 0.0f;
//volatile uint32_t g_feedback360_bf_motor_valid = 0U;
//volatile uint32_t g_feedback360_bf_motor_stream = 0U;
//
//static float SoundMotor_Wrap360(float x)
//{
//    while (x < 0.0f)
//    {
//        x += 360.0f;
//    }
//
//    while (x >= 360.0f)
//    {
//        x -= 360.0f;
//    }
//
//    return x;
//}
//
//static void SoundMotor_DebugSetTarget(const doa_first_sound_target_t *first_target,
//                                      uint32_t stream_id,
//                                      float bf_board_az_deg,
//                                      float motor_az_deg)
//{
//    float srp_motor_az_deg = 0.0f;
//
//    if (first_target != NULL)
//    {
//        srp_motor_az_deg = first_target->az_deg;
//    }
//
//    g_sound_motor_srp_motor_az_deg = srp_motor_az_deg;
//    g_sound_motor_bf_board_az_deg = bf_board_az_deg;
//    g_sound_motor_motor_az_deg = motor_az_deg;
//    g_sound_motor_stream = stream_id;
//    g_sound_motor_target_valid = 1U;
//    g_sound_motor_update_count++;
//
//    g_feedback360_srp_motor_az_deg = srp_motor_az_deg;
//    g_feedback360_bf_board_az_deg = bf_board_az_deg;
//    g_feedback360_bf_motor_az_deg = motor_az_deg;
//    g_feedback360_bf_motor_stream = stream_id;
//    g_feedback360_bf_motor_valid = 1U;
//}
//
//static void SoundMotor_DebugClearTarget(void)
//{
//    g_sound_motor_target_valid = 0U;
//    g_feedback360_bf_motor_valid = 0U;
//    g_sound_motor_reset_count++;
//}
//
//static uint8_t SoundMotor_GetBFTargetFromFirstSound(const doa_first_sound_target_t *first_target,
//                                                    float *out_motor_az_deg)
//{
//    uint32_t stream_id;
//    float bf_board_az_deg;
//    float motor_az_deg;
//
//    if ((first_target == NULL) || (out_motor_az_deg == NULL))
//    {
//        return 0U;
//    }
//
//    stream_id = (uint32_t)first_target->stream_id;
//    if (stream_id >= AUDIO_STREAM_COUNT)
//    {
//        return 0U;
//    }
//
//    /*
//     * g_bf_selected_pair_axis_deg_stream[] is updated by AudioBF_SetTarget...
//     * and represents the actual selected directed mic-pair / BF steering axis.
//     */
//    if (g_bf_target_valid_stream[stream_id] == 0U)
//    {
//        return 0U;
//    }
//
//    bf_board_az_deg = SoundMotor_Wrap360(g_bf_selected_pair_axis_deg_stream[stream_id]);
//    motor_az_deg = DOA_ConvertBoardAzToMotorAz(stream_id, bf_board_az_deg);
//
//    SoundMotor_DebugSetTarget(first_target,
//                              stream_id,
//                              bf_board_az_deg,
//                              motor_az_deg);
//
//    *out_motor_az_deg = motor_az_deg;
//    return 1U;
//}
//
//#define SOUND_MOTOR_STARTUP_HOME_DEG         0.0f
//#define SOUND_MOTOR_STARTUP_TIMEOUT_MS       5000U
//#define SOUND_MOTOR_STARTUP_TOLERANCE_DEG    2.0f
//
//static void SoundMotorController_MoveToStartupHome(void)
//{
//    uint32_t start_tick = HAL_GetTick();
//
//    while ((HAL_GetTick() - start_tick) < SOUND_MOTOR_STARTUP_TIMEOUT_MS)
//    {
//        float current_deg;
//        float error_deg;
//
//        /*
//         * 피드백이 아직 준비되지 않아도 목표각은 내부에 저장됩니다.
//         * 반복 호출하면서 피드백 수신 후 실제 제어가 시작됩니다.
//         */
//        (void)Feedback360Servo_SetTargetAngle0To359(
//            SOUND_MOTOR_STARTUP_HOME_DEG);
//
//        if (Feedback360Servo_IsFeedbackValid() != 0U)
//        {
//            current_deg = Feedback360Servo_GetAngleDeg();
//
//            /* 0도와 359도 경계를 고려한 오차 계산 */
//            error_deg = current_deg;
//
//            if (error_deg > 180.0f)
//            {
//                error_deg = 360.0f - error_deg;
//            }
//
//            if (fabsf(error_deg) <= SOUND_MOTOR_STARTUP_TOLERANCE_DEG)
//            {
//                break;
//            }
//        }
//
//        HAL_Delay(5U);
//    }
//
//    Feedback360Servo_Stop();
//}
//
//HAL_StatusTypeDef SoundMotorController_Init(void)
//{
//    HAL_StatusTypeDef ret;
//
//#if (SOUND_MOTOR_ENABLE_FEEDBACK360 != 0U)
//    ret = Feedback360Servo_Init();
//    g_sound_motor_feedback360_status = (uint32_t)ret;
//    if (ret != HAL_OK)
//    {
//        return ret;
//    }
//#else
//    g_sound_motor_feedback360_status = HAL_OK;
//#endif
//
//#if (SOUND_MOTOR_ENABLE_MX28AR != 0U)
//    ret = MX28AR_InitRealtime();
//    g_sound_motor_mx28ar_status = (uint32_t)ret;
//    if (ret != HAL_OK)
//    {
//        return ret;
//    }
//#else
//    g_sound_motor_mx28ar_status = HAL_OK;
//#endif
//
//    SoundMotorController_ResetTarget();
//#if (SOUND_MOTOR_ENABLE_FEEDBACK360 != 0U)
//    SoundMotorController_MoveToStartupHome();
//#endif
//    return HAL_OK;
//}
//
//void SoundMotorController_ResetTarget(void)
//{
//    SoundMotor_DebugClearTarget();
//
//#if (SOUND_MOTOR_ENABLE_FEEDBACK360 != 0U)
//    Feedback360Servo_ResetTarget();
//#endif
//
//    /*
//     * MX28AR is a position servo. When the sound target disappears, keep the
//     * last commanded position instead of torque-off, so the head does not sag
//     * or free-spin. Call MX28AR_TorqueOff() manually if you want release mode.
//     */
//}
//
//HAL_StatusTypeDef SoundMotorController_SetTargetAzimuthRealtime(float motor_az_deg)
//{
//    HAL_StatusTypeDef ret = HAL_OK;
//    HAL_StatusTypeDef final_ret = HAL_OK;
//
//    motor_az_deg = SoundMotor_Wrap360(motor_az_deg);
//    g_sound_motor_motor_az_deg = motor_az_deg;
//
//#if (SOUND_MOTOR_ENABLE_MX28AR != 0U)
//    ret = MX28AR_SetGoalPositionAzimuthRealtime(motor_az_deg);
//    g_sound_motor_mx28ar_status = (uint32_t)ret;
//    if (ret != HAL_OK)
//    {
//        final_ret = ret;
//    }
//#endif
//
//#if (SOUND_MOTOR_ENABLE_FEEDBACK360 != 0U)
//    ret = Feedback360Servo_SetTargetAzimuthRealtime(motor_az_deg);
//    g_sound_motor_feedback360_status = (uint32_t)ret;
//
//    /* HAL_BUSY only means the 20 ms PWM update period has not elapsed yet. */
//    if ((ret != HAL_OK) && (ret != HAL_BUSY))
//    {
//        final_ret = ret;
//    }
//#endif
//
//    return final_ret;
//}
//
//void SoundMotorController_Update(void)
//{
//    doa_first_sound_target_t first_target;
//    float motor_az_deg;
//
//#if (SOUND_MOTOR_ENABLE_FEEDBACK360 != 0U)
//    Feedback360Servo_Task();
//#endif
//
//    if (DOA_GetFirstSoundMotorTarget(&first_target) == 0U)
//    {
//        SoundMotorController_ResetTarget();
//        return;
//    }
//
//    if (SoundMotor_GetBFTargetFromFirstSound(&first_target, &motor_az_deg) == 0U)
//    {
//        SoundMotorController_ResetTarget();
//        return;
//    }
//
//    (void)SoundMotorController_SetTargetAzimuthRealtime(motor_az_deg);
//}
































#include "SoundMotorController.h"
#include "SoundMotorConfig.h"
#include "SRP_PHAT.h"
#include "audio_aec_bf_pcm.h"
#include <math.h>

#if (SOUND_MOTOR_ENABLE_MX28AR != 0U)
#include "MX28AR.h"
#endif

#if (SOUND_MOTOR_ENABLE_FEEDBACK360 != 0U)
#include "Feedback360Servo.h"
#endif

/* ------------------------------------------------------------------------- */
/* Debug/watch variables                                                     */
/* ------------------------------------------------------------------------- */

volatile uint32_t g_sound_motor_target_valid = 0U;
volatile uint32_t g_sound_motor_stream = 0U;
volatile uint32_t g_sound_motor_update_count = 0U;
volatile uint32_t g_sound_motor_reset_count = 0U;
volatile uint32_t g_sound_motor_mx28ar_enabled = SOUND_MOTOR_ENABLE_MX28AR;
volatile uint32_t g_sound_motor_feedback360_enabled = SOUND_MOTOR_ENABLE_FEEDBACK360;
volatile uint32_t g_sound_motor_mx28ar_status = HAL_OK;
volatile uint32_t g_sound_motor_feedback360_status = HAL_OK;
volatile float g_sound_motor_srp_motor_az_deg = 0.0f;
volatile float g_sound_motor_bf_board_az_deg = 0.0f;
volatile float g_sound_motor_motor_az_deg = 0.0f;

/* Startup-home debug/watch variables. */
volatile uint32_t g_sound_motor_startup_home_success = 0U;
volatile uint32_t g_sound_motor_startup_home_timeout_count = 0U;
volatile uint32_t g_sound_motor_startup_home_elapsed_ms = 0U;
volatile uint32_t g_sound_motor_startup_home_stable_ms = 0U;
volatile float g_sound_motor_startup_home_angle_deg = 0.0f;
volatile float g_sound_motor_startup_home_error_deg = 0.0f;

/* Backward-compatible debug/watch names from the Feedback360-only version. */
volatile float g_feedback360_srp_motor_az_deg = 0.0f;
volatile float g_feedback360_bf_board_az_deg = 0.0f;
volatile float g_feedback360_bf_motor_az_deg = 0.0f;
volatile uint32_t g_feedback360_bf_motor_valid = 0U;
volatile uint32_t g_feedback360_bf_motor_stream = 0U;

static float SoundMotor_Wrap360(float x)
{
    while (x < 0.0f)
    {
        x += 360.0f;
    }

    while (x >= 360.0f)
    {
        x -= 360.0f;
    }

    return x;
}

static void SoundMotor_DebugSetTarget(const doa_first_sound_target_t *first_target,
                                      uint32_t stream_id,
                                      float bf_board_az_deg,
                                      float motor_az_deg)
{
    float srp_motor_az_deg = 0.0f;

    if (first_target != NULL)
    {
        srp_motor_az_deg = first_target->az_deg;
    }

    g_sound_motor_srp_motor_az_deg = srp_motor_az_deg;
    g_sound_motor_bf_board_az_deg = bf_board_az_deg;
    g_sound_motor_motor_az_deg = motor_az_deg;
    g_sound_motor_stream = stream_id;
    g_sound_motor_target_valid = 1U;
    g_sound_motor_update_count++;

    g_feedback360_srp_motor_az_deg = srp_motor_az_deg;
    g_feedback360_bf_board_az_deg = bf_board_az_deg;
    g_feedback360_bf_motor_az_deg = motor_az_deg;
    g_feedback360_bf_motor_stream = stream_id;
    g_feedback360_bf_motor_valid = 1U;
}

static void SoundMotor_DebugClearTarget(void)
{
    g_sound_motor_target_valid = 0U;
    g_feedback360_bf_motor_valid = 0U;
    g_sound_motor_reset_count++;
}

static uint8_t SoundMotor_GetBFTargetFromFirstSound(const doa_first_sound_target_t *first_target,
                                                    float *out_motor_az_deg)
{
    uint32_t stream_id;
    float bf_board_az_deg;
    float motor_az_deg;

    if ((first_target == NULL) || (out_motor_az_deg == NULL))
    {
        return 0U;
    }

    stream_id = (uint32_t)first_target->stream_id;
    if (stream_id >= AUDIO_STREAM_COUNT)
    {
        return 0U;
    }

    /*
     * g_bf_selected_pair_axis_deg_stream[] is updated by AudioBF_SetTarget...
     * and represents the actual selected directed mic-pair / BF steering axis.
     */
    if (g_bf_target_valid_stream[stream_id] == 0U)
    {
        return 0U;
    }

    bf_board_az_deg = SoundMotor_Wrap360(g_bf_selected_pair_axis_deg_stream[stream_id]);
    motor_az_deg = DOA_ConvertBoardAzToMotorAz(stream_id, bf_board_az_deg);

    SoundMotor_DebugSetTarget(first_target,
                              stream_id,
                              bf_board_az_deg,
                              motor_az_deg);

    *out_motor_az_deg = motor_az_deg;
    return 1U;
}

#define SOUND_MOTOR_STARTUP_HOME_DEG         0.0f
#define SOUND_MOTOR_STARTUP_TIMEOUT_MS       10000U
#define SOUND_MOTOR_STARTUP_TOLERANCE_DEG    2.0f
#define SOUND_MOTOR_STARTUP_STABLE_MS        300U

/*
 * Power-on startup protection.
 *
 * STM32 starts much faster than the Feedback360 servo.
 * Do not begin the 0-degree homing operation until valid feedback PWM
 * has actually been received continuously.
 */
#define SOUND_MOTOR_FEEDBACK_READY_TIMEOUT_MS    3000U
#define SOUND_MOTOR_FEEDBACK_READY_STABLE_MS      200U
#define SOUND_MOTOR_FEEDBACK_LOST_MS               50U
#define SOUND_MOTOR_FEEDBACK_WAIT_POLL_MS           5U

static HAL_StatusTypeDef SoundMotorController_WaitForFeedbackReady(void)
{
    uint32_t start_tick;
    uint32_t now;

    uint32_t last_capture_count;
    uint32_t last_capture_change_tick;

    uint32_t stable_start_tick = 0U;
    uint8_t stable_active = 0U;

    /*
     * Motor must stay stopped while we wait for its internal electronics
     * and feedback PWM output to become ready.
     */
    Feedback360Servo_Stop();

    start_tick = HAL_GetTick();

    last_capture_count = g_feedback360_capture_count;
    last_capture_change_tick = start_tick;

    while ((HAL_GetTick() - start_tick) <
           SOUND_MOTOR_FEEDBACK_READY_TIMEOUT_MS)
    {
        now = HAL_GetTick();

        /*
         * Do not rely only on feedback_valid.
         *
         * feedback_valid may become 1 after a single valid PWM capture.
         * Here we also verify that capture_count continues increasing,
         * which proves that feedback PWM is actually running.
         */
        if ((Feedback360Servo_IsFeedbackValid() != 0U) &&
            (g_feedback360_capture_count != last_capture_count))
        {
            last_capture_count = g_feedback360_capture_count;
            last_capture_change_tick = now;

            if (stable_active == 0U)
            {
                stable_active = 1U;
                stable_start_tick = now;
            }

            /*
             * Feedback has continued arriving normally for the required
             * stable period.
             */
            if ((now - stable_start_tick) >=
                SOUND_MOTOR_FEEDBACK_READY_STABLE_MS)
            {
                return HAL_OK;
            }
        }

        /*
         * Feedback was seen once but then stopped.
         * Start the stability check again.
         */
        if (stable_active != 0U)
        {
            if ((now - last_capture_change_tick) >
                SOUND_MOTOR_FEEDBACK_LOST_MS)
            {
                stable_active = 0U;
                stable_start_tick = 0U;

                /*
                 * Use the current value as the new reference.
                 */
                last_capture_count = g_feedback360_capture_count;
            }
        }

        HAL_Delay(SOUND_MOTOR_FEEDBACK_WAIT_POLL_MS);
    }

    /*
     * Feedback PWM did not become stable within the startup timeout.
     * Keep the motor safely stopped.
     */
    Feedback360Servo_Stop();

    return HAL_TIMEOUT;
}

/*
 * Startup home must not be treated as complete just because the motor passed
 * through the 0-degree tolerance once.  Keep the closed-loop target alive and
 * require the feedback angle to remain inside the tolerance continuously.
 *
 * Also, a timeout is a real initialization failure.  The old code stopped the
 * motor at whatever angle it had reached (for example 23 deg) and still
 * returned HAL_OK from SoundMotorController_Init().
 */
static HAL_StatusTypeDef SoundMotorController_MoveToStartupHome(void)
{
    uint32_t start_tick = HAL_GetTick();
    uint32_t stable_start_tick = 0U;
    uint8_t stable_active = 0U;

    g_sound_motor_startup_home_success = 0U;
    g_sound_motor_startup_home_elapsed_ms = 0U;
    g_sound_motor_startup_home_stable_ms = 0U;
    g_sound_motor_startup_home_angle_deg = Feedback360Servo_GetAngleDeg();
    g_sound_motor_startup_home_error_deg = 0.0f;

    while ((HAL_GetTick() - start_tick) < SOUND_MOTOR_STARTUP_TIMEOUT_MS)
    {
        uint32_t now = HAL_GetTick();

        /*
         * Re-issue the same absolute target.  Feedback360Servo itself limits
         * the real PWM update to FEEDBACK360_PWM_UPDATE_MS.
         */
        (void)Feedback360Servo_SetTargetAngle0To359(
            SOUND_MOTOR_STARTUP_HOME_DEG);

        if (Feedback360Servo_IsFeedbackValid() != 0U)
        {
            float current_deg = Feedback360Servo_GetAngleDeg();
            float error_deg = SOUND_MOTOR_STARTUP_HOME_DEG - current_deg;

            while (error_deg > 180.0f)
            {
                error_deg -= 360.0f;
            }
            while (error_deg < -180.0f)
            {
                error_deg += 360.0f;
            }

            g_sound_motor_startup_home_angle_deg = current_deg;
            g_sound_motor_startup_home_error_deg = error_deg;

            if (fabsf(error_deg) <= SOUND_MOTOR_STARTUP_TOLERANCE_DEG)
            {
                if (stable_active == 0U)
                {
                    stable_active = 1U;
                    stable_start_tick = now;
                    g_sound_motor_startup_home_stable_ms = 0U;
                }
                else
                {
                    g_sound_motor_startup_home_stable_ms = now - stable_start_tick;
                }

                if ((now - stable_start_tick) >= SOUND_MOTOR_STARTUP_STABLE_MS)
                {
                    g_sound_motor_startup_home_success = 1U;
                    g_sound_motor_startup_home_elapsed_ms = now - start_tick;

                    /*
                     * Startup home is the known cable-neutral orientation.
                     * Reset the unwrapped multi-turn counter exactly here so
                     * all later azimuth commands can be cable-safe.
                     */
                    Feedback360Servo_ResetCableReferenceAtCurrentAngle();

                    /*
                     * Keep target_valid set to 1.  The regular task will hold
                     * 0 deg until a new sound target arrives.
                     */
                    Feedback360Servo_Stop();
                    return HAL_OK;
                }
            }
            else
            {
                stable_active = 0U;
                stable_start_tick = 0U;
                g_sound_motor_startup_home_stable_ms = 0U;
            }
        }

        HAL_Delay(5U);
    }

    g_sound_motor_startup_home_elapsed_ms = HAL_GetTick() - start_tick;
    g_sound_motor_startup_home_timeout_count++;

    /* Do not silently report a failed home as successful initialization. */
    Feedback360Servo_ResetTarget();
    return HAL_TIMEOUT;
}

//HAL_StatusTypeDef SoundMotorController_Init(void)
//{
//    HAL_StatusTypeDef ret;
//
//#if (SOUND_MOTOR_ENABLE_FEEDBACK360 != 0U)
//    ret = Feedback360Servo_Init();
//    g_sound_motor_feedback360_status = (uint32_t)ret;
//    if (ret != HAL_OK)
//    {
//        return ret;
//    }
//#else
//    g_sound_motor_feedback360_status = HAL_OK;
//#endif
//
//#if (SOUND_MOTOR_ENABLE_MX28AR != 0U)
//    ret = MX28AR_InitRealtime();
//    g_sound_motor_mx28ar_status = (uint32_t)ret;
//    if (ret != HAL_OK)
//    {
//        return ret;
//    }
//#else
//    g_sound_motor_mx28ar_status = HAL_OK;
//#endif
//
//    SoundMotorController_ResetTarget();
//#if (SOUND_MOTOR_ENABLE_FEEDBACK360 != 0U)
//    ret = SoundMotorController_MoveToStartupHome();
//    g_sound_motor_feedback360_status = (uint32_t)ret;
//    if (ret != HAL_OK)
//    {
//        return ret;
//    }
//#endif
//    return HAL_OK;
//}

HAL_StatusTypeDef SoundMotorController_Init(void)
{
    HAL_StatusTypeDef ret;

#if (SOUND_MOTOR_ENABLE_FEEDBACK360 != 0U)

    /*
     * ----------------------------------------------------------
     * 1. Start PWM output and Feedback PWM input capture
     * ----------------------------------------------------------
     */
    ret = Feedback360Servo_Init();

    g_sound_motor_feedback360_status = (uint32_t)ret;

    if (ret != HAL_OK)
    {
        return ret;
    }

    /*
     * ----------------------------------------------------------
     * 2. Keep motor stopped and wait until Feedback360 itself
     *    has completely booted.
     *
     *    This replaces the fixed HAL_Delay(1000).
     * ----------------------------------------------------------
     */
    Feedback360Servo_Stop();

    ret = SoundMotorController_WaitForFeedbackReady();

    g_sound_motor_feedback360_status = (uint32_t)ret;

    if (ret != HAL_OK)
    {
        /*
         * Feedback PWM never became stable.
         * Keep motor stopped and report startup failure.
         */
        Feedback360Servo_Stop();
        return ret;
    }

#else

    g_sound_motor_feedback360_status = HAL_OK;

#endif


#if (SOUND_MOTOR_ENABLE_MX28AR != 0U)

    ret = MX28AR_InitRealtime();

    g_sound_motor_mx28ar_status = (uint32_t)ret;

    if (ret != HAL_OK)
    {
        return ret;
    }

#else

    g_sound_motor_mx28ar_status = HAL_OK;

#endif


    /*
     * ----------------------------------------------------------
     * 3. Clear previous sound/motor targets
     * ----------------------------------------------------------
     */
    SoundMotorController_ResetTarget();


#if (SOUND_MOTOR_ENABLE_FEEDBACK360 != 0U)

    /*
     * ----------------------------------------------------------
     * 4. Feedback PWM is now confirmed stable.
     *    Start 0-degree homing.
     * ----------------------------------------------------------
     */
    ret = SoundMotorController_MoveToStartupHome();

    g_sound_motor_feedback360_status = (uint32_t)ret;

    if (ret != HAL_OK)
    {
        /*
         * 0-degree homing itself failed.
         */
        Feedback360Servo_Stop();
        return ret;
    }

#endif


    return HAL_OK;
}

void SoundMotorController_ClearSourceTarget(void)
{
    /* Clear only the sound/BF source-valid debug state.  Do not cancel the
     * absolute Feedback360 position target that may still be in progress. */
    SoundMotor_DebugClearTarget();
}

void SoundMotorController_ResetTarget(void)
{
    SoundMotor_DebugClearTarget();

#if (SOUND_MOTOR_ENABLE_FEEDBACK360 != 0U)
    Feedback360Servo_ResetTarget();
#endif

    /*
     * MX28AR is a position servo. When the sound target disappears, keep the
     * last commanded position instead of torque-off, so the head does not sag
     * or free-spin. Call MX28AR_TorqueOff() manually if you want release mode.
     */
}

HAL_StatusTypeDef SoundMotorController_SetTargetAzimuthRealtime(float motor_az_deg)
{
    HAL_StatusTypeDef ret = HAL_OK;
    HAL_StatusTypeDef final_ret = HAL_OK;

    motor_az_deg = SoundMotor_Wrap360(motor_az_deg);
    g_sound_motor_motor_az_deg = motor_az_deg;

#if (SOUND_MOTOR_ENABLE_MX28AR != 0U)
    ret = MX28AR_SetGoalPositionAzimuthRealtime(motor_az_deg);
    g_sound_motor_mx28ar_status = (uint32_t)ret;
    if (ret != HAL_OK)
    {
        final_ret = ret;
    }
#endif

#if (SOUND_MOTOR_ENABLE_FEEDBACK360 != 0U)
    ret = Feedback360Servo_SetTargetAzimuthRealtime(motor_az_deg);
    g_sound_motor_feedback360_status = (uint32_t)ret;

    /* HAL_BUSY only means the 20 ms PWM update period has not elapsed yet. */
    if ((ret != HAL_OK) && (ret != HAL_BUSY))
    {
        final_ret = ret;
    }
#endif

    return final_ret;
}

void SoundMotorController_Task(void)
{
#if (SOUND_MOTOR_ENABLE_FEEDBACK360 != 0U)
    /*
     * Keep servicing the last absolute pan target even when the speech/DOA
     * target has already disappeared.  This is required because a Feedback360
     * servo is speed-controlled by PWM and needs closed-loop updates until the
     * requested angle is actually reached.
     */
    Feedback360Servo_Task();
#endif
}

void SoundMotorController_Update(void)
{
    doa_first_sound_target_t first_target;
    float motor_az_deg;

    SoundMotorController_Task();

    if (DOA_GetFirstSoundMotorTarget(&first_target) == 0U)
    {
        /* No new sound target: keep finishing/holding the previous pan target. */
        SoundMotorController_ClearSourceTarget();
        return;
    }

    if (SoundMotor_GetBFTargetFromFirstSound(&first_target, &motor_az_deg) == 0U)
    {
        /* BF target gap must not cancel an in-progress absolute pan move. */
        SoundMotorController_ClearSourceTarget();
        return;
    }

    (void)SoundMotorController_SetTargetAzimuthRealtime(motor_az_deg);
}

