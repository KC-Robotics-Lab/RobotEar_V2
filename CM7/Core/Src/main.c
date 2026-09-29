/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usbd_cdc_if.h"
#include "usbd_def.h"
#include "shared_audio.h"
#include "SRP_PHAT.h"
#include "LED.h"
#include "audio_aec_bf_pcm.h"
#include <string.h>
#include "D131MWServo.h"
#include "SoundMotorController.h"
#include "Feedback360AngleTest.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* DUAL_CORE_BOOT_SYNC_SEQUENCE: Define for dual core boot synchronization    */
/*                             demonstration code based on hardware semaphore */
/* This define is present in both CM7/CM4 projects                            */
/* To comment when developping/debugging on a single core                     */
#define DUAL_CORE_BOOT_SYNC_SEQUENCE

#if defined(DUAL_CORE_BOOT_SYNC_SEQUENCE)
#ifndef HSEM_ID_0
#define HSEM_ID_0 (0U) /* HW semaphore 0*/
#endif
#endif /* DUAL_CORE_BOOT_SYNC_SEQUENCE */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define HSEM_AUDIO_ID   (1U)
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
__attribute__((section(".shared"), aligned(32))) shared_audio_frame_t g_shared_audio;
systick Systime;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint8_t CDC_Transmit_FS_Blocking(uint8_t *Buf, uint16_t Len, uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();
    uint8_t  result;

    do
    {
        result = CDC_Transmit_FS(Buf, Len);
        if (result == USBD_OK)
        {
            return USBD_OK;
        }

        if (result == USBD_BUSY)
        {
            if ((HAL_GetTick() - start) > timeout_ms)
            {
                return USBD_BUSY;
            }
        }
        else
        {
            return result;
        }

    } while (1);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim->Instance == TIM6)
	{
	    Systime.Flag_1ms = 1;
		Systime.Count_1ms++;
		if((Systime.Count_1ms % 10) == 0) Systime.Flag_10ms = 1;
		if((Systime.Count_1ms % 20) == 0) Systime.Flag_20ms = 1;
		if((Systime.Count_1ms % 40) == 0) Systime.Flag_40ms = 1;
		if((Systime.Count_1ms % 50) == 0) Systime.Flag_50ms = 1;
		if((Systime.Count_1ms % 60) == 0) Systime.Flag_60ms = 1;
		if((Systime.Count_1ms % 80) == 0) Systime.Flag_80ms = 1;
		if((Systime.Count_1ms % 100) == 0) Systime.Flag_100ms = 1;
		if((Systime.Count_1ms % 200) == 0) Systime.Flag_200ms = 1;
		if((Systime.Count_1ms % 250) == 0) Systime.Flag_250ms = 1;
		if((Systime.Count_1ms % 500) == 0) Systime.Flag_500ms = 1;
		if(Systime.Count_1ms == 1000)
		{
			Systime.Flag_1000ms = 1;
			Systime.Count_1ms = 0;
		}
	}
}

#define BF_LATCH_ACTIVE_CONFIRM_FRAMES          1U
#define BF_LATCH_FULL_SILENCE_RELEASE_MS        1500U

typedef enum
{
    BF_LATCH_STATE_BEAM_RELEASED_WAIT_SOUND = 0,
    BF_LATCH_STATE_ARMING                   = 1,
    BF_LATCH_STATE_LOCKED                   = 2
} bf_latch_state_e;

typedef struct
{
    bf_latch_state_e state;

    uint8_t locked_valid;
    uint8_t beam_enabled;

    uint32_t active_count;
    uint32_t silence_start_ms;

    float candidate_az_deg;
    float locked_az_deg;

    uint32_t lock_count;
    uint32_t release_count;
    uint32_t apply_count;
} bf_latch_state_t;

static bf_latch_state_t s_bf_latch[AUDIO_STREAM_COUNT] = {0};

/*
 * Debug watch variables
 *
 * index 0 = L board
 * index 1 = R board
 */
volatile uint32_t g_bf_latch_state_stream[AUDIO_STREAM_COUNT] = {0U, 0U};
volatile uint32_t g_bf_latch_locked_valid_stream[AUDIO_STREAM_COUNT] = {0U, 0U};
volatile uint32_t g_bf_latch_beam_enabled_stream[AUDIO_STREAM_COUNT] = {0U, 0U};

volatile uint32_t g_bf_latch_audio_present_stream[AUDIO_STREAM_COUNT] = {0U, 0U};
volatile uint32_t g_bf_latch_track_active_stream[AUDIO_STREAM_COUNT] = {0U, 0U};

volatile uint32_t g_bf_latch_active_count_stream[AUDIO_STREAM_COUNT] = {0U, 0U};
volatile uint32_t g_bf_latch_silence_ms_stream[AUDIO_STREAM_COUNT] = {0U, 0U};

volatile uint32_t g_bf_latch_lock_count_stream[AUDIO_STREAM_COUNT] = {0U, 0U};
volatile uint32_t g_bf_latch_release_count_stream[AUDIO_STREAM_COUNT] = {0U, 0U};
volatile uint32_t g_bf_latch_apply_count_stream[AUDIO_STREAM_COUNT] = {0U, 0U};

volatile float g_bf_latch_track_az_stream[AUDIO_STREAM_COUNT] = {0.0f, 0.0f};
volatile float g_bf_latch_candidate_az_stream[AUDIO_STREAM_COUNT] = {0.0f, 0.0f};
volatile float g_bf_latch_locked_az_stream[AUDIO_STREAM_COUNT] = {0.0f, 0.0f};

static float BF_Latch_Wrap360(float x)
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

static float BF_Latch_AngleLerpDeg(float cur_deg, float target_deg, float alpha)
{
    float d;

    cur_deg = BF_Latch_Wrap360(cur_deg);
    target_deg = BF_Latch_Wrap360(target_deg);

    d = target_deg - cur_deg;

    while (d > 180.0f)
    {
        d -= 360.0f;
    }

    while (d < -180.0f)
    {
        d += 360.0f;
    }

    return BF_Latch_Wrap360(cur_deg + (alpha * d));
}

static void BF_Latch_ApplyTarget(uint32_t stream_id, float az_deg)
{
    if (stream_id >= AUDIO_STREAM_COUNT)
    {
        return;
    }

    AudioBF_SetTargetAzDegForStream(stream_id, az_deg);

    s_bf_latch[stream_id].beam_enabled = 1U;
    s_bf_latch[stream_id].apply_count++;

    g_bf_latch_beam_enabled_stream[stream_id] = 1U;
    g_bf_latch_apply_count_stream[stream_id] = s_bf_latch[stream_id].apply_count;
}

static void BF_Latch_ReleaseBeam(uint32_t stream_id)
{
    bf_latch_state_t *st;

    if (stream_id >= AUDIO_STREAM_COUNT)
    {
        return;
    }

    st = &s_bf_latch[stream_id];

    /*
     * 완전 무음 확정 시:
     * - BF 방향 lock 해제
     * - BF 빔 해제
     * - 다음 소리에서 새 SRP-PHAT 각도를 기다림
     */
    AudioBF_ClearTargetForStream(stream_id);

    st->state = BF_LATCH_STATE_BEAM_RELEASED_WAIT_SOUND;
    st->locked_valid = 0U;
    st->beam_enabled = 0U;
    st->active_count = 0U;
    st->silence_start_ms = 0U;
    st->candidate_az_deg = 0.0f;
    st->locked_az_deg = 0.0f;
    st->release_count++;

    g_bf_latch_state_stream[stream_id] = (uint32_t)st->state;
    g_bf_latch_locked_valid_stream[stream_id] = 0U;
    g_bf_latch_beam_enabled_stream[stream_id] = 0U;
    g_bf_latch_active_count_stream[stream_id] = 0U;
    g_bf_latch_silence_ms_stream[stream_id] = 0U;
    g_bf_latch_candidate_az_stream[stream_id] = 0.0f;
    g_bf_latch_locked_az_stream[stream_id] = 0.0f;
    g_bf_latch_release_count_stream[stream_id] = st->release_count;
}

