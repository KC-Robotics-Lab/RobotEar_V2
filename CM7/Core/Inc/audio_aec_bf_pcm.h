#ifndef INC_AUDIO_AEC_BF_PCM_H_
#define INC_AUDIO_AEC_BF_PCM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "shared_audio.h"

/*
 * AcousticBF microphone distance.
 * 210 is safer when cardioid delay is enabled.
 */
#define ACOUSTIC_BF_MIC_DISTANCE      210U

/* TDM input layout per board: [MIC1, MIC2, MIC3, MIC4] */
#define AUDIO_BF_TDM_CHANNELS         4U

/* Each AcousticBF instance receives one selected 2-mic pair. */
#define AUDIO_BF_USE_CHANNELS         2U

/* 16 kHz, 1 ms = 16 samples/channel. */
#define AUDIO_BF_1MS_SAMPLES_PER_CH   16U

/*
 * One BF per physical 4ch microphone board:
 *   stream 0 = L board -> USB ch0
 *   stream 1 = R board -> USB ch1
 */
#define AUDIO_BF_PAIR_COUNT           AUDIO_STREAM_COUNT
#define AUDIO_BF_STREAM_CHANNELS      AUDIO_STREAM_COUNT

/* Each AcousticBF instance outputs mono. */
#define AUDIO_BF_OUT_CHANNELS         1U

/*
 * One mono AEC instance per microphone per board:
 *   2 boards x 4 microphones = 8 instances.
 */
#define AUDIO_AEC_INSTANCES           (AUDIO_STREAM_COUNT * AUDIO_BF_TDM_CHANNELS)

/*
 * Uncomment this define if you want to configure and start acquisition
 * independently from USB functionalities.
 */
#define DISABLE_USB_DRIVEN_ACQUISITION

/*
 * Final CDC/USB packet PCM buffer.
 * Payload format:
 *   [L_BF, R_BF, L_BF, R_BF, ...]
 */
extern int16_t USBOUT[AUDIO_HOP_SAMPLES_PER_CH * AUDIO_BF_STREAM_CHANNELS];

/* AEC init debug symbols. */
extern volatile uint32_t g_aec_init_stage;
extern volatile uint32_t g_aec_last_error;
extern volatile uint32_t g_aec_memory_size[AUDIO_AEC_INSTANCES];
extern volatile uint32_t g_aec_memory_addr[AUDIO_AEC_INSTANCES];
extern volatile uint32_t g_aec_bypass_count;
extern volatile uint32_t g_aec_process_count;

/* BF init/debug symbols. */
extern volatile uint32_t g_bf_init_stage;
extern volatile uint32_t g_bf_last_error;
extern volatile uint32_t g_bf_memory_size_stream[AUDIO_STREAM_COUNT];
extern volatile uint32_t g_bf_memory_addr_stream[AUDIO_STREAM_COUNT];

/* Legacy names kept for existing watch/debug expressions. */
extern volatile uint32_t g_bf12_memory_size;
extern volatile uint32_t g_bf34_memory_size;
extern volatile uint32_t g_bf12_memory_addr;
extern volatile uint32_t g_bf34_memory_addr;

extern volatile uint32_t g_dbg_bf_before_fs;
extern volatile uint32_t g_dbg_bf_before_m1;
extern volatile uint32_t g_dbg_bf_before_m2;
extern volatile uint32_t g_dbg_bf_before_out;
extern volatile uint32_t g_dbg_bf_before_type;
extern volatile uint32_t g_dbg_bf_before_mem;
extern volatile uint32_t g_dbg_bf_before_size;

extern volatile uint32_t g_bf_usb_busy_drop;
extern volatile uint32_t g_bf_usb_send_count;
extern volatile float g_bf_usb_last_el_deg_stream[AUDIO_STREAM_COUNT];
extern volatile uint32_t g_bf_usb_last_el_valid_stream[AUDIO_STREAM_COUNT];

/* Common held elevation used by both D131MW motor control and USB packets. */
void AudioBF_SetLastElevationForStream(uint32_t stream_id, float el_deg);
float AudioBF_GetLastElevationForStream(uint32_t stream_id);
uint8_t AudioBF_GetLastElevationValidForStream(uint32_t stream_id);

/* Per-board steering/debug state. */
extern volatile float g_bf_target_az_deg_stream[AUDIO_STREAM_COUNT];
extern volatile uint32_t g_bf_target_valid_stream[AUDIO_STREAM_COUNT];
extern volatile uint32_t g_bf_selected_axis_stream[AUDIO_STREAM_COUNT];
extern volatile uint32_t g_bf_selected_front_mic_stream[AUDIO_STREAM_COUNT];
extern volatile uint32_t g_bf_selected_back_mic_stream[AUDIO_STREAM_COUNT];
extern volatile uint32_t g_bf_selected_pair_directed_index_stream[AUDIO_STREAM_COUNT];
extern volatile float g_bf_selected_pair_axis_deg_stream[AUDIO_STREAM_COUNT];
extern volatile float g_bf_selected_pair_error_deg_stream[AUDIO_STREAM_COUNT];

/* Legacy single-target names kept for compatibility. */
extern volatile float g_bf_target_az_deg;
extern volatile uint32_t g_bf_target_valid;
extern volatile uint32_t g_bf_selected_axis;
extern volatile uint32_t g_bf_selected_front_mic;
extern volatile uint32_t g_bf_selected_back_mic;

extern volatile uint32_t g_bf_beam_enabled_stream[AUDIO_STREAM_COUNT];

/* Exported functions ------------------------------------------------------- */
void Audio_Libraries_Init(void);
uint32_t AudioProcess(const uint16_t *tdm4ch, uint32_t frames_per_ch);
uint32_t AudioProcessForStream(uint32_t stream_id,
                               const uint16_t *tdm4ch,
                               uint32_t frames_per_ch,
                               int16_t *out_mono);

void AudioAEC_SetReferenceFrame(const int16_t *reference_mono,
                                uint32_t frames_per_ch);
void AudioAEC_ClearReferenceFrame(void);

void SW_Task1_Callback(void);
void SW_Task1_Start(void);

void AEC_BF_Process(void);

/* SRP-PHAT -> AcousticBF steering input. */
void AudioBF_SetTargetAzDegForStream(uint32_t stream_id, float az_deg);
void AudioBF_ClearTargetForStream(uint32_t stream_id);

/* Legacy wrappers: operate on L board. */
void AudioBF_SetTargetAzDeg(float az_deg);
void AudioBF_ClearTarget(void);

#ifdef __cplusplus
}
#endif

#endif /* INC_AUDIO_AEC_BF_PCM_H_ */
