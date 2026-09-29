//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------
//-----------------------------------20260618_final---------------------------------------
//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------
#include "audio_aec_bf_pcm.h"
#include "main.h"
#include "acoustic_bf.h"
#include "acoustic_ec.h"
#include "usbd_cdc_if.h"
#include "usbd_def.h"
#include "SRP_PHAT.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define AUDIO_BF_PAIR0_MIC_A_SLOT        0U
#define AUDIO_BF_PAIR0_MIC_B_SLOT        1U

/* AEC front-end configuration. */
#define AUDIO_AEC_PRIMARY_CHANNELS       1U
#define AUDIO_AEC_REFERENCE_CHANNELS     1U
#define AUDIO_AEC_OUTPUT_CHANNELS        1U
#define AUDIO_AEC_TAIL_LENGTH            1024U
#define AUDIO_AEC_INTERNAL_MEM_BYTES     (56U * AUDIO_AEC_TAIL_LENGTH)
#define AUDIO_BF_INTERNAL_MEM_BYTES      (24U * 1024U)

#define BF_STREAM_MAGIC0                 ((uint8_t)'K')
#define BF_STREAM_MAGIC1                 ((uint8_t)'C')
#define BF_STREAM_SAMPLE_RATE            16000U
#define BF_STREAM_CHANNELS               AUDIO_BF_STREAM_CHANNELS

#define AUDIO_BF_UART1_DEBUG_DECIM       50U
#define AUDIO_BF_UART1_DEBUG_TIMEOUT_MS  10U

/* Place large R-board AEC memory in RAM_D2. Linker script must define this section. */
#define AUDIO_RAM_D2_BSS __attribute__((section(".audio_ram_d2_bss"), aligned(32), used))

typedef struct __attribute__((packed))
{
    uint8_t  magic[2];          /* "KC" */
    uint32_t seq;

    int16_t l_az_deg_x10;       /* L mic board azimuth   * 10 */
    int16_t l_el_deg_x10;       /* L mic board elevation * 10 */
    int16_t r_az_deg_x10;       /* R mic board azimuth   * 10 */
    int16_t r_el_deg_x10;       /* R mic board elevation * 10 */

    int16_t l_bf_dir_deg_x10;   /* L mic board BF direction    * 10 */
    int16_t r_bf_dir_deg_x10;   /* R mic board BF direction    * 10 */

    uint32_t payload_bytes;     /* PCM payload length in bytes */
} bf_stream_header_t;

extern UART_HandleTypeDef huart1;

int16_t USBOUT[AUDIO_HOP_SAMPLES_PER_CH * AUDIO_BF_STREAM_CHANNELS];

typedef struct
{
    AcousticBF_Handler_t handler;
    AcousticBF_Config_t  config;
    uint8_t internal_mem[AUDIO_BF_INTERNAL_MEM_BYTES] __attribute__((aligned(32)));

    int16_t aec_tdm4_out_1ms[AUDIO_BF_1MS_SAMPLES_PER_CH * AUDIO_BF_TDM_CHANNELS]
        __attribute__((aligned(32)));
    int16_t bf_in_2ch_1ms[AUDIO_BF_1MS_SAMPLES_PER_CH * AUDIO_BF_USE_CHANNELS]
        __attribute__((aligned(32)));
    int16_t bf_out_mono_1ms[AUDIO_BF_1MS_SAMPLES_PER_CH]
        __attribute__((aligned(32)));

    float target_az_deg;
    uint32_t target_valid;
    uint32_t target_mic_a_slot;
    uint32_t target_mic_b_slot;

    uint32_t selected_axis;
    uint32_t selected_front_mic;
    uint32_t selected_back_mic;
    uint32_t selected_pair_index;
    uint32_t selected_pair_directed_index;
    float selected_pair_axis_deg;
    float selected_pair_error_deg;
} AudioBF_StreamContext_t;

static AudioBF_StreamContext_t s_bf_stream[AUDIO_STREAM_COUNT] __attribute__((aligned(32)));

static int16_t s_aec_mic_mono_1ms[AUDIO_STREAM_COUNT][AUDIO_BF_TDM_CHANNELS][AUDIO_BF_1MS_SAMPLES_PER_CH]
    __attribute__((aligned(32)));
static int16_t s_aec_out_mono_1ms[AUDIO_STREAM_COUNT][AUDIO_BF_TDM_CHANNELS][AUDIO_BF_1MS_SAMPLES_PER_CH]
    __attribute__((aligned(32)));

static AcousticEC_Handler_t s_aec_handler[AUDIO_STREAM_COUNT][AUDIO_BF_TDM_CHANNELS];
static AcousticEC_Config_t  s_aec_config[AUDIO_STREAM_COUNT][AUDIO_BF_TDM_CHANNELS];

/*
 * L-board AEC memory stays in default RAM_D1.
 * R-board AEC memory is placed in RAM_D2.
 * R size: 4 mics x 64KB = 256KB.
 */
static uint8_t s_aec_internal_mem_l[AUDIO_BF_TDM_CHANNELS][AUDIO_AEC_INTERNAL_MEM_BYTES]
    __attribute__((aligned(32)));
static uint8_t s_aec_internal_mem_r[AUDIO_BF_TDM_CHANNELS][AUDIO_AEC_INTERNAL_MEM_BYTES]
    AUDIO_RAM_D2_BSS;

volatile uint32_t g_aec_init_stage = 0U;
volatile uint32_t g_aec_last_error = 0U;
volatile uint32_t g_aec_memory_size[AUDIO_AEC_INSTANCES] = {0U};
volatile uint32_t g_aec_memory_addr[AUDIO_AEC_INSTANCES] = {0U};

static int16_t s_aec_reference_hop[AUDIO_HOP_SAMPLES_PER_CH] __attribute__((aligned(32)));
static uint32_t s_aec_reference_frames = 0U;
static uint32_t s_aec_reference_valid = 0U;

volatile uint32_t g_aec_bypass_count = 0U;
volatile uint32_t g_aec_process_count = 0U;

volatile uint32_t g_bf_init_stage = 0U;
volatile uint32_t g_bf_last_error = 0U;
volatile uint32_t g_bf_memory_size_stream[AUDIO_STREAM_COUNT] = {0U};
volatile uint32_t g_bf_memory_addr_stream[AUDIO_STREAM_COUNT] = {0U};

volatile uint32_t g_bf12_memory_size = 0U;
volatile uint32_t g_bf34_memory_size = 0U;
volatile uint32_t g_bf12_memory_addr = 0U;
volatile uint32_t g_bf34_memory_addr = 0U;

volatile uint32_t g_dbg_bf_before_fs = 0U;
volatile uint32_t g_dbg_bf_before_m1 = 0U;
volatile uint32_t g_dbg_bf_before_m2 = 0U;
volatile uint32_t g_dbg_bf_before_out = 0U;
volatile uint32_t g_dbg_bf_before_type = 0U;
volatile uint32_t g_dbg_bf_before_mem = 0U;
volatile uint32_t g_dbg_bf_before_size = 0U;