static void BF_Latch_DebugUpdate(uint32_t stream_id,
                                 const doa_result_t *track,
                                 uint8_t audio_present)
{
    bf_latch_state_t *st;

    if (stream_id >= AUDIO_STREAM_COUNT)
    {
        return;
    }

    st = &s_bf_latch[stream_id];

    g_bf_latch_state_stream[stream_id] = (uint32_t)st->state;
    g_bf_latch_locked_valid_stream[stream_id] = (uint32_t)st->locked_valid;
    g_bf_latch_beam_enabled_stream[stream_id] = (uint32_t)st->beam_enabled;

    g_bf_latch_audio_present_stream[stream_id] = (uint32_t)audio_present;

    if (track != NULL)
    {
        g_bf_latch_track_active_stream[stream_id] = (uint32_t)track->active;
        g_bf_latch_track_az_stream[stream_id] = track->az_deg;
    }

    g_bf_latch_active_count_stream[stream_id] = st->active_count;
    g_bf_latch_lock_count_stream[stream_id] = st->lock_count;
    g_bf_latch_release_count_stream[stream_id] = st->release_count;
    g_bf_latch_apply_count_stream[stream_id] = st->apply_count;

    g_bf_latch_candidate_az_stream[stream_id] = st->candidate_az_deg;
    g_bf_latch_locked_az_stream[stream_id] = st->locked_az_deg;
}

static void BF_Latch_LockNow(uint32_t stream_id)
{
    bf_latch_state_t *st;

    if (stream_id >= AUDIO_STREAM_COUNT)
    {
        return;
    }

    st = &s_bf_latch[stream_id];

    st->state = BF_LATCH_STATE_LOCKED;
    st->locked_valid = 1U;
    st->beam_enabled = 1U;
    st->locked_az_deg = BF_Latch_Wrap360(st->candidate_az_deg);
    st->lock_count++;

    BF_Latch_ApplyTarget(stream_id, st->locked_az_deg);
}

static void BF_Latch_StartNewCandidate(uint32_t stream_id, float srp_az_deg)
{
    bf_latch_state_t *st;

    if (stream_id >= AUDIO_STREAM_COUNT)
    {
        return;
    }

    st = &s_bf_latch[stream_id];

    /*
     * 무음 이후 새 소리에서 처음 track->active == 1이 된 순간.
     * 여기서만 SRP-PHAT 각도를 새 BF 방향 후보로 사용한다.
     */
    st->state = BF_LATCH_STATE_ARMING;
    st->locked_valid = 0U;
    st->beam_enabled = 1U;
    st->active_count = 1U;
    st->silence_start_ms = 0U;
    st->candidate_az_deg = BF_Latch_Wrap360(srp_az_deg);

    BF_Latch_ApplyTarget(stream_id, st->candidate_az_deg);

    if (BF_LATCH_ACTIVE_CONFIRM_FRAMES <= 1U)
    {
        BF_Latch_LockNow(stream_id);
    }
}

static void BF_Latch_UpdateOneStream(uint32_t stream_id,
                                     const doa_result_t *track,
                                     uint8_t audio_present)
{
    bf_latch_state_t *st;
    uint32_t now;

    if ((stream_id >= AUDIO_STREAM_COUNT) || (track == NULL))
    {
        return;
    }

    st = &s_bf_latch[stream_id];
    now = HAL_GetTick();

    /*
     * 오디오가 있는 상태.
     * 이 구간에서는 완전 무음이 아니므로 BF를 절대 해제하지 않는다.
     */
    if (audio_present != 0U)
    {
        st->silence_start_ms = 0U;
        g_bf_latch_silence_ms_stream[stream_id] = 0U;

        if (st->state == BF_LATCH_STATE_BEAM_RELEASED_WAIT_SOUND)
        {
            /*
             * 빔 해제 상태에서 새 소리가 들어왔지만,
             * BF 방향은 track->active == 1일 때만 잡는다.
             */
            if (track->active != 0U)
            {
                BF_Latch_StartNewCandidate(stream_id, track->az_deg);
            }
        }
        else if (st->state == BF_LATCH_STATE_ARMING)
        {
            if (track->active != 0U)
            {
                st->active_count++;

                st->candidate_az_deg = BF_Latch_AngleLerpDeg(
                    st->candidate_az_deg,
                    track->az_deg,
                    0.5f
                );

                BF_Latch_ApplyTarget(stream_id, st->candidate_az_deg);

                if (st->active_count >= BF_LATCH_ACTIVE_CONFIRM_FRAMES)
                {
                    BF_Latch_LockNow(stream_id);
                }
            }
            else
            {
                /*
                 * 오디오는 있는데 SRP-PHAT track이 순간적으로 끊긴 상태.
                 * 빔을 해제하지 않고 마지막 candidate 방향으로 유지한다.
                 */
                BF_Latch_ApplyTarget(stream_id, st->candidate_az_deg);
            }
        }
        else
        {
            /*
             * LOCKED 상태.
             * 오디오가 움직여도, SRP-PHAT 각도가 변해도 locked_az_deg만 사용한다.
             */
            if (st->locked_valid != 0U)
            {
                BF_Latch_ApplyTarget(stream_id, st->locked_az_deg);
            }
        }

        BF_Latch_DebugUpdate(stream_id, track, audio_present);
        return;
    }

    /*
     * 여기부터 audio_present == 0.
     * 단, 바로 해제하지 않고 일정 시간 유지해서 순간적인 끊김을 방지한다.
     * 완전 무음으로 확정되기 전까지는 현재 BF 방향을 유지한다.
     */
    if (st->state == BF_LATCH_STATE_LOCKED)
    {
        if (st->locked_valid != 0U)
        {
            BF_Latch_ApplyTarget(stream_id, st->locked_az_deg);
        }
    }
    else if (st->state == BF_LATCH_STATE_ARMING)
    {
        BF_Latch_ApplyTarget(stream_id, st->candidate_az_deg);
    }
    else
    {
        /*
         * 이미 빔 해제 상태.
         */
        BF_Latch_DebugUpdate(stream_id, track, audio_present);
        return;
    }

    if (st->silence_start_ms == 0U)
    {
        st->silence_start_ms = now;
        g_bf_latch_silence_ms_stream[stream_id] = 0U;
    }
    else
    {
        uint32_t silence_ms = now - st->silence_start_ms;
        g_bf_latch_silence_ms_stream[stream_id] = silence_ms;

        if (silence_ms >= BF_LATCH_FULL_SILENCE_RELEASE_MS)
        {
            /*
             * 완전 무음 확정.
             * 여기서 BF 방향과 BF 빔을 해제한다.
             */
            BF_Latch_ReleaseBeam(stream_id);
        }
    }

    BF_Latch_DebugUpdate(stream_id, track, audio_present);
}

static void BF_Latch_UpdateFromDoaTracks(const doa_result_t doa_tracks[DOA_MAX_SOURCES])
{
    uint8_t audio_present_l;
    uint8_t audio_present_r;

    if (doa_tracks == NULL)
    {
        return;
    }

    audio_present_l = DOA_GetAudioPresentForStream(AUDIO_STREAM_L);
    audio_present_r = DOA_GetAudioPresentForStream(AUDIO_STREAM_R);

    BF_Latch_UpdateOneStream(AUDIO_STREAM_L,
                             &doa_tracks[AUDIO_STREAM_L],
                             audio_present_l);

    BF_Latch_UpdateOneStream(AUDIO_STREAM_R,
                             &doa_tracks[AUDIO_STREAM_R],
                             audio_present_r);
}

