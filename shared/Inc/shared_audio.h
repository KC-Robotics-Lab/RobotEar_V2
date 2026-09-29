//#pragma once
//#include <stdint.h>
//
//#define AUDIO_FS_HZ                  (16000U)
//#define AUDIO_NMIC                   (4U)
//
//#define AUDIO_FRAME_SAMPLES_PER_CH   (512U)
//#define AUDIO_HOP_SAMPLES_PER_CH     (256U)
//
//#define SHARED_HOP_WORDS             (AUDIO_HOP_SAMPLES_PER_CH * AUDIO_NMIC)
//#define SHARED_AUDIO_WORDS           (SHARED_HOP_WORDS * 2U)
//
//#define SHARED_SLOT_HALF             (0U)
//#define SHARED_SLOT_FULL             (1U)
//#define SHARED_SLOT_COUNT            (2U)
//
//#define SHARED_HALF_OFFSET           (0U)
//#define SHARED_FULL_OFFSET           (SHARED_HOP_WORDS)
//
//#define AUDIO_STREAM_COUNT           (2U)
//#define AUDIO_STREAM_L               (0U)
//#define AUDIO_STREAM_R               (1U)
//
//typedef struct
//{
//    volatile uint32_t slot_guard[SHARED_SLOT_COUNT];
//    volatile uint32_t slot_seq[SHARED_SLOT_COUNT];
//    volatile uint32_t notify_seq;
//    volatile uint32_t overrun;
//    volatile uint32_t reserved[2];
//
//    volatile uint16_t samples[SHARED_AUDIO_WORDS];
//} shared_audio_stream_t;
//
//typedef struct
//{
//    shared_audio_stream_t stream[AUDIO_STREAM_COUNT];
//} shared_audio_frame_t;
//
//extern shared_audio_frame_t g_shared_audio;
















#pragma once
#include <stdint.h>

#define AUDIO_FS_HZ                  (16000U)
#define AUDIO_NMIC                   (4U)

#define AUDIO_FRAME_SAMPLES_PER_CH   (512U)
#define AUDIO_HOP_SAMPLES_PER_CH     (256U)

#define SHARED_HOP_WORDS             (AUDIO_HOP_SAMPLES_PER_CH * AUDIO_NMIC)
#define SHARED_AUDIO_WORDS           (SHARED_HOP_WORDS * 2U)

#define SHARED_SLOT_HALF             (0U)
#define SHARED_SLOT_FULL             (1U)
#define SHARED_SLOT_COUNT            (2U)

#define SHARED_HALF_OFFSET           (0U)
#define SHARED_FULL_OFFSET           (SHARED_HOP_WORDS)

#define AUDIO_STREAM_COUNT           (2U)
#define AUDIO_STREAM_L               (0U)
#define AUDIO_STREAM_R               (1U)

#define AUDIO_STREAM_MASK_L           (1UL << AUDIO_STREAM_L)
#define AUDIO_STREAM_MASK_R           (1UL << AUDIO_STREAM_R)
#define AUDIO_STREAM_MASK_ALL         (AUDIO_STREAM_MASK_L | AUDIO_STREAM_MASK_R)

#define AUDIO_STREAM_MASK_HAS(mask, stream_id) \
    ((((uint32_t)(mask)) & (1UL << (uint32_t)(stream_id))) != 0UL)

typedef struct
{
    volatile uint32_t slot_guard[SHARED_SLOT_COUNT];
    volatile uint32_t slot_seq[SHARED_SLOT_COUNT];
    volatile uint32_t notify_seq;
    volatile uint32_t overrun;
    volatile uint32_t reserved[2];

    volatile uint16_t samples[SHARED_AUDIO_WORDS];
} shared_audio_stream_t;

typedef struct
{
    shared_audio_stream_t stream[AUDIO_STREAM_COUNT];

    /* Runtime stream availability published by CM4. */
    volatile uint32_t active_stream_mask;
    volatile uint32_t config_seq;
} shared_audio_frame_t;

extern shared_audio_frame_t g_shared_audio;