volatile float g_bf_target_az_deg_stream[AUDIO_STREAM_COUNT] = {0.0f, 0.0f};
volatile uint32_t g_bf_target_valid_stream[AUDIO_STREAM_COUNT] = {0U, 0U};
volatile uint32_t g_bf_selected_axis_stream[AUDIO_STREAM_COUNT] = {0U, 0U};
volatile uint32_t g_bf_selected_front_mic_stream[AUDIO_STREAM_COUNT] = {0U, 0U};
volatile uint32_t g_bf_selected_back_mic_stream[AUDIO_STREAM_COUNT] = {1U, 1U};
volatile uint32_t g_bf_selected_pair_directed_index_stream[AUDIO_STREAM_COUNT] = {0U, 0U};
volatile float g_bf_selected_pair_axis_deg_stream[AUDIO_STREAM_COUNT] = {0.0f, 0.0f};
volatile float g_bf_selected_pair_error_deg_stream[AUDIO_STREAM_COUNT] = {0.0f, 0.0f};

/*
 * 0: BF beam bypass 상태
 * 1: AcousticBF beam 동작 상태
 */
volatile uint32_t g_bf_beam_enabled_stream[AUDIO_STREAM_COUNT] = {0U, 0U};

/* Legacy single-target debug symbols. Updated with the most recently set stream. */
volatile float g_bf_target_az_deg = 0.0f;
volatile uint32_t g_bf_target_valid = 0U;
volatile uint32_t g_bf_selected_axis = 0U;
volatile uint32_t g_bf_selected_front_mic = 0U;
volatile uint32_t g_bf_selected_back_mic = 1U;
volatile uint32_t g_bf_usb_busy_drop = 0U;
volatile uint32_t g_bf_usb_send_count = 0U;

/*
 * Common elevation hold values used by both the D131MW elevation motor path
 * and the USB packet header.
 *
 * The symbol names keep the previous "usb" prefix for debug/watch
 * compatibility, but these are no longer USB-only values.  main.c updates
 * them with the exact elevation value sent to the motor; BF_Send_to_PC() only
 * reads these values so PC output and motor command stay identical.
 */
volatile float g_bf_usb_last_el_deg_stream[AUDIO_STREAM_COUNT] = {0.0f, 0.0f};
volatile uint32_t g_bf_usb_last_el_valid_stream[AUDIO_STREAM_COUNT] = {0U, 0U};

volatile uint32_t g_bf_selected_pair_index = 0U;
volatile uint32_t g_bf_selected_pair_reversed = 0U;
volatile uint32_t g_bf_selected_pair_directed_index = 0U;
volatile float g_bf_selected_pair_axis_deg = 0.0f;
volatile float g_bf_selected_pair_error_deg = 0.0f;

volatile uint32_t g_bf_uart_debug_tx_count = 0U;
volatile uint32_t g_bf_uart_debug_drop_count = 0U;

static uint32_t AudioAEC_FlatIndex(uint32_t stream_id, uint32_t mic_slot)
{
    return (stream_id * AUDIO_BF_TDM_CHANNELS) + mic_slot;
}

static uint8_t *AudioAEC_GetInternalMem(uint32_t stream_id, uint32_t mic_slot)
{
    if ((stream_id == AUDIO_STREAM_R) && (mic_slot < AUDIO_BF_TDM_CHANNELS))
    {
        return s_aec_internal_mem_r[mic_slot];
    }

    return s_aec_internal_mem_l[mic_slot];
}

static float AudioBF_WrapDeg360(float x)
{
    while (x < 0.0f) { x += 360.0f; }
    while (x >= 360.0f) { x -= 360.0f; }
    return x;
}

static float AudioBF_AngDiffDeg(float a, float b)
{
    float d = a - b;
    while (d > 180.0f) { d -= 360.0f; }
    while (d < -180.0f) { d += 360.0f; }
    return (d < 0.0f) ? -d : d;
}

#define AUDIO_BF_DIRECTED_PAIR_COUNT    12U

typedef struct
{
    uint32_t bf_m1_slot;
    uint32_t bf_m2_slot;
    float axis_deg;
} AudioBF_DirectedPair_t;

/*
 * SRP-PHAT azimuth convention:
 *              90 deg
 *        MIC2          MIC4
 * 0 deg        center        180 deg
 *        MIC1          MIC3
 *             270 deg
 */
static const AudioBF_DirectedPair_t s_bf_directed_pair_table[AUDIO_BF_DIRECTED_PAIR_COUNT] =
{
	{0U, 1U,  90.0f},		//0			ok
	{1U, 0U, 315.0f},		//1			ok
	{0U, 2U,   0.0f}, 		//2			ok
	{2U, 0U, 225.0f},		//3			ok
	{0U, 3U, 315.0f}, 		//4
	{3U, 0U, 135.0f},		//5
	{1U, 2U,  45.0f}, 		//6
	{2U, 1U, 225.0f},		//7
	{1U, 3U,  45.0f}, 		//8			ok
	{3U, 1U, 180.0f},		//9			ok
	{2U, 3U, 135.0f}, 		//10		ok
	{3U, 2U, 270.0f}		//11		ok
};

static uint32_t AudioBF_SelectDirectedPairIndexFromAz(float az_deg,
                                                       float *out_axis_deg,
                                                       float *out_error_deg)
{
    float az = AudioBF_WrapDeg360(az_deg);
    uint32_t best_pair = 0U;

    if (az > 0.0 && az <= 45.0f)       		{ best_pair = 8U;  } /* BF_M1=MIC2, BF_M2=MIC4, axis   0 deg */
    else if (az > 45.0 && az <= 90.0f)  	{ best_pair = 0U;  } /* BF_M1=MIC1, BF_M2=MIC2, axis  90 deg */
    else if (az > 90.0 && az <= 135.0f) 	{ best_pair = 10U; } /* BF_M1=MIC3, BF_M2=MIC4, axis  90 deg */
    else if (az > 135.0 &&az <= 180.0f) 	{ best_pair = 9U;  } /* BF_M1=MIC4, BF_M2=MIC2, axis 180 deg */
    else if (az > 180.0 && az <= 225.0f) 	{ best_pair = 3U;  } /* BF_M1=MIC3, BF_M2=MIC1, axis 180 deg */
    else if (az > 225.0 && az <= 270.0f) 	{ best_pair = 11U; } /* BF_M1=MIC4, BF_M2=MIC3, axis 270 deg */
    else if (az > 270.0 && az <= 315.0f) 	{ best_pair = 1U;  } /* BF_M1=MIC2, BF_M2=MIC1, axis 270 deg */
    else                   					{ best_pair = 2U;  } /* BF_M1=MIC1, BF_M2=MIC3, axis   0 deg */

    float best_axis = AudioBF_WrapDeg360(s_bf_directed_pair_table[best_pair].axis_deg);
    float best_error = AudioBF_AngDiffDeg(az, best_axis);

    if (out_axis_deg != NULL) { *out_axis_deg = best_axis; }
    if (out_error_deg != NULL) { *out_error_deg = best_error; }
    return best_pair;
}