/*
 * One first-sound target is read, then the BF selected azimuth axis is used
 * for the azimuth motor.  D131MW elevation behavior is selected at build time.
 *
 *   BF selected azimuth axis -> MX28AR / Feedback360 azimuth motor
 *   common last elevation    -> D131MW elevation servo and USB packet
 *
 * Select one of the two modes below by changing BF_ELEVATION_CONTROL_MODE.
 *
 *   BF_ELEVATION_MODE_LATCH_ONCE
 *     - Existing behavior.
 *     - D131MW elevation is updated only once when the BF selected stream/axis
 *       changes.
 *     - While the BF axis is maintained, D131MW keeps the previously latched
 *       elevation and does not chase real-time el_deg or track_id noise.
 *
 *   BF_ELEVATION_MODE_REALTIME
 *     - D131MW follows the common last elevation value updated from
 *       target->el_deg while a valid BF target exists.
 *     - HitecD131MWServo_SetTdoaLsElevationRealtime() still applies its internal
 *       update period/deadband to reduce servo jitter.
 */
#define BF_ELEVATION_MODE_LATCH_ONCE               0U
#define BF_ELEVATION_MODE_REALTIME                 1U

#ifndef BF_ELEVATION_CONTROL_MODE
#define BF_ELEVATION_CONTROL_MODE                  BF_ELEVATION_MODE_LATCH_ONCE
#endif

#if ((BF_ELEVATION_CONTROL_MODE != BF_ELEVATION_MODE_LATCH_ONCE) && \
     (BF_ELEVATION_CONTROL_MODE != BF_ELEVATION_MODE_REALTIME))
#error "BF_ELEVATION_CONTROL_MODE must be BF_ELEVATION_MODE_LATCH_ONCE or BF_ELEVATION_MODE_REALTIME"
#endif

#define ELEVATION_MOTOR_CENTER_ON_LOST             0U
#define ELEVATION_MOTOR_CENTER_DEG                 45.0f
#define BF_ELEVATION_AXIS_CHANGE_THRESHOLD_DEG     1.0f
#define BF_ELEVATION_INVALID_STREAM_ID             0xFFFFFFFFU

