//#ifndef SRP_PHAT_H
//#define SRP_PHAT_H
//
//#include <stdint.h>
//
//#define DOA_MAX_SOURCES 				2
//#define DOA_INTERNAL_MAX_SOURCES 		2
//
//typedef struct {
//    uint8_t active;
//    uint32_t id;
//    float az_deg;
//    float el_deg;
//    float score;
//} doa_result_t;
//
//typedef struct {
//    uint8_t valid;
//    uint8_t stream_id;       // AUDIO_STREAM_L 또는 AUDIO_STREAM_R
//    float az_deg;            // 모터로 보낼 최종 방위각
//    float el_deg;
//    uint32_t track_id;
//    uint32_t first_seq;
//    uint32_t first_sample_index;
//    uint32_t rms;
//} doa_first_sound_target_t;
//
//extern volatile uint32_t g_hop_ready_cnt;
//
///* Debug watch variables */
//extern volatile uint32_t g_doa_audio_present_stream[DOA_MAX_SOURCES];
//extern volatile uint32_t g_doa_last_rms_stream[DOA_MAX_SOURCES];
//extern volatile uint32_t g_doa_silence_hops_stream[DOA_MAX_SOURCES];
//
//extern volatile float g_doa_led_raw_az_deg_stream[DOA_MAX_SOURCES];
//extern volatile uint32_t g_doa_led_raw_active_stream[DOA_MAX_SOURCES];
//extern volatile uint32_t g_doa_led_raw_seq_stream[DOA_MAX_SOURCES];
//
//void doa_init(void);
//
//uint8_t DOA_GetFirstSoundMotorTarget(doa_first_sound_target_t *out);
//void DOA_ResetFirstSoundMotorTarget(void);
//float DOA_ConvertBoardAzToMotorAz(uint32_t stream_id, float board_az_deg);
//
///* compatibility wrappers */
//doa_result_t doa_process_frame(const int16_t *frame_interleaved_4ch);
//int doa_process_frame_top2(const int16_t *frame_interleaved_4ch,
//                           doa_result_t out[DOA_INTERNAL_MAX_SOURCES]);
//int doa_process_frame_separate(const int16_t *frame_interleaved_4ch,
//                               doa_result_t out[DOA_INTERNAL_MAX_SOURCES]);
//
//int doa_process_frame_single_azel(const int16_t *frame_interleaved_4ch, doa_result_t *out);
//void Degree_Proccess(void);
//
//uint8_t DOA_GetLatestTracks(doa_result_t out[DOA_MAX_SOURCES]);
//uint8_t DOA_GetLedRawAzimuthForStream(uint32_t stream_id, float *az_deg, uint8_t *active);
//uint8_t DOA_GetPrimaryTarget(float *az_deg, float *el_deg, uint32_t *track_id);
//
//uint8_t DOA_GetAudioPresentForStream(uint32_t stream_id);
//uint32_t DOA_GetLastRmsForStream(uint32_t stream_id);
//uint32_t DOA_GetSilenceHopsForStream(uint32_t stream_id);
//
//#endif /* INC_SRP_PHAT_H_ */





















#ifndef SRP_PHAT_H
#define SRP_PHAT_H

#include <stdint.h>

#define DOA_MAX_SOURCES 				2
#define DOA_INTERNAL_MAX_SOURCES 		2

typedef struct {
    uint8_t active;
    uint32_t id;
    float az_deg;
    float el_deg;             /* tracked/smoothed elevation */
    float score;

    /* Raw TDOA-LS elevation measurement for frame-quality selection. */
    uint8_t el_valid;
    float el_raw_deg;
    float el_resid;           /* TDOA-LS RMS residual in meters */
} doa_result_t;

typedef struct {
    uint8_t valid;
    uint8_t stream_id;       // AUDIO_STREAM_L 또는 AUDIO_STREAM_R
    float az_deg;            // 모터로 보낼 최종 방위각
    float srp_az_deg;        // motor latch 시점의 원본 SRP-PHAT 방위각 (board frame)
    float el_deg;            // tracked/smoothed elevation

    /* Raw TDOA-LS result from the newly processed DOA frame. */
    uint8_t el_valid;
    float el_raw_deg;
    float el_resid;          // TDOA-LS RMS residual in meters

    uint32_t track_id;
    uint32_t first_seq;
    uint32_t first_sample_index;
    uint32_t rms;
    uint32_t update_seq;
} doa_first_sound_target_t;

extern volatile uint32_t g_hop_ready_cnt;

/* Debug watch variables */
extern volatile uint32_t g_doa_audio_present_stream[DOA_MAX_SOURCES];
extern volatile uint32_t g_doa_last_rms_stream[DOA_MAX_SOURCES];
extern volatile uint32_t g_doa_silence_hops_stream[DOA_MAX_SOURCES];

/* Per-channel RMS debug variables (512 samples/channel frame). */
extern volatile uint32_t g_sai1a_rms_ch0;
extern volatile uint32_t g_sai1a_rms_ch1;
extern volatile uint32_t g_sai1a_rms_ch2;
extern volatile uint32_t g_sai1a_rms_ch3;
extern volatile uint32_t g_sai1b_rms_ch0;
extern volatile uint32_t g_sai1b_rms_ch1;
extern volatile uint32_t g_sai1b_rms_ch2;
extern volatile uint32_t g_sai1b_rms_ch3;

extern volatile float g_doa_led_raw_az_deg_stream[DOA_MAX_SOURCES];
extern volatile uint32_t g_doa_led_raw_active_stream[DOA_MAX_SOURCES];
extern volatile uint32_t g_doa_led_raw_seq_stream[DOA_MAX_SOURCES];

void doa_init(void);

uint8_t DOA_GetFirstSoundMotorTarget(doa_first_sound_target_t *out);
void DOA_ResetFirstSoundMotorTarget(void);
float DOA_ConvertBoardAzToMotorAz(uint32_t stream_id, float board_az_deg);

/* compatibility wrappers */
doa_result_t doa_process_frame(const int16_t *frame_interleaved_4ch);
int doa_process_frame_top2(const int16_t *frame_interleaved_4ch,
                           doa_result_t out[DOA_INTERNAL_MAX_SOURCES]);
int doa_process_frame_separate(const int16_t *frame_interleaved_4ch,
                               doa_result_t out[DOA_INTERNAL_MAX_SOURCES]);

int doa_process_frame_single_azel(const int16_t *frame_interleaved_4ch, doa_result_t *out);
void Degree_Proccess(void);

uint8_t DOA_GetLatestTracks(doa_result_t out[DOA_MAX_SOURCES]);
uint8_t DOA_GetLedRawAzimuthForStream(uint32_t stream_id, float *az_deg, uint8_t *active);
uint8_t DOA_GetPrimaryTarget(float *az_deg, float *el_deg, uint32_t *track_id);

uint8_t DOA_GetAudioPresentForStream(uint32_t stream_id);
uint32_t DOA_GetLastRmsForStream(uint32_t stream_id);
uint32_t DOA_GetSilenceHopsForStream(uint32_t stream_id);

#endif /* INC_SRP_PHAT_H_ */