static void AudioBF_SelectPairFromAz(float az_deg,
                                     uint32_t *front_mic,
                                     uint32_t *back_mic,
                                     uint32_t *axis,
                                     float *axis_deg,
                                     float *error_deg)
{
    float best_axis = 0.0f;
    float best_error = 0.0f;
    uint32_t best_pair = AudioBF_SelectDirectedPairIndexFromAz(az_deg, &best_axis, &best_error);
    const AudioBF_DirectedPair_t *pair = &s_bf_directed_pair_table[best_pair];

    /* Method 2: table order is the exact AcousticBF input order. */
    *front_mic = pair->bf_m1_slot;
    *back_mic  = pair->bf_m2_slot;
    *axis = best_pair;

    if (axis_deg != NULL) { *axis_deg = best_axis; }
    if (error_deg != NULL) { *error_deg = best_error; }
}

static uint32_t AudioBF_DegToTenths(float deg)
{
    float wrapped = AudioBF_WrapDeg360(deg);
    uint32_t tenth = (uint32_t)((wrapped * 10.0f) + 0.5f);
    if (tenth >= 3600U) { tenth -= 3600U; }
    return tenth;
}

static uint32_t AudioBF_ErrorDegToTenths(float deg)
{
    if (deg < 0.0f) { deg = -deg; }
    return (uint32_t)((deg * 10.0f) + 0.5f);
}

static int16_t AudioBF_DegToX10(float deg, uint32_t wrap360)
{
    float x = deg;

    if (wrap360 != 0U)
    {
        x = AudioBF_WrapDeg360(x);
    }

    if (x >= 0.0f)
    {
        return (int16_t)((x * 10.0f) + 0.5f);
    }

    return (int16_t)((x * 10.0f) - 0.5f);
}

static uint32_t s_bf_uart_debug_decim_count = 0U;

static void AudioBF_UART1_DebugPrint(uint32_t stream_id)
{
    char msg[192];
    int n;

    if (stream_id >= AUDIO_STREAM_COUNT) { return; }

    s_bf_uart_debug_decim_count++;
    if (s_bf_uart_debug_decim_count < AUDIO_BF_UART1_DEBUG_DECIM) { return; }
    s_bf_uart_debug_decim_count = 0U;

    const uint32_t az10   = AudioBF_DegToTenths(g_bf_target_az_deg_stream[stream_id]);
    const uint32_t axis10 = AudioBF_DegToTenths(g_bf_selected_pair_axis_deg_stream[stream_id]);
    const uint32_t err10  = AudioBF_ErrorDegToTenths(g_bf_selected_pair_error_deg_stream[stream_id]);

    n = snprintf(msg,
                 sizeof(msg),
                 "BF[%lu] SRP=%lu.%01lu deg, pair=%lu, axis=%lu.%01lu deg, err=%lu.%01lu deg, BF_M1=MIC%lu, BF_M2=MIC%lu, valid=%lu\r\n",
                 (unsigned long)stream_id,
                 (unsigned long)(az10 / 10U),
                 (unsigned long)(az10 % 10U),
                 (unsigned long)g_bf_selected_pair_directed_index_stream[stream_id],
                 (unsigned long)(axis10 / 10U),
                 (unsigned long)(axis10 % 10U),
                 (unsigned long)(err10 / 10U),
                 (unsigned long)(err10 % 10U),
                 (unsigned long)(g_bf_selected_front_mic_stream[stream_id] + 1U),
                 (unsigned long)(g_bf_selected_back_mic_stream[stream_id] + 1U),
                 (unsigned long)g_bf_target_valid_stream[stream_id]);

    if ((n > 0) && (n < (int)sizeof(msg)))
    {
        if (HAL_UART_Transmit(&huart1, (uint8_t *)msg, (uint16_t)n,
                              AUDIO_BF_UART1_DEBUG_TIMEOUT_MS) == HAL_OK)
        {
            g_bf_uart_debug_tx_count++;
        }
        else
        {
            g_bf_uart_debug_drop_count++;
        }
    }
}

static void AudioBF_UpdateLegacyDebugFromStream(uint32_t stream_id)
{
    if (stream_id >= AUDIO_STREAM_COUNT) { return; }

    g_bf_target_az_deg = g_bf_target_az_deg_stream[stream_id];
    g_bf_target_valid = g_bf_target_valid_stream[stream_id];
    g_bf_selected_axis = g_bf_selected_axis_stream[stream_id];
    g_bf_selected_front_mic = g_bf_selected_front_mic_stream[stream_id];
    g_bf_selected_back_mic = g_bf_selected_back_mic_stream[stream_id];
    g_bf_selected_pair_index = g_bf_selected_pair_directed_index_stream[stream_id];
    g_bf_selected_pair_directed_index = g_bf_selected_pair_directed_index_stream[stream_id];
    g_bf_selected_pair_reversed = 0U;
    g_bf_selected_pair_axis_deg = g_bf_selected_pair_axis_deg_stream[stream_id];
    g_bf_selected_pair_error_deg = g_bf_selected_pair_error_deg_stream[stream_id];
}

static void AudioBF_UpdateTargetPairFromAz(uint32_t stream_id, float az_deg)
{
    AudioBF_StreamContext_t *ctx;
    uint32_t front_mic = AUDIO_BF_PAIR0_MIC_A_SLOT;
    uint32_t back_mic = AUDIO_BF_PAIR0_MIC_B_SLOT;
    uint32_t axis = 0U;
    float axis_deg = 0.0f;
    float error_deg = 0.0f;

    if (stream_id >= AUDIO_STREAM_COUNT) { return; }

    ctx = &s_bf_stream[stream_id];
    AudioBF_SelectPairFromAz(az_deg, &front_mic, &back_mic, &axis, &axis_deg, &error_deg);

    ctx->target_mic_a_slot = front_mic;
    ctx->target_mic_b_slot = back_mic;
    ctx->selected_axis = axis;
    ctx->selected_front_mic = front_mic;
    ctx->selected_back_mic = back_mic;
    ctx->selected_pair_index = axis;
    ctx->selected_pair_directed_index = axis;
    ctx->selected_pair_axis_deg = axis_deg;
    ctx->selected_pair_error_deg = error_deg;

    g_bf_selected_axis_stream[stream_id] = axis;
    g_bf_selected_front_mic_stream[stream_id] = front_mic;
    g_bf_selected_back_mic_stream[stream_id] = back_mic;
    g_bf_selected_pair_directed_index_stream[stream_id] = axis;
    g_bf_selected_pair_axis_deg_stream[stream_id] = axis_deg;
    g_bf_selected_pair_error_deg_stream[stream_id] = error_deg;

    AudioBF_UpdateLegacyDebugFromStream(stream_id);
}