///*
// * TILT one-shot latch stabilization.
// * - Ignore the first 3 genuinely new DOA elevation measurements.
// * - Then keep a rolling window of 7 measurements.
// * - Latch only when max-min of those 7 values is <= 6 degrees.
// * - The latched value is the median of the 7 measurements.
// * PAN latching remains immediate and unchanged.
// */
//#define ELEVATION_STABLE_IGNORE_SAMPLES            3U
//#define ELEVATION_STABLE_WINDOW_SIZE               7U
//#define ELEVATION_STABLE_MAX_SPREAD_DEG            6.0f
//
//volatile uint32_t g_elevation_motor_target_valid = 0U;
//volatile uint32_t g_elevation_motor_stream = 0U;
//volatile uint32_t g_elevation_motor_update_count = 0U;
//volatile float g_elevation_motor_tdoa_el_deg = ELEVATION_MOTOR_CENTER_DEG;
//
//volatile uint32_t g_sound_elevation_same_target_count = 0U;
//volatile uint32_t g_sound_elevation_no_target_count = 0U;
//volatile uint32_t g_sound_elevation_latch_update_count = 0U;
//volatile uint32_t g_sound_elevation_latch_hold_count = 0U;
//volatile uint32_t g_sound_elevation_realtime_update_count = 0U;
//volatile uint32_t g_sound_elevation_control_mode = BF_ELEVATION_CONTROL_MODE;
//volatile float g_sound_elevation_bf_board_az_deg = 0.0f;
//volatile float g_sound_elevation_motor_az_deg = 0.0f;
//volatile float g_sound_elevation_raw_el_deg = ELEVATION_MOTOR_CENTER_DEG;
//volatile float g_sound_elevation_el_deg = ELEVATION_MOTOR_CENTER_DEG;
//volatile uint32_t g_sound_motor_event_latch_valid = 0U;
//volatile uint32_t g_sound_motor_event_first_seq = 0U;
//volatile uint32_t g_sound_motor_event_latch_count = 0U;
//volatile float g_sound_motor_latched_pan_deg = 0.0f;
//volatile float g_sound_motor_latched_tilt_deg = ELEVATION_MOTOR_CENTER_DEG;
//
///* CubeIDE Watch variables for the stabilized TILT latch. */
//volatile uint32_t g_sound_elevation_stable_ignore_count = 0U;
//volatile uint32_t g_sound_elevation_stable_sample_count = 0U;
//volatile uint32_t g_sound_elevation_stable_latched = 0U;
//volatile uint32_t g_sound_elevation_stable_last_update_seq = 0U;
//volatile float g_sound_elevation_stable_last_raw_deg = ELEVATION_MOTOR_CENTER_DEG;
//volatile float g_sound_elevation_stable_min_deg = ELEVATION_MOTOR_CENTER_DEG;
//volatile float g_sound_elevation_stable_max_deg = ELEVATION_MOTOR_CENTER_DEG;
//volatile float g_sound_elevation_stable_spread_deg = 0.0f;
//volatile float g_sound_elevation_stable_median_deg = ELEVATION_MOTOR_CENTER_DEG;
//volatile float g_sound_elevation_stable_samples[ELEVATION_STABLE_WINDOW_SIZE] = {0.0f};
//
//static uint8_t s_elevation_motor_was_valid = 0U;
//static uint8_t s_bf_elevation_latch_valid = 0U;
//static uint32_t s_bf_elevation_latched_stream = BF_ELEVATION_INVALID_STREAM_ID;
//static float s_bf_elevation_latched_bf_axis_deg = 0.0f;
//static float s_bf_elevation_latched_el_deg = ELEVATION_MOTOR_CENTER_DEG;
//static uint8_t s_sound_motor_event_latch_valid = 0U;
//static uint32_t s_sound_motor_event_first_seq = 0U;
//static float s_sound_motor_latched_pan_deg = 0.0f;
//
//static uint32_t s_elevation_stable_ignore_count = 0U;
//static uint32_t s_elevation_stable_sample_count = 0U;
//static uint32_t s_elevation_stable_write_index = 0U;
//static uint32_t s_elevation_stable_last_update_seq = 0U;
//static uint8_t s_elevation_stable_latched = 0U;
//static float s_elevation_stable_samples[ELEVATION_STABLE_WINDOW_SIZE] = {0.0f};
//
//static float SoundElevation_AbsFloat(float x)
//{
//    return (x < 0.0f) ? -x : x;
//}
//
//static float SoundElevation_Wrap360(float x)
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
//static float SoundElevation_AngleDiffDeg(float a, float b)
//{
//    float d;
//
//    a = SoundElevation_Wrap360(a);
//    b = SoundElevation_Wrap360(b);
//
//    d = a - b;
//
//    while (d > 180.0f)
//    {
//        d -= 360.0f;
//    }
//
//    while (d < -180.0f)
//    {
//        d += 360.0f;
//    }
//
//    return SoundElevation_AbsFloat(d);
//}
//
//static void SoundElevation_StableLatchReset(void)
//{
//    s_elevation_stable_ignore_count = 0U;
//    s_elevation_stable_sample_count = 0U;
//    s_elevation_stable_write_index = 0U;
//    s_elevation_stable_last_update_seq = 0U;
//    s_elevation_stable_latched = 0U;
//    memset(s_elevation_stable_samples, 0, sizeof(s_elevation_stable_samples));
//
//    g_sound_elevation_stable_ignore_count = 0U;
//    g_sound_elevation_stable_sample_count = 0U;
//    g_sound_elevation_stable_latched = 0U;
//    g_sound_elevation_stable_last_update_seq = 0U;
//    g_sound_elevation_stable_last_raw_deg = ELEVATION_MOTOR_CENTER_DEG;
//    g_sound_elevation_stable_min_deg = ELEVATION_MOTOR_CENTER_DEG;
//    g_sound_elevation_stable_max_deg = ELEVATION_MOTOR_CENTER_DEG;
//    g_sound_elevation_stable_spread_deg = 0.0f;
//    g_sound_elevation_stable_median_deg = ELEVATION_MOTOR_CENTER_DEG;
//    memset((void *)g_sound_elevation_stable_samples, 0, sizeof(g_sound_elevation_stable_samples));
//}
//
//static float SoundElevation_MedianWindow(void)
//{
//    float sorted[ELEVATION_STABLE_WINDOW_SIZE];
//
//    for (uint32_t i = 0U; i < ELEVATION_STABLE_WINDOW_SIZE; i++)
//    {
//        sorted[i] = s_elevation_stable_samples[i];
//    }
//
//    /* Small fixed-size insertion sort: deterministic and cheap for 7 values. */
//    for (uint32_t i = 1U; i < ELEVATION_STABLE_WINDOW_SIZE; i++)
//    {
//        float key = sorted[i];
//        uint32_t j = i;
//
//        while ((j > 0U) && (sorted[j - 1U] > key))
//        {
//            sorted[j] = sorted[j - 1U];
//            j--;
//        }
//        sorted[j] = key;
//    }
//
//    return sorted[ELEVATION_STABLE_WINDOW_SIZE / 2U];
//}
//
//static void SoundAndElevationMotor_ResetTarget(void)
//{
////    SoundMotorController_ResetTarget();
//	SoundMotorController_ClearSourceTarget();
//
//    g_elevation_motor_target_valid = 0U;
//    g_sound_elevation_no_target_count++;
//
//    /* DOA first-sound target is cleared only after 1.5 s of real silence. */
//    s_sound_motor_event_latch_valid = 0U;
//    s_sound_motor_event_first_seq = 0U;
//    g_sound_motor_event_latch_valid = 0U;
//
//    SoundElevation_StableLatchReset();
//
//#if (ELEVATION_MOTOR_CENTER_ON_LOST != 0U)
//    if (s_elevation_motor_was_valid != 0U)
//    {
//        HitecD131MWServo_SetTdoaLsElevation(ELEVATION_MOTOR_CENTER_DEG);
//        AudioBF_SetLastElevationForStream(AUDIO_STREAM_L, ELEVATION_MOTOR_CENTER_DEG);
//        AudioBF_SetLastElevationForStream(AUDIO_STREAM_R, ELEVATION_MOTOR_CENTER_DEG);
//        s_bf_elevation_latched_el_deg = ELEVATION_MOTOR_CENTER_DEG;
//        g_elevation_motor_tdoa_el_deg = ELEVATION_MOTOR_CENTER_DEG;
//        g_sound_elevation_el_deg = ELEVATION_MOTOR_CENTER_DEG;
//    }
//
//    SoundAndElevationMotor_ClearLatch();
//#else
//    /*
//     * Lost target mode keeps the last D131MW position.
//     * The latch is intentionally kept, so short BF/DOA gaps do not cause
//     * unnecessary elevation re-latching or servo jitter.
//     */
//#endif
//
//    s_elevation_motor_was_valid = 0U;
//}
//
//static void SoundAndElevationMotor_LatchElevation(const doa_first_sound_target_t *target,
//                                                   float bf_board_az_deg)
//{
//    uint32_t stream_id = (uint32_t)target->stream_id;
//
//    s_bf_elevation_latch_valid = 1U;
//    s_bf_elevation_latched_stream = stream_id;
//    s_bf_elevation_latched_bf_axis_deg = bf_board_az_deg;
//
//    /*
//     * Store the exact elevation command in the common last-elevation holder.
//     * USB packets read the same holder, so the PC elevation value and the
//     * D131MW motor command are identical.
//     */
//    AudioBF_SetLastElevationForStream(stream_id, target->el_deg);
//    s_bf_elevation_latched_el_deg = AudioBF_GetLastElevationForStream(stream_id);
//
////    HitecD131MWServo_SetTdoaLsElevationRealtime(s_bf_elevation_latched_el_deg);
//    HitecD131MWServo_SetTargetTdoaLsElevation(s_bf_elevation_latched_el_deg);
//
//    g_sound_elevation_latch_update_count++;
//}
//
///*
// * Feed exactly one sample per newly processed DOA target.  The main loop runs
// * much faster than DOA, so update_seq prevents the same elevation value from
// * being counted repeatedly.
// */
//static uint8_t SoundAndElevationMotor_TryLatchStableElevation(
//    const doa_first_sound_target_t *target,
//    float bf_board_az_deg)
//{
//    float min_el;
//    float max_el;
//    float median_el;
//    doa_first_sound_target_t stable_target;
//
//    if ((target == NULL) || (s_elevation_stable_latched != 0U))
//    {
//        return 0U;
//    }
//
//    /* No newly calculated DOA frame since the previous main-loop pass. */
//    if (target->update_seq == s_elevation_stable_last_update_seq)
//    {
//        return 0U;
//    }
//
//    s_elevation_stable_last_update_seq = target->update_seq;
//    g_sound_elevation_stable_last_update_seq = target->update_seq;
//    g_sound_elevation_stable_last_raw_deg = target->el_deg;
//
//    /* Ignore the unstable first three elevation measurements of each event. */
//    if (s_elevation_stable_ignore_count < ELEVATION_STABLE_IGNORE_SAMPLES)
//    {
//        s_elevation_stable_ignore_count++;
//        g_sound_elevation_stable_ignore_count = s_elevation_stable_ignore_count;
//        return 0U;
//    }
//
//    s_elevation_stable_samples[s_elevation_stable_write_index] = target->el_deg;
//    g_sound_elevation_stable_samples[s_elevation_stable_write_index] = target->el_deg;
//
//    s_elevation_stable_write_index++;
//    if (s_elevation_stable_write_index >= ELEVATION_STABLE_WINDOW_SIZE)
//    {
//        s_elevation_stable_write_index = 0U;
//    }
//
//    if (s_elevation_stable_sample_count < ELEVATION_STABLE_WINDOW_SIZE)
//    {
//        s_elevation_stable_sample_count++;
//    }
//    g_sound_elevation_stable_sample_count = s_elevation_stable_sample_count;
//
//    if (s_elevation_stable_sample_count < ELEVATION_STABLE_WINDOW_SIZE)
//    {
//        return 0U;
//    }
//
//    min_el = s_elevation_stable_samples[0];
//    max_el = s_elevation_stable_samples[0];
//    for (uint32_t i = 1U; i < ELEVATION_STABLE_WINDOW_SIZE; i++)
//    {
//        if (s_elevation_stable_samples[i] < min_el)
//        {
//            min_el = s_elevation_stable_samples[i];
//        }
//        if (s_elevation_stable_samples[i] > max_el)
//        {
//            max_el = s_elevation_stable_samples[i];
//        }
//    }
//
//    median_el = SoundElevation_MedianWindow();
//
//    g_sound_elevation_stable_min_deg = min_el;
//    g_sound_elevation_stable_max_deg = max_el;
//    g_sound_elevation_stable_spread_deg = max_el - min_el;
//    g_sound_elevation_stable_median_deg = median_el;
//
//    /* Not converged yet: keep the rolling 7-sample window and try next DOA frame. */
//    if ((max_el - min_el) > ELEVATION_STABLE_MAX_SPREAD_DEG)
//    {
//        return 0U;
//    }
//
//    stable_target = *target;
//    stable_target.el_deg = median_el;
//    SoundAndElevationMotor_LatchElevation(&stable_target, bf_board_az_deg);
//
//    s_elevation_stable_latched = 1U;
//    g_sound_elevation_stable_latched = 1U;
//    return 1U;
//}
//
//static void SoundAndElevationMotor_UpdateDebug(const doa_first_sound_target_t *target,
//                                                float bf_board_az_deg,
//                                                float motor_az_deg,
//                                                float latched_el_deg,
//                                                uint8_t elevation_latched_now)
//{
//    uint32_t stream_id = (uint32_t)target->stream_id;
//
//    g_sound_motor_srp_motor_az_deg = target->az_deg;
//    g_sound_motor_bf_board_az_deg = bf_board_az_deg;
//    g_sound_motor_motor_az_deg = motor_az_deg;
//    g_sound_motor_stream = stream_id;
//    g_sound_motor_target_valid = 1U;
//    g_sound_motor_update_count++;
//
//    /* Backward-compatible watch variables. */
//    g_feedback360_srp_motor_az_deg = target->az_deg;
//    g_feedback360_bf_board_az_deg = bf_board_az_deg;
//    g_feedback360_bf_motor_az_deg = motor_az_deg;
//    g_feedback360_bf_motor_stream = stream_id;
//    g_feedback360_bf_motor_valid = 1U;
//
////    g_elevation_motor_target_valid = 1U;
//    g_elevation_motor_target_valid = (s_elevation_stable_latched != 0U) ? 1U : 0U;
//    g_elevation_motor_stream = stream_id;
//    g_elevation_motor_tdoa_el_deg = latched_el_deg;
//    if (elevation_latched_now != 0U)
//    {
//        g_elevation_motor_update_count++;
//    }
//
//    g_sound_elevation_same_target_count++;
//    g_sound_elevation_bf_board_az_deg = bf_board_az_deg;
//    g_sound_elevation_motor_az_deg = motor_az_deg;
//    g_sound_elevation_raw_el_deg = target->el_deg;
//    g_sound_elevation_el_deg = latched_el_deg;
//
//    if (elevation_latched_now == 0U)
//    {
//        g_sound_elevation_latch_hold_count++;
//    }
//}
//
//static void SoundAndElevationMotor_UpdateFromFirstSound(void)
//{
//    doa_first_sound_target_t target;
//    float motor_az_deg;
//    float debug_board_az_deg;
//    uint8_t elevation_latched_now = 0U;
//
//    /*
//     * PAN is a feedback-controlled continuous servo, so it must keep running
//     * its closed-loop task until the latched position is reached.
//     * TILT uses the same pattern at the command level: its Task() walks the
//     * current commanded elevation toward the one-shot latched elevation.
//     */
//    SoundMotorController_Task();
//    HitecD131MWServo_Task();
//
//    if (DOA_GetFirstSoundMotorTarget(&target) == 0U)
//    {
//        SoundAndElevationMotor_ResetTarget();
//        return;
//    }
//
//    /*
//     * IMPORTANT: motor targets no longer depend on BF selected pair/axis.
//     * target.az_deg is already converted to the motor azimuth frame by
//     * DOA_FirstSoundUpdateTarget(). target.el_deg is the TDOA-LS elevation.
//     *
//     * PAN latches immediately once per sound event. TILT intentionally waits
//     * for several newly calculated elevation frames and latches only after the
//     * values converge. The first-sound event resets after 1.5 s of real silence.
//     */
//    if ((s_sound_motor_event_latch_valid == 0U) ||
//        (s_sound_motor_event_first_seq != target.first_seq))
//    {
//        motor_az_deg = SoundElevation_Wrap360(target.az_deg);
//        debug_board_az_deg = motor_az_deg;
//
//        /* PAN behavior is intentionally unchanged: latch immediately once. */
//        s_sound_motor_event_latch_valid = 1U;
//        s_sound_motor_event_first_seq = target.first_seq;
//        s_sound_motor_latched_pan_deg = motor_az_deg;
//        (void)SoundMotorController_SetTargetAzimuthRealtime(s_sound_motor_latched_pan_deg);
//
//        /*
//         * TILT starts a fresh stabilization window for this sound event.
//         * The old physical TILT position is held until the new elevation is stable.
//         */
//        SoundElevation_StableLatchReset();
//
//        g_sound_motor_event_latch_valid = 1U;
//        g_sound_motor_event_first_seq = target.first_seq;
//        g_sound_motor_event_latch_count++;
//        g_sound_motor_latched_pan_deg = s_sound_motor_latched_pan_deg;
//    }
//    else
//    {
//        /* Same event: PAN stays at its one-shot latched position. */
//        motor_az_deg = s_sound_motor_latched_pan_deg;
//        debug_board_az_deg = motor_az_deg;
//    }
//
//    /*
//     * TILT only: ignore first 3 new DOA measurements, then examine a rolling
//     * 7-sample window. Once max-min <= 6 deg, latch the median exactly once.
//     */
//    if (s_elevation_stable_latched == 0U)
//    {
//        if (SoundAndElevationMotor_TryLatchStableElevation(&target,
//                                                           debug_board_az_deg) != 0U)
//        {
//            elevation_latched_now = 1U;
//            g_sound_motor_latched_tilt_deg = s_bf_elevation_latched_el_deg;
//        }
//    }
//
//    s_elevation_motor_was_valid = 1U;
//
//    SoundAndElevationMotor_UpdateDebug(&target,
//                                       debug_board_az_deg,
//                                       motor_az_deg,
//                                       s_bf_elevation_latched_el_deg,
//                                       elevation_latched_now);
//}