void AudioBF_SetTargetAzDegForStream(uint32_t stream_id, float az_deg)
{
    if (stream_id >= AUDIO_STREAM_COUNT) { return; }

    s_bf_stream[stream_id].target_az_deg = AudioBF_WrapDeg360(az_deg);
    s_bf_stream[stream_id].target_valid = 1U;

    g_bf_target_az_deg_stream[stream_id] = s_bf_stream[stream_id].target_az_deg;
    g_bf_target_valid_stream[stream_id] = 1U;
    g_bf_beam_enabled_stream[stream_id] = 1U;

    AudioBF_UpdateTargetPairFromAz(stream_id, s_bf_stream[stream_id].target_az_deg);
}

void AudioBF_ClearTargetForStream(uint32_t stream_id)
{
    if (stream_id >= AUDIO_STREAM_COUNT) { return; }

    /*
     * target_valid = 0 이면 AudioProcessForStream()에서 AcousticBF를 bypass한다.
     * 즉, BF 방향 lock 해제 + BF 빔 해제 상태가 된다.
     */
    s_bf_stream[stream_id].target_valid = 0U;

    g_bf_target_valid_stream[stream_id] = 0U;
    g_bf_beam_enabled_stream[stream_id] = 0U;

    AudioBF_UpdateLegacyDebugFromStream(stream_id);
}

void AudioBF_SetTargetAzDeg(float az_deg)
{
    AudioBF_SetTargetAzDegForStream(AUDIO_STREAM_L, az_deg);
}

void AudioBF_ClearTarget(void)
{
    AudioBF_ClearTargetForStream(AUDIO_STREAM_L);
}

static void AudioBF_Copy_TDM4CH_U16_To_S16_1ms(const uint16_t *src_tdm4,
                                               int16_t *dst_tdm4_s16)
{
    for (uint32_t i = 0U; i < AUDIO_BF_1MS_SAMPLES_PER_CH; i++)
    {
        for (uint32_t ch = 0U; ch < AUDIO_BF_TDM_CHANNELS; ch++)
        {
            dst_tdm4_s16[(i * AUDIO_BF_TDM_CHANNELS) + ch] =
                (int16_t)src_tdm4[(i * AUDIO_BF_TDM_CHANNELS) + ch];
        }
    }
}

static void AudioBF_Pack_TDM4CH_Pair_To_2CH_1ms(const int16_t *src_tdm4_s16,
                                                int16_t *dst_2ch,
                                                uint32_t mic_a_slot,
                                                uint32_t mic_b_slot)
{
    if (mic_a_slot >= AUDIO_BF_TDM_CHANNELS) { mic_a_slot = AUDIO_BF_PAIR0_MIC_A_SLOT; }
    if (mic_b_slot >= AUDIO_BF_TDM_CHANNELS) { mic_b_slot = AUDIO_BF_PAIR0_MIC_B_SLOT; }

    for (uint32_t i = 0U; i < AUDIO_BF_1MS_SAMPLES_PER_CH; i++)
    {
        dst_2ch[(i * AUDIO_BF_USE_CHANNELS) + 0U] =
            src_tdm4_s16[(i * AUDIO_BF_TDM_CHANNELS) + mic_a_slot];
        dst_2ch[(i * AUDIO_BF_USE_CHANNELS) + 1U] =
            src_tdm4_s16[(i * AUDIO_BF_TDM_CHANNELS) + mic_b_slot];
    }
}

static void AudioBF_Bypass_TDM4CH_Average_To_Mono_1ms(const int16_t *src_tdm4_s16,
                                                       int16_t *dst_mono)
{
    for (uint32_t i = 0U; i < AUDIO_BF_1MS_SAMPLES_PER_CH; i++)
    {
        int32_t sum = 0;

        sum += src_tdm4_s16[(i * AUDIO_BF_TDM_CHANNELS) + 0U];
        sum += src_tdm4_s16[(i * AUDIO_BF_TDM_CHANNELS) + 1U];
        sum += src_tdm4_s16[(i * AUDIO_BF_TDM_CHANNELS) + 2U];
        sum += src_tdm4_s16[(i * AUDIO_BF_TDM_CHANNELS) + 3U];

        dst_mono[i] = (int16_t)(sum / 4);
    }
}

static void AudioAEC_InitOne(uint32_t stream_id, uint32_t mic_slot)
{
    uint32_t error_value = 0U;
    uint32_t flat = AudioAEC_FlatIndex(stream_id, mic_slot);
    AcousticEC_Handler_t *handler = &s_aec_handler[stream_id][mic_slot];
    AcousticEC_Config_t  *config  = &s_aec_config[stream_id][mic_slot];
    uint8_t *internal_mem = AudioAEC_GetInternalMem(stream_id, mic_slot);

    memset(handler, 0, sizeof(*handler));
    memset(config, 0, sizeof(*config));

    handler->tail_length            = AUDIO_AEC_TAIL_LENGTH;
    handler->preprocess_init        = 0U;
    handler->ptr_primary_channels   = AUDIO_AEC_PRIMARY_CHANNELS;
    handler->ptr_reference_channels = AUDIO_AEC_REFERENCE_CHANNELS;
    handler->ptr_output_channels    = AUDIO_AEC_OUTPUT_CHANNELS;

    g_aec_init_stage = 10U + flat;
    error_value = AcousticEC_getMemorySize(handler);
    g_aec_last_error = error_value;
    g_aec_memory_size[flat] = handler->internal_memory_size;
    if (error_value != 0U) { Error_Handler(); }

    g_aec_init_stage = 20U + flat;
    if (handler->internal_memory_size > AUDIO_AEC_INTERNAL_MEM_BYTES)
    {
        g_aec_last_error = handler->internal_memory_size;
        Error_Handler();
    }

    /* .audio_ram_d2_bss is NOLOAD, so clear explicitly. */
    memset(internal_mem, 0, AUDIO_AEC_INTERNAL_MEM_BYTES);
    handler->pInternalMemory = (uint32_t *)internal_mem;
    g_aec_memory_addr[flat] = (uint32_t)handler->pInternalMemory;

    if ((((uint32_t)handler->pInternalMemory) & 0x1FU) != 0U)
    {
        g_aec_last_error = 0xEEEE0002U;
        Error_Handler();
    }

    g_aec_init_stage = 30U + flat;
    error_value = AcousticEC_Init(handler);
    g_aec_last_error = error_value;
    if (error_value != 0U) { Error_Handler(); }

    config->preprocess_state        = ACOUSTIC_EC_PREPROCESS_DISABLE;
    config->AGC_value               = 0U;
    config->noise_suppress_default  = 0;
    config->echo_suppress_default   = 0;
    config->echo_suppress_active    = 0;
    config->residual_echo_remove    = 0U;

    g_aec_init_stage = 40U + flat;
    error_value = AcousticEC_setConfig(handler, config);
    g_aec_last_error = error_value;
    if (error_value != 0U) { Error_Handler(); }
}

static void AudioAEC_Init(void)
{
    for (uint32_t stream = 0U; stream < AUDIO_STREAM_COUNT; stream++)
    {
        for (uint32_t mic = 0U; mic < AUDIO_BF_TDM_CHANNELS; mic++)
        {
            AudioAEC_InitOne(stream, mic);
        }
    }

    g_aec_init_stage = 100U;
    g_aec_last_error = 0U;
    memset(s_aec_reference_hop, 0, sizeof(s_aec_reference_hop));
    s_aec_reference_frames = 0U;
    s_aec_reference_valid = 0U;
}

static const int16_t *AudioAEC_GetReference1ms(uint32_t frame_offset)
{
    if ((s_aec_reference_valid != 0U) &&
        (s_aec_reference_frames >= (frame_offset + AUDIO_BF_1MS_SAMPLES_PER_CH)) &&
        (s_aec_reference_frames <= AUDIO_HOP_SAMPLES_PER_CH))
    {
        return &s_aec_reference_hop[frame_offset];
    }
    return NULL;
}

static void AudioAEC_Process_TDM4CH_1ms(uint32_t stream_id,
                                        const uint16_t *mic_tdm4,
                                        int16_t *aec_out_tdm4,
                                        uint32_t frame_offset)
{
    const int16_t *reference_1ms = AudioAEC_GetReference1ms(frame_offset);

    if ((stream_id >= AUDIO_STREAM_COUNT) || (mic_tdm4 == NULL) || (aec_out_tdm4 == NULL))
    {
        return;
    }

    if (reference_1ms == NULL)
    {
        AudioBF_Copy_TDM4CH_U16_To_S16_1ms(mic_tdm4, aec_out_tdm4);
        g_aec_bypass_count++;
        return;
    }

    for (uint32_t i = 0U; i < AUDIO_BF_1MS_SAMPLES_PER_CH; i++)
    {
        for (uint32_t mic = 0U; mic < AUDIO_BF_TDM_CHANNELS; mic++)
        {
            s_aec_mic_mono_1ms[stream_id][mic][i] =
                (int16_t)mic_tdm4[(i * AUDIO_BF_TDM_CHANNELS) + mic];
        }
    }

    for (uint32_t mic = 0U; mic < AUDIO_BF_TDM_CHANNELS; mic++)
    {
        uint32_t ready = AcousticEC_Data_Input((void *)s_aec_mic_mono_1ms[stream_id][mic],
                                               (void *)reference_1ms,
                                               (void *)s_aec_out_mono_1ms[stream_id][mic],
                                               &s_aec_handler[stream_id][mic]);
        if (ready != 0U)
        {
            (void)AcousticEC_Process(&s_aec_handler[stream_id][mic]);
            g_aec_process_count++;
        }
    }

    for (uint32_t i = 0U; i < AUDIO_BF_1MS_SAMPLES_PER_CH; i++)
    {
        for (uint32_t mic = 0U; mic < AUDIO_BF_TDM_CHANNELS; mic++)
        {
            aec_out_tdm4[(i * AUDIO_BF_TDM_CHANNELS) + mic] =
                s_aec_out_mono_1ms[stream_id][mic][i];
        }
    }
}

void AudioAEC_SetReferenceFrame(const int16_t *reference_mono,
                                uint32_t frames_per_ch)
{
    uint32_t frames_to_copy;

    if ((reference_mono == NULL) || (frames_per_ch == 0U))
    {
        AudioAEC_ClearReferenceFrame();
        return;
    }

    frames_to_copy = frames_per_ch;
    if (frames_to_copy > AUDIO_HOP_SAMPLES_PER_CH)
    {
        frames_to_copy = AUDIO_HOP_SAMPLES_PER_CH;
    }

    memcpy(s_aec_reference_hop, reference_mono, sizeof(int16_t) * frames_to_copy);
    if (frames_to_copy < AUDIO_HOP_SAMPLES_PER_CH)
    {
        memset(&s_aec_reference_hop[frames_to_copy], 0,
               sizeof(int16_t) * (AUDIO_HOP_SAMPLES_PER_CH - frames_to_copy));
    }

    s_aec_reference_frames = frames_to_copy;
    s_aec_reference_valid = (frames_to_copy >= AUDIO_BF_1MS_SAMPLES_PER_CH) ? 1U : 0U;
}

void AudioAEC_ClearReferenceFrame(void)
{
    memset(s_aec_reference_hop, 0, sizeof(s_aec_reference_hop));
    s_aec_reference_frames = 0U;
    s_aec_reference_valid = 0U;
}

static void AudioBF_InitOne(uint32_t stream_id)
{
    uint32_t error_value = 0U;
    AudioBF_StreamContext_t *ctx;

    if (stream_id >= AUDIO_STREAM_COUNT) { return; }
    ctx = &s_bf_stream[stream_id];

    memset(&ctx->handler, 0, sizeof(ctx->handler));
    memset(&ctx->config, 0, sizeof(ctx->config));
    memset(ctx->internal_mem, 0, AUDIO_BF_INTERNAL_MEM_BYTES);

    ctx->target_az_deg = 0.0f;
    ctx->target_valid = 0U;
    ctx->target_mic_a_slot = AUDIO_BF_PAIR0_MIC_A_SLOT;
    ctx->target_mic_b_slot = AUDIO_BF_PAIR0_MIC_B_SLOT;

    ctx->handler.algorithm_type_init = ACOUSTIC_BF_TYPE_CARDIOID_DENOISE;
    ctx->handler.ref_mic_enable   = ACOUSTIC_BF_REF_DISABLE;
    ctx->handler.ptr_out_channels = 1U;
    ctx->handler.data_format        = ACOUSTIC_BF_DATA_FORMAT_PCM;
    ctx->handler.sampling_frequency = ACOUSTIC_BF_FS_16;
    ctx->handler.ptr_M1_channels = AUDIO_BF_USE_CHANNELS;
    ctx->handler.ptr_M2_channels = AUDIO_BF_USE_CHANNELS;
    ctx->handler.delay_enable = ACOUSTIC_BF_CARDOID_DELAY_ENABLE;
    ctx->handler.mixer_enable = ACOUSTIC_BF_MIXER_DISABLE;

    g_bf_init_stage = 10U + stream_id;
    error_value = AcousticBF_getMemorySize(&ctx->handler);
    g_bf_last_error = error_value;

    g_bf_memory_size_stream[stream_id] = ctx->handler.internal_memory_size;
    g_bf_memory_addr_stream[stream_id] = (uint32_t)ctx->internal_mem;

    if (stream_id == AUDIO_STREAM_L)
    {
        g_bf12_memory_size = ctx->handler.internal_memory_size;
        g_bf12_memory_addr = (uint32_t)ctx->internal_mem;
    }
    else
    {
        g_bf34_memory_size = ctx->handler.internal_memory_size;
        g_bf34_memory_addr = (uint32_t)ctx->internal_mem;
    }

    if (error_value != 0U) { Error_Handler(); }

    g_bf_init_stage = 20U + stream_id;
    if (ctx->handler.internal_memory_size > AUDIO_BF_INTERNAL_MEM_BYTES)
    {
        g_bf_last_error = ctx->handler.internal_memory_size;
        Error_Handler();
    }

    ctx->handler.pInternalMemory = (uint32_t *)ctx->internal_mem;
    if ((((uint32_t)ctx->handler.pInternalMemory) & 0x1FU) != 0U)
    {
        g_bf_last_error = 0xEEEE0001U;
        Error_Handler();
    }

    g_dbg_bf_before_fs   = ctx->handler.sampling_frequency;
    g_dbg_bf_before_m1   = ctx->handler.ptr_M1_channels;
    g_dbg_bf_before_m2   = ctx->handler.ptr_M2_channels;
    g_dbg_bf_before_out  = ctx->handler.ptr_out_channels;
    g_dbg_bf_before_type = ctx->handler.algorithm_type_init;
    g_dbg_bf_before_mem  = (uint32_t)ctx->handler.pInternalMemory;
    g_dbg_bf_before_size = ctx->handler.internal_memory_size;

    g_bf_init_stage = 30U + stream_id;
    error_value = AcousticBF_Init(&ctx->handler);
    g_bf_last_error = error_value;
    if (error_value != 0U) { Error_Handler(); }

    ctx->config.algorithm_type = ACOUSTIC_BF_TYPE_CARDIOID_DENOISE;
    ctx->config.M2_gain        = 0.0f;
    ctx->config.mic_distance   = ACOUSTIC_BF_MIC_DISTANCE;
    ctx->config.volume         = 0;

    g_bf_init_stage = 40U + stream_id;
    error_value = AcousticBF_setConfig(&ctx->handler, &ctx->config);
    g_bf_last_error = error_value;
    if (error_value != 0U) { Error_Handler(); }

    AudioBF_UpdateTargetPairFromAz(stream_id, 0.0f);
    g_bf_init_stage = 100U + stream_id;
}