/*
 * TILT TDOA-LS multi-frame selection.
 *
 * PAN remains an immediate one-shot latch.
 * TILT collects 12 genuinely new, valid raw TDOA-LS frames.  After 12 are
 * collected, it computes the elevation median, keeps only samples within
 * +/-5 degrees of that median, and selects the REAL measured frame with the
 * smallest TDOA-LS RMS residual.  No averaged/median angle is sent to motor.
 */
#define TILT_TDOA_SELECT_FRAME_COUNT               12U
#define TILT_TDOA_MEDIAN_GATE_DEG                  5.0f
#define TILT_TDOA_RESID_MAX_M                      0.012f

volatile uint32_t g_elevation_motor_target_valid = 0U;
volatile uint32_t g_elevation_motor_stream = 0U;
volatile uint32_t g_elevation_motor_update_count = 0U;
volatile float g_elevation_motor_tdoa_el_deg = ELEVATION_MOTOR_CENTER_DEG;

volatile uint32_t g_sound_elevation_same_target_count = 0U;
volatile uint32_t g_sound_elevation_no_target_count = 0U;
volatile uint32_t g_sound_elevation_latch_update_count = 0U;
volatile uint32_t g_sound_elevation_latch_hold_count = 0U;
volatile uint32_t g_sound_elevation_realtime_update_count = 0U;
volatile uint32_t g_sound_elevation_control_mode = BF_ELEVATION_CONTROL_MODE;
volatile float g_sound_elevation_bf_board_az_deg = 0.0f;
volatile float g_sound_elevation_motor_az_deg = 0.0f;
volatile float g_sound_elevation_raw_el_deg = ELEVATION_MOTOR_CENTER_DEG;
volatile float g_sound_elevation_el_deg = ELEVATION_MOTOR_CENTER_DEG;
volatile uint32_t g_sound_motor_event_latch_valid = 0U;
volatile uint32_t g_sound_motor_event_first_seq = 0U;
volatile uint32_t g_sound_motor_event_latch_count = 0U;
volatile float g_sound_motor_latched_pan_deg = 0.0f;
volatile float g_sound_motor_latched_tilt_deg = ELEVATION_MOTOR_CENTER_DEG;