uint32_t AudioProcessForStream(uint32_t stream_id,
                               const uint16_t *tdm4ch,
                               uint32_t frames_per_ch,
                               int16_t *out_mono)
{
    uint32_t produced_frames = 0U;
    AudioBF_StreamContext_t *ctx;

    if ((stream_id >= AUDIO_STREAM_COUNT) || (tdm4ch == NULL) || (out_mono == NULL))
    {
        return 0U;
    }

    ctx = &s_bf_stream[stream_id];

    if (ctx->target_valid != 0U)
    {
        AudioBF_UpdateTargetPairFromAz(stream_id, ctx->target_az_deg);
    }

    /* Uncomment when debugging steering. */
    /* AudioBF_UART1_DebugPrint(stream_id); */

    for (uint32_t frame = 0U;
         frame + AUDIO_BF_1MS_SAMPLES_PER_CH <= frames_per_ch;
         frame += AUDIO_BF_1MS_SAMPLES_PER_CH)
    {
        const uint16_t *src_1ms = &tdm4ch[frame * AUDIO_BF_TDM_CHANNELS];

        AudioAEC_Process_TDM4CH_1ms(stream_id,
                                    src_1ms,
                                    ctx->aec_tdm4_out_1ms,
                                    frame);

        if (ctx->target_valid == 0U)
        {
            /*
             * BF beam release 상태.
             * AcousticBF를 돌리지 않고 AEC 이후 4ch 평균을 mono로 출력한다.
             */
            AudioBF_Bypass_TDM4CH_Average_To_Mono_1ms(ctx->aec_tdm4_out_1ms,
                                                       ctx->bf_out_mono_1ms);

            for (uint32_t i = 0U; i < AUDIO_BF_1MS_SAMPLES_PER_CH; i++)
            {
                out_mono[produced_frames + i] = ctx->bf_out_mono_1ms[i];
            }

            produced_frames += AUDIO_BF_1MS_SAMPLES_PER_CH;
        }
        else
        {
            /*
             * BF beam enabled 상태.
             */
            AudioBF_Pack_TDM4CH_Pair_To_2CH_1ms(ctx->aec_tdm4_out_1ms,
                                                ctx->bf_in_2ch_1ms,
                                                ctx->target_mic_a_slot,
                                                ctx->target_mic_b_slot);

            uint32_t ready = AcousticBF_FirstStep((void *)&ctx->bf_in_2ch_1ms[0U],
                                                  (void *)&ctx->bf_in_2ch_1ms[1U],
                                                  (void *)ctx->bf_out_mono_1ms,
                                                  &ctx->handler);

            for (uint32_t i = 0U; i < AUDIO_BF_1MS_SAMPLES_PER_CH; i++)
            {
                out_mono[produced_frames + i] = ctx->bf_out_mono_1ms[i];
            }

            produced_frames += AUDIO_BF_1MS_SAMPLES_PER_CH;

            if (ready == 1U)
            {
                (void)AcousticBF_SecondStep(&ctx->handler);
            }
        }
    }

    return produced_frames;
}

/* Legacy wrapper: process L stream and duplicate into USBOUT. */
uint32_t AudioProcess(const uint16_t *tdm4ch, uint32_t frames_per_ch)
{
    static int16_t legacy_out[AUDIO_HOP_SAMPLES_PER_CH] __attribute__((aligned(32)));
    uint32_t frames = AudioProcessForStream(AUDIO_STREAM_L, tdm4ch, frames_per_ch, legacy_out);

    for (uint32_t i = 0U; i < frames; i++)
    {
        USBOUT[(i * AUDIO_BF_STREAM_CHANNELS) + AUDIO_STREAM_L] = legacy_out[i];
        USBOUT[(i * AUDIO_BF_STREAM_CHANNELS) + AUDIO_STREAM_R] = legacy_out[i];
    }

    return frames;
}

void Audio_Libraries_Init(void)
{
    __HAL_RCC_CRC_CLK_ENABLE();

    AudioAEC_Init();
    AudioBF_InitOne(AUDIO_STREAM_L);
    AudioBF_InitOne(AUDIO_STREAM_R);
}

void SW_Task1_Callback(void)
{
    (void)AcousticBF_SecondStep(&s_bf_stream[AUDIO_STREAM_L].handler);
    (void)AcousticBF_SecondStep(&s_bf_stream[AUDIO_STREAM_R].handler);
}

void SW_Task1_Start(void)
{
    HAL_NVIC_SetPendingIRQ(EXTI1_IRQn);
}

static uint32_t g_bf_stream_seq = 0U;

#define BF_TX_PACKET_BYTES  (sizeof(bf_stream_header_t) + \
                             (AUDIO_HOP_SAMPLES_PER_CH * AUDIO_BF_STREAM_CHANNELS * sizeof(int16_t)))

static uint8_t s_bf_tx_packet[2U][BF_TX_PACKET_BYTES] __attribute__((aligned(32)));
static uint32_t s_bf_tx_packet_index = 0U;

static void BF_CleanDCacheRange(const void *addr, uint32_t size)
{
    uintptr_t start = ((uintptr_t)addr) & ~(uintptr_t)31U;
    uintptr_t end   = (((uintptr_t)addr) + size + 31U) & ~(uintptr_t)31U;

    SCB_CleanDCache_by_Addr((uint32_t *)start,
                            (int32_t)(end - start));
}