/* CubeIDE Watch variables for TDOA-LS best-frame TILT selection. */
volatile uint32_t g_sound_elevation_select_sample_count = 0U;
volatile uint32_t g_sound_elevation_select_rejected_count = 0U;
volatile uint32_t g_sound_elevation_select_latched = 0U;
volatile uint32_t g_sound_elevation_select_last_update_seq = 0U;
volatile uint32_t g_sound_elevation_select_best_index = 0U;
volatile float g_sound_elevation_select_last_raw_deg = ELEVATION_MOTOR_CENTER_DEG;
volatile float g_sound_elevation_select_last_resid_m = 0.0f;
volatile float g_sound_elevation_select_median_deg = ELEVATION_MOTOR_CENTER_DEG;
volatile float g_sound_elevation_select_best_deg = ELEVATION_MOTOR_CENTER_DEG;
volatile float g_sound_elevation_select_best_resid_m = 0.0f;
volatile float g_sound_elevation_select_samples[TILT_TDOA_SELECT_FRAME_COUNT] = {0.0f};
volatile float g_sound_elevation_select_residuals[TILT_TDOA_SELECT_FRAME_COUNT] = {0.0f};

static uint8_t s_elevation_motor_was_valid = 0U;
static uint8_t s_bf_elevation_latch_valid = 0U;
static uint32_t s_bf_elevation_latched_stream = BF_ELEVATION_INVALID_STREAM_ID;
static float s_bf_elevation_latched_bf_axis_deg = 0.0f;
static float s_bf_elevation_latched_el_deg = ELEVATION_MOTOR_CENTER_DEG;
static uint8_t s_sound_motor_event_latch_valid = 0U;
static uint32_t s_sound_motor_event_first_seq = 0U;
static float s_sound_motor_latched_pan_deg = 0.0f;

static uint32_t s_elevation_select_sample_count = 0U;
static uint32_t s_elevation_select_last_update_seq = 0U;
static uint8_t s_elevation_select_latched = 0U;
static float s_elevation_select_samples[TILT_TDOA_SELECT_FRAME_COUNT] = {0.0f};
static float s_elevation_select_residuals[TILT_TDOA_SELECT_FRAME_COUNT] = {0.0f};

static float SoundElevation_AbsFloat(float x)
{
    return (x < 0.0f) ? -x : x;
}

static float SoundElevation_Wrap360(float x)
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

static float SoundElevation_AngleDiffDeg(float a, float b)
{
    float d;

    a = SoundElevation_Wrap360(a);
    b = SoundElevation_Wrap360(b);

    d = a - b;

    while (d > 180.0f)
    {
        d -= 360.0f;
    }

    while (d < -180.0f)
    {
        d += 360.0f;
    }

    return SoundElevation_AbsFloat(d);
}

static void SoundElevation_BestFrameReset(void)
{
    s_elevation_select_sample_count = 0U;
    s_elevation_select_last_update_seq = 0U;
    s_elevation_select_latched = 0U;
    memset(s_elevation_select_samples, 0, sizeof(s_elevation_select_samples));
    memset(s_elevation_select_residuals, 0, sizeof(s_elevation_select_residuals));

    g_sound_elevation_select_sample_count = 0U;
    g_sound_elevation_select_rejected_count = 0U;
    g_sound_elevation_select_latched = 0U;
    g_sound_elevation_select_last_update_seq = 0U;
    g_sound_elevation_select_best_index = 0U;
    g_sound_elevation_select_last_raw_deg = ELEVATION_MOTOR_CENTER_DEG;
    g_sound_elevation_select_last_resid_m = 0.0f;
    g_sound_elevation_select_median_deg = ELEVATION_MOTOR_CENTER_DEG;
    g_sound_elevation_select_best_deg = ELEVATION_MOTOR_CENTER_DEG;
    g_sound_elevation_select_best_resid_m = 0.0f;
    memset((void *)g_sound_elevation_select_samples, 0, sizeof(g_sound_elevation_select_samples));
    memset((void *)g_sound_elevation_select_residuals, 0, sizeof(g_sound_elevation_select_residuals));
}

static float SoundElevation_SelectMedian(void)
{
    float sorted[TILT_TDOA_SELECT_FRAME_COUNT];

    for (uint32_t i = 0U; i < TILT_TDOA_SELECT_FRAME_COUNT; i++)
    {
        sorted[i] = s_elevation_select_samples[i];
    }

    /* Small fixed-size insertion sort: deterministic for 12 values. */
    for (uint32_t i = 1U; i < TILT_TDOA_SELECT_FRAME_COUNT; i++)
    {
        float key = sorted[i];
        uint32_t j = i;

        while ((j > 0U) && (sorted[j - 1U] > key))
        {
            sorted[j] = sorted[j - 1U];
            j--;
        }
        sorted[j] = key;
    }

#if ((TILT_TDOA_SELECT_FRAME_COUNT & 1U) != 0U)
    return sorted[TILT_TDOA_SELECT_FRAME_COUNT / 2U];
#else
    return 0.5f * (sorted[(TILT_TDOA_SELECT_FRAME_COUNT / 2U) - 1U] +
                   sorted[TILT_TDOA_SELECT_FRAME_COUNT / 2U]);
#endif
}

static void SoundAndElevationMotor_ResetTarget(void)
{
//    SoundMotorController_ResetTarget();
	SoundMotorController_ClearSourceTarget();

    g_elevation_motor_target_valid = 0U;
    g_sound_elevation_no_target_count++;

    /* DOA first-sound target is cleared only after 1.5 s of real silence. */
    s_sound_motor_event_latch_valid = 0U;
    s_sound_motor_event_first_seq = 0U;
    g_sound_motor_event_latch_valid = 0U;

    SoundElevation_BestFrameReset();

#if (ELEVATION_MOTOR_CENTER_ON_LOST != 0U)
    if (s_elevation_motor_was_valid != 0U)
    {
        HitecD131MWServo_SetTdoaLsElevation(ELEVATION_MOTOR_CENTER_DEG);
        AudioBF_SetLastElevationForStream(AUDIO_STREAM_L, ELEVATION_MOTOR_CENTER_DEG);
        AudioBF_SetLastElevationForStream(AUDIO_STREAM_R, ELEVATION_MOTOR_CENTER_DEG);
        s_bf_elevation_latched_el_deg = ELEVATION_MOTOR_CENTER_DEG;
        g_elevation_motor_tdoa_el_deg = ELEVATION_MOTOR_CENTER_DEG;
        g_sound_elevation_el_deg = ELEVATION_MOTOR_CENTER_DEG;
    }

    SoundAndElevationMotor_ClearLatch();
#else
    /*
     * Lost target mode keeps the last D131MW position.
     * The latch is intentionally kept, so short BF/DOA gaps do not cause
     * unnecessary elevation re-latching or servo jitter.
     */
#endif

    s_elevation_motor_was_valid = 0U;
}

static void SoundAndElevationMotor_LatchElevation(const doa_first_sound_target_t *target,
                                                   float bf_board_az_deg)
{
    uint32_t stream_id = (uint32_t)target->stream_id;

    s_bf_elevation_latch_valid = 1U;
    s_bf_elevation_latched_stream = stream_id;
    s_bf_elevation_latched_bf_axis_deg = bf_board_az_deg;

    /*
     * Store the exact elevation command in the common last-elevation holder.
     * USB packets read the same holder, so the PC elevation value and the
     * D131MW motor command are identical.
     */
    AudioBF_SetLastElevationForStream(stream_id, target->el_deg);
    s_bf_elevation_latched_el_deg = AudioBF_GetLastElevationForStream(stream_id);

//    HitecD131MWServo_SetTdoaLsElevationRealtime(s_bf_elevation_latched_el_deg);
    HitecD131MWServo_SetTargetTdoaLsElevation(s_bf_elevation_latched_el_deg);

    g_sound_elevation_latch_update_count++;
}