void AudioBF_SetLastElevationForStream(uint32_t stream_id, float el_deg)
{
    if (stream_id >= AUDIO_STREAM_COUNT)
    {
        return;
    }

    g_bf_usb_last_el_deg_stream[stream_id] = el_deg;
    g_bf_usb_last_el_valid_stream[stream_id] = 1U;
}

float AudioBF_GetLastElevationForStream(uint32_t stream_id)
{
    if (stream_id >= AUDIO_STREAM_COUNT)
    {
        return 0.0f;
    }

    if (g_bf_usb_last_el_valid_stream[stream_id] != 0U)
    {
        return g_bf_usb_last_el_deg_stream[stream_id];
    }

    return 0.0f;
}

uint8_t AudioBF_GetLastElevationValidForStream(uint32_t stream_id)
{
    if (stream_id >= AUDIO_STREAM_COUNT)
    {
        return 0U;
    }

    return (g_bf_usb_last_el_valid_stream[stream_id] != 0U) ? 1U : 0U;
}

static void BF_Send_to_PC(const int16_t *pcm, uint32_t frames, uint32_t channels)
{
    bf_stream_header_t hdr;
    uint32_t payload_bytes;
    uint32_t total_bytes;
    doa_result_t doa_tracks[DOA_MAX_SOURCES];

    if ((pcm == NULL) || (frames == 0U) || (channels == 0U)) { return; }

    payload_bytes = frames * channels * sizeof(int16_t);
    total_bytes = sizeof(bf_stream_header_t) + payload_bytes;

    if (total_bytes > BF_TX_PACKET_BYTES)
    {
        g_bf_usb_busy_drop++;
        return;
    }

    memset(&hdr, 0, sizeof(hdr));
    memset(doa_tracks, 0, sizeof(doa_tracks));

    hdr.magic[0]      = BF_STREAM_MAGIC0;
    hdr.magic[1]      = BF_STREAM_MAGIC1;
    hdr.seq           = g_bf_stream_seq++;
    hdr.payload_bytes = payload_bytes;

    /*
     * USB header angle fields are transferred as degree x 10.
     * Example: 48.3 deg -> 483.
     *
     * DOA fields are the latest detected source direction.
     * BF direction fields are the actual per-board AcousticBF steering
     * direction selected from the 12 directed mic-pair table.
     */
    hdr.l_bf_dir_deg_x10 = AudioBF_DegToX10(g_bf_selected_pair_axis_deg_stream[AUDIO_STREAM_L], 1U);
    hdr.r_bf_dir_deg_x10 = AudioBF_DegToX10(g_bf_selected_pair_axis_deg_stream[AUDIO_STREAM_R], 1U);

    /*
     * USB header angle fields:
     *   azimuth/elevation are transferred as degree x 10.
     *   Example: 48.3 deg -> 483.
     *
     * DOA_GetLatestTracks() provides:
     *   index 0 = AUDIO_STREAM_L
     *   index 1 = AUDIO_STREAM_R
     */
    {
        uint8_t latest_tracks_valid;
        float l_led_raw_az_deg = 0.0f;
        float r_led_raw_az_deg = 0.0f;
        float l_usb_el_deg = 0.0f;
        float r_usb_el_deg = 0.0f;
        uint8_t l_led_active = 0U;
        uint8_t r_led_active = 0U;

        latest_tracks_valid = DOA_GetLatestTracks(doa_tracks);

        /*
         * Make PC azimuth follow the same raw-angle display path as LED.
         * If the stream is inactive, DOA_SetLedForStream() keeps the previous
         * value, so the PC azimuth holds the last LED angle.
         */
        (void)DOA_GetLedRawAzimuthForStream(AUDIO_STREAM_L, &l_led_raw_az_deg, &l_led_active);
        (void)DOA_GetLedRawAzimuthForStream(AUDIO_STREAM_R, &r_led_raw_az_deg, &r_led_active);

        (void)l_led_active;
        (void)r_led_active;

        /*
         * Elevation uses the same held value that is sent to the D131MW motor.
         * Do not update it here from latest DOA tracks; main.c is the single
         * writer so USB elevation and motor elevation stay identical.
         */
        (void)latest_tracks_valid;
        l_usb_el_deg = AudioBF_GetLastElevationForStream(AUDIO_STREAM_L);
        r_usb_el_deg = AudioBF_GetLastElevationForStream(AUDIO_STREAM_R);

        hdr.l_az_deg_x10 = AudioBF_DegToX10(l_led_raw_az_deg, 1U);
        hdr.l_el_deg_x10 = AudioBF_DegToX10(l_usb_el_deg, 0U);
        hdr.r_az_deg_x10 = AudioBF_DegToX10(r_led_raw_az_deg, 1U);
        hdr.r_el_deg_x10 = AudioBF_DegToX10(r_usb_el_deg, 0U);
    }

    uint8_t *tx = s_bf_tx_packet[s_bf_tx_packet_index];

    memcpy(&tx[0], &hdr, sizeof(hdr));
    memcpy(&tx[sizeof(hdr)], pcm, payload_bytes);
    BF_CleanDCacheRange(tx, total_bytes);

    if (CDC_Transmit_FS(tx, (uint16_t)total_bytes) == USBD_OK)
    {
        g_bf_usb_send_count++;
        s_bf_tx_packet_index ^= 1U;
    }
    else
    {
        g_bf_usb_busy_drop++;
    }
}

static uint16_t s_cm7_bf_hop_tdm4[AUDIO_STREAM_COUNT][SHARED_HOP_WORDS]
    __attribute__((aligned(32)));
static uint32_t s_cm7_last_slot_seq[AUDIO_STREAM_COUNT][SHARED_SLOT_COUNT] = {{0U, 0U}, {0U, 0U}};
static uint32_t s_bf_next_slot[AUDIO_STREAM_COUNT] = {SHARED_SLOT_HALF, SHARED_SLOT_HALF};

static int16_t s_bf_stream_out_hop[AUDIO_STREAM_COUNT][AUDIO_HOP_SAMPLES_PER_CH]
    __attribute__((aligned(32)));
static uint32_t s_bf_stream_frames[AUDIO_STREAM_COUNT] = {0U, 0U};
static uint32_t s_bf_stream_valid[AUDIO_STREAM_COUNT] = {0U, 0U};

volatile uint32_t g_audio_half_ready = 0U;
volatile uint32_t g_audio_full_ready = 0U;
volatile uint32_t g_audio_shared_retry = 0U;
volatile uint32_t g_audio_shared_drop = 0U;
volatile uint32_t g_bf_out_samples = 0U;
volatile uint32_t g_aecbf_process_hops = 0U;

static void CM7_InvalidateDCacheRange(const void *addr, uint32_t size)
{
    uintptr_t start = ((uintptr_t)addr) & ~(uintptr_t)31U;
    uintptr_t end   = (((uintptr_t)addr) + size + 31U) & ~(uintptr_t)31U;

    SCB_InvalidateDCache_by_Addr((uint32_t *)start,
                                 (int32_t)(end - start));
}