/*
 * Collect one RAW TDOA-LS measurement per newly processed DOA target.
 * After 12 accepted frames, choose one actual measured elevation:
 *   1) calculate median elevation of 12 frames
 *   2) keep frames within median +/- 5 deg
 *   3) among them, choose the frame with the minimum real LS RMS residual
 *
 * update_seq prevents the fast main loop from counting one DOA result more
 * than once.  Invalid or very poor-residual frames are not counted.
 */
static uint8_t SoundAndElevationMotor_TrySelectBestTdoaElevation(
    const doa_first_sound_target_t *target,
    float bf_board_az_deg)
{
    float median_el;
    float best_el = ELEVATION_MOTOR_CENTER_DEG;
    float best_resid = 1e30f;
    uint32_t best_index = 0U;
    uint8_t found = 0U;
    doa_first_sound_target_t best_target;

    if ((target == NULL) || (s_elevation_select_latched != 0U))
    {
        return 0U;
    }

    /* No newly calculated DOA frame since the previous main-loop pass. */
    if (target->update_seq == s_elevation_select_last_update_seq)
    {
        return 0U;
    }

    s_elevation_select_last_update_seq = target->update_seq;
    g_sound_elevation_select_last_update_seq = target->update_seq;
    g_sound_elevation_select_last_raw_deg = target->el_raw_deg;
    g_sound_elevation_select_last_resid_m = target->el_resid;

    /* Only actual, physically valid raw TDOA-LS frames enter the selector. */
    if ((target->el_valid == 0U) ||
        (target->el_raw_deg < 0.0f) ||
        (target->el_raw_deg > 90.0f) ||
        (target->el_resid < 0.0f) ||
        (target->el_resid > TILT_TDOA_RESID_MAX_M))
    {
        g_sound_elevation_select_rejected_count++;
        return 0U;
    }

    if (s_elevation_select_sample_count < TILT_TDOA_SELECT_FRAME_COUNT)
    {
        uint32_t i = s_elevation_select_sample_count;

        s_elevation_select_samples[i] = target->el_raw_deg;
        s_elevation_select_residuals[i] = target->el_resid;
        g_sound_elevation_select_samples[i] = target->el_raw_deg;
        g_sound_elevation_select_residuals[i] = target->el_resid;

        s_elevation_select_sample_count++;
        g_sound_elevation_select_sample_count = s_elevation_select_sample_count;
    }

    if (s_elevation_select_sample_count < TILT_TDOA_SELECT_FRAME_COUNT)
    {
        return 0U;
    }

    median_el = SoundElevation_SelectMedian();
    g_sound_elevation_select_median_deg = median_el;

    /*
     * Do not send the median itself.  Select one REAL TDOA-LS frame near the
     * population center, then use residual only to rank those plausible frames.
     */
    for (uint32_t i = 0U; i < TILT_TDOA_SELECT_FRAME_COUNT; i++)
    {
        float diff = SoundElevation_AbsFloat(s_elevation_select_samples[i] - median_el);

        if (diff > TILT_TDOA_MEDIAN_GATE_DEG)
        {
            continue;
        }

        if ((found == 0U) || (s_elevation_select_residuals[i] < best_resid))
        {
            best_el = s_elevation_select_samples[i];
            best_resid = s_elevation_select_residuals[i];
            best_index = i;
            found = 1U;
        }
    }

    /* Median gate should normally leave candidates; keep a deterministic fallback. */
    if (found == 0U)
    {
        float best_diff = 1e30f;
        for (uint32_t i = 0U; i < TILT_TDOA_SELECT_FRAME_COUNT; i++)
        {
            float diff = SoundElevation_AbsFloat(s_elevation_select_samples[i] - median_el);
            if (diff < best_diff)
            {
                best_diff = diff;
                best_el = s_elevation_select_samples[i];
                best_resid = s_elevation_select_residuals[i];
                best_index = i;
            }
        }
    }

    g_sound_elevation_select_best_index = best_index;
    g_sound_elevation_select_best_deg = best_el;
    g_sound_elevation_select_best_resid_m = best_resid;

    best_target = *target;
    best_target.el_deg = best_el;
    best_target.el_raw_deg = best_el;
    best_target.el_resid = best_resid;
    best_target.el_valid = 1U;

    SoundAndElevationMotor_LatchElevation(&best_target, bf_board_az_deg);

    s_elevation_select_latched = 1U;
    g_sound_elevation_select_latched = 1U;
    return 1U;
}

static void SoundAndElevationMotor_UpdateDebug(const doa_first_sound_target_t *target,
                                                float bf_board_az_deg,
                                                float motor_az_deg,
                                                float latched_el_deg,
                                                uint8_t elevation_latched_now)
{
    uint32_t stream_id = (uint32_t)target->stream_id;

    g_sound_motor_srp_motor_az_deg = target->az_deg;
    g_sound_motor_bf_board_az_deg = bf_board_az_deg;
    g_sound_motor_motor_az_deg = motor_az_deg;
    g_sound_motor_stream = stream_id;
    g_sound_motor_target_valid = 1U;
    g_sound_motor_update_count++;

    /* Backward-compatible watch variables. */
    g_feedback360_srp_motor_az_deg = target->az_deg;
    g_feedback360_bf_board_az_deg = bf_board_az_deg;
    g_feedback360_bf_motor_az_deg = motor_az_deg;
    g_feedback360_bf_motor_stream = stream_id;
    g_feedback360_bf_motor_valid = 1U;

//    g_elevation_motor_target_valid = 1U;
    g_elevation_motor_target_valid = (s_elevation_select_latched != 0U) ? 1U : 0U;
    g_elevation_motor_stream = stream_id;
    g_elevation_motor_tdoa_el_deg = latched_el_deg;
    if (elevation_latched_now != 0U)
    {
        g_elevation_motor_update_count++;
    }

    g_sound_elevation_same_target_count++;
    g_sound_elevation_bf_board_az_deg = bf_board_az_deg;
    g_sound_elevation_motor_az_deg = motor_az_deg;
    g_sound_elevation_raw_el_deg = target->el_raw_deg;
    g_sound_elevation_el_deg = latched_el_deg;

    if (elevation_latched_now == 0U)
    {
        g_sound_elevation_latch_hold_count++;
    }
}

static void SoundAndElevationMotor_UpdateFromFirstSound(void)
{
    doa_first_sound_target_t target;
    float motor_az_deg;
    float debug_board_az_deg;
    uint8_t elevation_latched_now = 0U;

    /*
     * PAN is a feedback-controlled continuous servo, so it must keep running
     * its closed-loop task until the latched position is reached.
     * TILT uses the same pattern at the command level: its Task() walks the
     * current commanded elevation toward the one-shot latched elevation.
     */
    SoundMotorController_Task();
    HitecD131MWServo_Task();

    if (DOA_GetFirstSoundMotorTarget(&target) == 0U)
    {
        SoundAndElevationMotor_ResetTarget();
        return;
    }

    /*
     * IMPORTANT: motor targets no longer depend on BF selected pair/axis.
     * target.az_deg is already converted to the motor azimuth frame by
     * DOA_FirstSoundUpdateTarget(). target.el_deg is the TDOA-LS elevation.
     *
     * PAN latches immediately once per sound event. TILT intentionally collects
     * multiple raw TDOA-LS frames and chooses the best actual measured frame.
     * The first-sound event resets after 1.5 s of real silence.
     */
    if ((s_sound_motor_event_latch_valid == 0U) ||
        (s_sound_motor_event_first_seq != target.first_seq))
    {
        motor_az_deg = SoundElevation_Wrap360(target.az_deg);
        debug_board_az_deg = motor_az_deg;

        /* PAN behavior is intentionally unchanged: latch immediately once. */
        s_sound_motor_event_latch_valid = 1U;
        s_sound_motor_event_first_seq = target.first_seq;
        s_sound_motor_latched_pan_deg = motor_az_deg;
        (void)SoundMotorController_SetTargetAzimuthRealtime(s_sound_motor_latched_pan_deg);

        LED_R_op(SoundElevation_Wrap360(target.srp_az_deg));

        /*
         * TILT starts a fresh 12-frame TDOA-LS quality-selection window.
         * The old physical TILT position is held until the best frame is chosen.
         */
        SoundElevation_BestFrameReset();

        g_sound_motor_event_latch_valid = 1U;
        g_sound_motor_event_first_seq = target.first_seq;
        g_sound_motor_event_latch_count++;
        g_sound_motor_latched_pan_deg = s_sound_motor_latched_pan_deg;
    }
    else
    {
        /* Same event: PAN stays at its one-shot latched position. */
        motor_az_deg = s_sound_motor_latched_pan_deg;
        debug_board_az_deg = motor_az_deg;
    }

    /*
     * TILT only: collect 12 valid raw TDOA-LS frames.  The median is used only
     * as an outlier gate; the actual motor command is the measured frame near
     * that median with the smallest LS RMS residual.
     */
    if (s_elevation_select_latched == 0U)
    {
        if (SoundAndElevationMotor_TrySelectBestTdoaElevation(&target,
                                                           debug_board_az_deg) != 0U)
        {
            elevation_latched_now = 1U;
            g_sound_motor_latched_tilt_deg = s_bf_elevation_latched_el_deg;
        }
    }

    s_elevation_motor_was_valid = 1U;

    SoundAndElevationMotor_UpdateDebug(&target,
                                       debug_board_az_deg,
                                       motor_az_deg,
                                       s_bf_elevation_latched_el_deg,
                                       elevation_latched_now);
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */
/* USER CODE BEGIN Boot_Mode_Sequence_0 */
#if defined(DUAL_CORE_BOOT_SYNC_SEQUENCE)
  int32_t timeout;
#endif /* DUAL_CORE_BOOT_SYNC_SEQUENCE */
/* USER CODE END Boot_Mode_Sequence_0 */

  /* Enable the CPU Cache */

  /* Enable I-Cache---------------------------------------------------------*/
  SCB_EnableICache();

  /* Enable D-Cache---------------------------------------------------------*/
  SCB_EnableDCache();

/* USER CODE BEGIN Boot_Mode_Sequence_1 */
#if defined(DUAL_CORE_BOOT_SYNC_SEQUENCE)
  /* Wait until CPU2 boots and enters in stop mode or timeout*/
  timeout = 0xFFFF;
  while((__HAL_RCC_GET_FLAG(RCC_FLAG_D2CKRDY) != RESET) && (timeout-- > 0));
  if ( timeout < 0 )
  {
  Error_Handler();
  }
#endif /* DUAL_CORE_BOOT_SYNC_SEQUENCE */
/* USER CODE END Boot_Mode_Sequence_1 */
  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* Configure the peripherals common clocks */
  PeriphCommonClock_Config();
/* USER CODE BEGIN Boot_Mode_Sequence_2 */
#if defined(DUAL_CORE_BOOT_SYNC_SEQUENCE)
/* When system initialization is finished, Cortex-M7 will release Cortex-M4 by means of
HSEM notification */
/*HW semaphore Clock enable*/
__HAL_RCC_HSEM_CLK_ENABLE();

memset((void *)&g_shared_audio, 0, sizeof(g_shared_audio));
SCB_CleanDCache_by_Addr((uint32_t *)&g_shared_audio, sizeof(g_shared_audio));

__DMB();
/*Take HSEM */
HAL_HSEM_FastTake(HSEM_ID_0);
/*Release HSEM in order to notify the CPU2(CM4)*/
HAL_HSEM_Release(HSEM_ID_0,0);
/* wait until CPU2 wakes up from stop mode */
timeout = 0xFFFF;
while((__HAL_RCC_GET_FLAG(RCC_FLAG_D2CKRDY) == RESET) && (timeout-- > 0));
if ( timeout < 0 )
{
Error_Handler();
}
#endif /* DUAL_CORE_BOOT_SYNC_SEQUENCE */
/* USER CODE END Boot_Mode_Sequence_2 */

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_TIM6_Init();
  MX_USB_DEVICE_Init();
  MX_USART1_UART_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
  LED_L_Init();
  LED_R_Init();

  HAL_HSEM_ActivateNotification(__HAL_HSEM_SEMID_TO_MASK(HSEM_AUDIO_ID));
  doa_init();

  HAL_TIM_Base_Start_IT(&htim6);

#if (FEEDBACK360_ANGLE_TEST_ENABLE != 0U)
  /*
   * Standalone pan motor test mode.
   * Feedback360AngleTest.c controls TIM2_CH2/TIM3 feedback and repeats
   * 0 -> 90 -> 180 -> 270 deg.  Normal sound tracking motor output is bypassed.
   */
  HitecD131MWServo_Init();

  if (Feedback360AngleTest_Init() != HAL_OK)
  {
      Error_Handler();
  }
#else
  Audio_Libraries_Init();
//  MX28AR_TorqueOn();

  HitecD131MWServo_Init();
  HitecD131MWServo_SetAngle(0.0f);

//  HAL_Delay(1000);			// 타이밍 딜레이

  if (SoundMotorController_Init() != HAL_OK)
  {
      Error_Handler();
  }
#endif
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
#if (FEEDBACK360_ANGLE_TEST_ENABLE != 0U)
      Feedback360AngleTest_Task();
#else
	  Degree_Proccess();

	  doa_result_t doa_tracks[DOA_MAX_SOURCES];

	  memset(doa_tracks, 0, sizeof(doa_tracks));

	  (void)DOA_GetLatestTracks(doa_tracks);

	  BF_Latch_UpdateFromDoaTracks(doa_tracks);

//	  SoundMotorController_Update();
	  SoundAndElevationMotor_UpdateFromFirstSound();

	  AEC_BF_Process();

//      HitecD131MWServo_SetAngle(20.0f);
//      HAL_Delay(1000);
//      HitecD131MWServo_SetAngle(0.0f);
//      HAL_Delay(1000);
//      HitecD131MWServo_SetAngle(-25.0f);
//      HAL_Delay(1000);
#endif

	  if(Systime.Flag_1000ms)
	  {
		  Systime.Flag_1000ms = 0;
//		  MX28AR_LED_Toggle();
		  HAL_GPIO_TogglePin(Green_LED_GPIO_Port, Green_LED_Pin);
	  }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_DIRECT_SMPS_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 50;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SAI1;
  PeriphClkInitStruct.PLL3.PLL3M = 8;
  PeriphClkInitStruct.PLL3.PLL3N = 28;
  PeriphClkInitStruct.PLL3.PLL3P = 55;
  PeriphClkInitStruct.PLL3.PLL3Q = 2;
  PeriphClkInitStruct.PLL3.PLL3R = 2;
  PeriphClkInitStruct.PLL3.PLL3RGE = RCC_PLL3VCIRANGE_3;
  PeriphClkInitStruct.PLL3.PLL3VCOSEL = RCC_PLL3VCOWIDE;
  PeriphClkInitStruct.PLL3.PLL3FRACN = 1006;
  PeriphClkInitStruct.Sai1ClockSelection = RCC_SAI1CLKSOURCE_PLL3;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_HSEM_FreeCallback(uint32_t SemMask)
{
	if (SemMask & __HAL_HSEM_SEMID_TO_MASK(HSEM_AUDIO_ID))
	{
		HAL_HSEM_ActivateNotification(__HAL_HSEM_SEMID_TO_MASK(HSEM_AUDIO_ID));
//		SCB_InvalidateDCache_by_Addr((uint32_t*)&g_shared_audio, sizeof(g_shared_audio));
		g_hop_ready_cnt++;
	}
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