static uint32_t AudioShared_GetActiveStreamMask(void)
{
    CM7_InvalidateDCacheRange((const void *)&g_shared_audio.active_stream_mask,
                               sizeof(g_shared_audio.active_stream_mask) + sizeof(g_shared_audio.config_seq));
    return g_shared_audio.active_stream_mask & AUDIO_STREAM_MASK_ALL;
}

static uint8_t AudioShared_IsStreamActive(uint32_t stream_id)
{
    if (stream_id >= AUDIO_STREAM_COUNT)
    {
        return 0U;
    }

    return AUDIO_STREAM_MASK_HAS(AudioShared_GetActiveStreamMask(), stream_id) ? 1U : 0U;
}

static uint32_t AudioShared_GetSlotOffset(uint32_t slot)
{
    return (slot == SHARED_SLOT_HALF) ? SHARED_HALF_OFFSET : SHARED_FULL_OFFSET;
}

static uint8_t AudioShared_CopySlotToLocal(uint32_t stream_id,
                                           uint32_t slot,
                                           uint16_t *dst)
{
    uint32_t offset;
    uint32_t guard_before;
    uint32_t guard_after;
    uint32_t seq_before;
    uint32_t seq_after;

    if ((stream_id >= AUDIO_STREAM_COUNT) ||
        (dst == NULL) ||
        (slot >= SHARED_SLOT_COUNT) ||
        (AudioShared_IsStreamActive(stream_id) == 0U))
    {
        return 0U;
    }

    offset = AudioShared_GetSlotOffset(slot);

    for (uint32_t retry = 0U; retry < 3U; retry++)
    {
        CM7_InvalidateDCacheRange(&g_shared_audio, sizeof(g_shared_audio));

        guard_before = g_shared_audio.stream[stream_id].slot_guard[slot];
        seq_before   = g_shared_audio.stream[stream_id].slot_seq[slot];

        if (seq_before == s_cm7_last_slot_seq[stream_id][slot])
        {
            return 0U;
        }

        if ((guard_before & 1U) != 0U)
        {
            g_audio_shared_retry++;
            continue;
        }

        memcpy(dst,
               (const void *)&g_shared_audio.stream[stream_id].samples[offset],
               sizeof(uint16_t) * SHARED_HOP_WORDS);

        __DMB();
        CM7_InvalidateDCacheRange(&g_shared_audio, sizeof(g_shared_audio));

        guard_after = g_shared_audio.stream[stream_id].slot_guard[slot];
        seq_after   = g_shared_audio.stream[stream_id].slot_seq[slot];

        if ((guard_before == guard_after) &&
            ((guard_after & 1U) == 0U) &&
            (seq_before == seq_after))
        {
            s_cm7_last_slot_seq[stream_id][slot] = seq_before;
            return 1U;
        }

        g_audio_shared_retry++;
    }

    g_audio_shared_drop++;
    return 0U;
}

static uint32_t AEC_BF_ProcessOneStream(uint32_t stream_id)
{
    uint32_t slot;
    uint32_t frames;

    if ((stream_id >= AUDIO_STREAM_COUNT) ||
        (AudioShared_IsStreamActive(stream_id) == 0U))
    {
        return 0U;
    }

    slot = s_bf_next_slot[stream_id];

    if (AudioShared_CopySlotToLocal(stream_id, slot, s_cm7_bf_hop_tdm4[stream_id]) == 0U)
    {
        slot = (slot == SHARED_SLOT_HALF) ? SHARED_SLOT_FULL : SHARED_SLOT_HALF;
        if (AudioShared_CopySlotToLocal(stream_id, slot, s_cm7_bf_hop_tdm4[stream_id]) == 0U)
        {
            return 0U;
        }
    }

    s_bf_next_slot[stream_id] = (slot == SHARED_SLOT_HALF) ? SHARED_SLOT_FULL : SHARED_SLOT_HALF;

    frames = AudioProcessForStream(stream_id,
                                   s_cm7_bf_hop_tdm4[stream_id],
                                   AUDIO_HOP_SAMPLES_PER_CH,
                                   s_bf_stream_out_hop[stream_id]);

    if (frames != 0U)
    {
        s_bf_stream_frames[stream_id] = frames;
        s_bf_stream_valid[stream_id] = 1U;
    }

    return frames;
}

static uint32_t AudioBF_BuildStereoUsbOut(uint32_t frames_hint)
{
    uint32_t frames = frames_hint;

    if (frames == 0U) { return 0U; }
    if (frames > AUDIO_HOP_SAMPLES_PER_CH) { frames = AUDIO_HOP_SAMPLES_PER_CH; }

    for (uint32_t i = 0U; i < frames; i++)
    {
        int16_t l = 0;
        int16_t r = 0;

        if ((s_bf_stream_valid[AUDIO_STREAM_L] != 0U) &&
            (i < s_bf_stream_frames[AUDIO_STREAM_L]))
        {
            l = s_bf_stream_out_hop[AUDIO_STREAM_L][i];
        }

        if ((s_bf_stream_valid[AUDIO_STREAM_R] != 0U) &&
            (i < s_bf_stream_frames[AUDIO_STREAM_R]))
        {
            r = s_bf_stream_out_hop[AUDIO_STREAM_R][i];
        }

        USBOUT[(i * AUDIO_BF_STREAM_CHANNELS) + AUDIO_STREAM_L] = l;
        USBOUT[(i * AUDIO_BF_STREAM_CHANNELS) + AUDIO_STREAM_R] = r;
    }

    return frames;
}

void AEC_BF_Process(void)
{
    uint32_t active_mask = AudioShared_GetActiveStreamMask();
    uint32_t frames_l = 0U;
    uint32_t frames_r = 0U;
    uint32_t frames;

    if (AUDIO_STREAM_MASK_HAS(active_mask, AUDIO_STREAM_L))
    {
        frames_l = AEC_BF_ProcessOneStream(AUDIO_STREAM_L);
    }
    else
    {
        s_bf_stream_valid[AUDIO_STREAM_L] = 0U;
        s_bf_stream_frames[AUDIO_STREAM_L] = 0U;
    }

    if (AUDIO_STREAM_MASK_HAS(active_mask, AUDIO_STREAM_R))
    {
        frames_r = AEC_BF_ProcessOneStream(AUDIO_STREAM_R);
    }
    else
    {
        s_bf_stream_valid[AUDIO_STREAM_R] = 0U;
        s_bf_stream_frames[AUDIO_STREAM_R] = 0U;
        memset(s_bf_stream_out_hop[AUDIO_STREAM_R], 0,
               sizeof(s_bf_stream_out_hop[AUDIO_STREAM_R]));
    }

    if ((frames_l == 0U) && (frames_r == 0U))
    {
        return;
    }

    frames = (frames_l != 0U) ? frames_l : frames_r;
    if ((frames_l != 0U) && (frames_r != 0U) && (frames_r < frames))
    {
        frames = frames_r;
    }

    /* Stereo packet format is preserved; inactive R is zero-filled. */
    g_bf_out_samples = AudioBF_BuildStereoUsbOut(frames);
    BF_Send_to_PC(USBOUT, g_bf_out_samples, BF_STREAM_CHANNELS);
}


