/**
******************************************************************************
* @file    acoustic_bf_cardoid.c
* @author  SRA
* @brief   Beamforming core library
******************************************************************************
* @attention
*
* Copyright (c) 2022 STMicroelectronics.
* All rights reserved.
*
* This software is licensed under terms that can be found in the LICENSE file in
* the root directory of this software component.
* If no LICENSE file comes with this software, it is provided AS-IS.
*
*
******************************************************************************
*/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __ACOUSTIC_BF_CARDOID_C
#define __ACOUSTIC_BF_CARDOID_C

/* Includes ------------------------------------------------------------------*/
#include "acoustic_bf_cardoid.h"
#include "cardoid.h"
#include "delay.h"
#include "pdm2pcm_glo.h"
#include <errno.h>
/* Private typedef -----------------------------------------------------------*/
struct _context_t;

typedef struct
{
  uint32_t (*DelayPdm)(Delay_Handler_t *const pDelayHdle, void *buffer_in, void *buffer_out, uint16_t sampling_frequency);
  uint8_t (*FirstStep)(struct _context_t *const pContext, void *pM1, void *pM2, int16_t *pOut);
} context_cb_t;


typedef struct
{
  PDM2PCM_Handler_t *pHdle; /* Keep pointer to allocate internal mem only if needed */
  PDM2PCM_Config_t  *pConf;
} pdm2pcm_t;

typedef struct
{
  uint16_t  libBitOrder;
  uint16_t  libEndianness;
  pdm2pcm_t m1;
  pdm2pcm_t m2;
  pdm2pcm_t m1Delayed;
  pdm2pcm_t m2Delayed;
} pdm2pcm_instances_t;

typedef struct
{
  Cardoid_Handler_t hdle;
  uint8_t           isRearBfNeeded; // to avoid multiple test, this set at init & config stage
  uint16_t          mic_distance;
} cardoid_t;

typedef struct
{
  Delay_Type_t      type;
  Delay_Handler_t  *pHdleM1;
  Delay_Handler_t  *pHdleM2;
  uint8_t          *pPdmBuff;
} delay_t;

typedef struct _context_t
{
  /*** Store in context all data from handler at init *******/
  uint32_t  sampling_frequency;
  uint8_t   data_format;
  uint8_t   ptr_M1_channels;
  uint8_t   ptr_M2_channels;
  uint8_t   ptr_out_channels;
  uint8_t   ref_select;
  uint8_t   rear_enable;
  uint8_t   hwIP;
  /*** context variables *******/
  uint8_t   bufferState;
  size_t    szBytes1ms;     // avoid multiplication in first step calls while running
  size_t    szBytes2ms;     // avoid multiplication in first step calls while running
  size_t    szBytes8ms;     // avoid multiplication in first step calls while running
  uint8_t   isInputPdm;      // to avoid multiple test (MSB & LSB), this set at init & config stage
  float32_t M2_gain;
  uint16_t  cntSamples8ms;
  uint16_t  cntSamples1ms;
  uint8_t   frameReadyCnt;
  int16_t   overall_gain;

  /*** Store in context cardoid data *******/
  cardoid_t cardoid;

  /*** Store in context some callbacks set at init to avoid test during firstStep (cpu optim )*******/
  context_cb_t Callbacks;

  /*** Store in context all data pointer to avoid setting again each firstStep *******/
  uint16_t *pPcmM1;
  uint16_t *pPcmM1Delayed;
  uint16_t *pPcmM2;
  uint16_t *pPcmM2Delayed;
  uint16_t *pPcmBeamFront;
  uint16_t *pPcmBeamRear;
  int16_t  *pPcmM18ms;
  int16_t  *pPcmBeamFront8ms;
  int16_t  *pPcmBeamRear8ms;
  int16_t  *pOut8ms;

  /*** Store in context all struct pointer for filtering and delays *******/
  delay_t   delay;
  pdm2pcm_instances_t *pPdmFilter;


  /*** Store in context struct pointer for post bf filtering *******/
  AcousticBF_cardoid_postProc_t postProc; /*!< Post processing handler */
} context_t;

/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
#define SPEED_OF_SOUND              343.0f
#define STR_LIB_NAME                "ST AcousticBF"
#define BUFF_8MS_NB_SPLES           128U /* 8ms * 16 samples per ms  */
#define BUFF_1MS_NB_SPLES           16U
#define BUFF_8MS_PINGPONG_NB_SPLES  (BUFF_8MS_NB_SPLES * 2U)
#define PCM_SAMPLES_SIZE_BYTES      sizeof(uint16_t)
#define PDM_SAMPLES_SIZE_BYTES      sizeof(uint8_t)
#define SIZEOF_ALIGN                ACOUSTIC_BF_SIZEOF_ALIGN

#ifndef ACOUSTIC_BF_CARDOID_HIGH_PASS_TAP
  #define ACOUSTIC_BF_CARDOID_HIGH_PASS_TAP 1932735281UL
#endif

/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static uint32_t s_initContext(context_t            *const pContext, AcousticBF_cardoid_Handler_t *const pAcousticBfHdle);
static uint32_t s_storeUserConf(context_t          *const pContext, AcousticBF_cardoid_Handler_t *const pAcousticBfHdle);
static uint32_t s_setEndianess(context_t           *const pContext, uint32_t hwIP);
static uint8_t  s_isFrameReady(context_t           *const pContext);
static void     s_storeCardoidsFrontRear(context_t *const pContext, int16_t *pPcmBeamFront, int16_t *pPcmBeamRear, int16_t *pPcmM1, int16_t *pOut);
static void     s_storeCardoidsFront(context_t     *const pContext, int16_t *pPcmBeamFront, int16_t *pPcmM1, int16_t *pOut);
static void     s_setRearBf(context_t              *const pContext);


/* Private functions for first step (on for each case depending on PCM vs PDM input and need for delay):
*--------------------------------------------------------------------------------------------------------
*/

/* Callbacks for PDM input */
/* if delay is applied in PDM domain */
static uint8_t s_firstStepPdmDelayPdmFront(context_t     *const pContext, void *pM1, void *pM2, int16_t *pOut);
static uint8_t s_firstStepPdmDelayPdmFrontRear(context_t *const pContext, void *pM1, void *pM2, int16_t *pOut);

/* if delay is applied in PCM domain */
static uint8_t s_firstStepPdmDelayPcmFront(context_t     *const pContext, void *pM1, void *pM2, int16_t *pOut);
static uint8_t s_firstStepPdmDelayPcmFrontRear(context_t *const pContext, void *pM1, void *pM2, int16_t *pOut);

/* First step callbacks for PCM input */
static uint8_t s_runPcmDelayedFrontRear(context_t  *const pContext, uint16_t *pPcmM1, uint16_t *pPcmM2, int16_t *pOut);
static uint8_t s_runPcmDelayedFront(context_t      *const pContext, uint16_t *pPcmM1, uint16_t *pPcmM2, int16_t *pOut);


/* if no delay is applied */
static uint8_t s_firstStepPcmNoDelayFront(context_t     *const pContext, void *pM1, void *pM2, int16_t *pOut);
static uint8_t s_firstStepPcmNoDelayFrontRear(context_t *const pContext, void *pM1, void *pM2, int16_t *pOut);

/* if delay is applied */
static uint8_t s_firstStepPcmDelayedFront(context_t     *const pContext, void *pM1, void *pM2, int16_t *pOut);
static uint8_t s_firstStepPcmDelayedFrontRear(context_t *const pContext, void *pM1, void *pM2, int16_t *pOut);

/* Private functions for second step (called in  a ping pong manner):
*---------------------------------------------------------------------
*/
static void s_runSecondStep(context_t *const pContext, int16_t *const pMic, int16_t *const pFront, int16_t *const pRear, int16_t *const pOut);


/* PDM filtering related functions
* ----------------------------------
*/
static uint16_t s_getPdmDecRatio(uint32_t sampling_frequency);
static uint32_t s_initPdmAndDelayInstances(context_t *const pContext);
static uint32_t s_initPcmAndDelayInstances(context_t *const pContext);
static uint32_t s_initPdmFilter(PDM2PCM_Handler_t *const pPdmHdle, PDM2PCM_Config_t *const pPdmConfig, uint16_t bit_order, uint16_t endianness, uint16_t nbChIn, uint16_t nbChOut, int16_t mic_gain, uint16_t decimation_factor);
//static void     s_delay_one_pcm(uint16_t *pDest, uint16_t *pSrc, size_t sizeBytes, uint16_t idLast);

/* Functions Definition ------------------------------------------------------*/

uint32_t AcousticBF_cardoid_Init(AcousticBF_cardoid_Handler_t *pHandler)
{
  context_t *const pContext = (context_t *)(pHandler->pInternalMemory);
  uint32_t ret = ACOUSTIC_BF_TYPE_ERROR_NONE;
  if (pContext == NULL)
  {
    ret = ACOUSTIC_BF_ALLOCATION_ERROR;
  }
  else
  {
    (void)memset(pHandler->pInternalMemory, 0, pHandler->internal_memory_size);
    ret = s_storeUserConf(pContext, pHandler);
  }

  if (ret == ACOUSTIC_BF_TYPE_ERROR_NONE)
  {
    ret = s_initContext(pContext, pHandler);
  }

  if (ret == ACOUSTIC_BF_TYPE_ERROR_NONE)
  {
    /*CASE PDM DATA INPUT*/
    if (pContext->isInputPdm == 1U)
    {
      ret |= s_initPdmAndDelayInstances(pContext);
    }
    else
    {
      ret |= s_initPcmAndDelayInstances(pContext);
    }
  }
  return ret;
}

uint32_t AcousticBF_cardoid_GetMemorySize(AcousticBF_cardoid_Handler_t *pHandler)
{
  uint32_t          byte_offset = SIZEOF_ALIGN(context_t);
  Cardoid_Handler_t cardoidHandler;
  uint8_t           nbAntennas = (pHandler->rear_enable == ACOUSTIC_BF_CARDOID_REAR_ENABLE) ? 2U : 1U;

  byte_offset += BUFF_8MS_PINGPONG_NB_SPLES * PCM_SAMPLES_SIZE_BYTES;           // pPcmM18ms todo: only if needed
  byte_offset += BUFF_8MS_PINGPONG_NB_SPLES * PCM_SAMPLES_SIZE_BYTES;           // pPcmBeamFront8ms
  byte_offset += BUFF_8MS_PINGPONG_NB_SPLES * PCM_SAMPLES_SIZE_BYTES;           // pPcmBeamRear8ms
  byte_offset += BUFF_8MS_PINGPONG_NB_SPLES * PCM_SAMPLES_SIZE_BYTES;           // pOut8ms
  byte_offset += BUFF_1MS_NB_SPLES          * PCM_SAMPLES_SIZE_BYTES;           // pPcmM1
  byte_offset += BUFF_1MS_NB_SPLES          * PCM_SAMPLES_SIZE_BYTES;           // pPcmM2
  byte_offset += (uint32_t)nbAntennas * BUFF_1MS_NB_SPLES * PCM_SAMPLES_SIZE_BYTES;       // pPcmBeamFront & pPcmBeamRear


  if (pHandler->delay_enable == ACOUSTIC_BF_CARDOID_DELAY_ENABLE)
  {
    // memory for internal delay context delay.c
    Delay_Handler_t delayHdle;
    delayHdle.nb_samples = BUFF_1MS_NB_SPLES;
    Delay_getMemorySize(&delayHdle);
    byte_offset += (uint32_t)nbAntennas * delayHdle.internal_memory_size;
    byte_offset += (uint32_t)nbAntennas * SIZEOF_ALIGN(Delay_Handler_t);                    // pHdleDelayM2 and pHdleDelayM1 if rear needed
    byte_offset += (uint32_t)nbAntennas * BUFF_1MS_NB_SPLES * PCM_SAMPLES_SIZE_BYTES;       // delayed PDM data converted in PCM data
  }
  if ((pHandler->data_format == ACOUSTIC_BF_CARDOID_DATA_FORMAT_PDM_MSB) ||
      (pHandler->data_format == ACOUSTIC_BF_CARDOID_DATA_FORMAT_PDM_LSB))
  {
    uint8_t nbPdmInstances = 2U * nbAntennas;
    byte_offset += SIZEOF_ALIGN(pdm2pcm_instances_t);
    byte_offset += nbPdmInstances * (SIZEOF_ALIGN(PDM2PCM_Handler_t) + SIZEOF_ALIGN(PDM2PCM_Config_t));
    byte_offset += 256UL * PDM_SAMPLES_SIZE_BYTES;   // pContext->delay.pPdmBuff
  }

  Cardoid_getMemorySize(&cardoidHandler);
  byte_offset += cardoidHandler.internal_memory_size;

  while ((++byte_offset % 4U) != 0U)
  {
  }
  pHandler->internal_memory_size = byte_offset;
  return ACOUSTIC_BF_TYPE_ERROR_NONE;
}

uint32_t AcousticBF_cardoid_SetHWIP(AcousticBF_cardoid_Handler_t *pHandler, uint32_t hwIP)
{
  uint32_t         ret      = ACOUSTIC_BF_TYPE_ERROR_NONE;
  context_t *const pContext = (context_t *)(pHandler->pInternalMemory);
  if (hwIP != pContext->hwIP)
  {
    ret |= s_setEndianess(pContext, hwIP);

    /* Reinit PDM filter wiz proper endianess */
    if (pContext->isInputPdm == 1U)
    {
      ret |= s_initPdmAndDelayInstances(pContext);
    }
  }
  return ret;
}

uint32_t AcousticBF_cardoid_FirstStep(void *pM1, void *pM2, void *pOut, AcousticBF_cardoid_Handler_t *pHandler)
{
  context_t *const pContext = (context_t *)(pHandler->pInternalMemory);
  return pContext->Callbacks.FirstStep(pContext, pM1, pM2, pOut);
}


uint32_t AcousticBF_cardoid_SecondStep(AcousticBF_cardoid_Handler_t *pHandler)
{
  uint32_t ret = ACOUSTIC_BF_TYPE_ERROR_NONE;
  context_t *const pContext = (context_t *)(pHandler->pInternalMemory);

  if (pContext->bufferState == 1U)
  {
    pContext->bufferState = 0U;
    s_runSecondStep(pContext, pContext->pPcmM18ms, pContext->pPcmBeamFront8ms, pContext->pPcmBeamRear8ms,  &pContext->pOut8ms[128]);
  }
  else if (pContext->bufferState == 2U)
  {
    pContext->bufferState = 0U;
    s_runSecondStep(pContext, &pContext->pPcmM18ms[128], &pContext->pPcmBeamFront8ms[128], &pContext->pPcmBeamRear8ms[128], pContext->pOut8ms);
  }
  else
  {
    ret = ACOUSTIC_BF_PROCESSING_ERROR;
  }
  return ret;
}

uint32_t AcousticBF_cardoid_RegisterPostProc(AcousticBF_cardoid_Handler_t *pHandler, AcousticBF_cardoid_postProc_t *pPostProc)
{
  context_t *const pContext = (context_t *)(pHandler->pInternalMemory);
  uint32_t ret = ACOUSTIC_BF_TYPE_ERROR_NONE;

  if (pContext != NULL)
  {
    pContext->postProc.pCb   = pPostProc->pCb;
    pContext->postProc.pHdle = pPostProc->pHdle;
  }
  else
  {
    ret = ACOUSTIC_BF_ALLOCATION_ERROR;
  }
  return ret;
}

uint32_t AcousticBF_cardoid_SetConfig(AcousticBF_cardoid_Handler_t *pHandler, AcousticBF_cardoid_Config_t *pConfig)
{
  context_t *const pContext = (context_t *)(pHandler->pInternalMemory);
  uint32_t ret              = ACOUSTIC_BF_TYPE_ERROR_NONE;
  uint8_t dist_has_changed  = 0U;
  uint8_t vol_has_changed  = 0U;

  if (pConfig->M2_gain >= 0.0f)
  {
    pContext->M2_gain = pConfig->M2_gain;
  }
  else
  {
    pContext->M2_gain = 1.0f;
    ret |= ACOUSTIC_BF_M2_GAIN_ERROR;
  }

  if (pConfig->volume != pContext->overall_gain)
  {
    vol_has_changed = 1U;
    pContext->overall_gain = pConfig->volume;
  }
  if (pHandler->data_format == ACOUSTIC_BF_CARDOID_DATA_FORMAT_PCM)
  {
    /* TODO : in case of no delay, mic_distance can be shorter than 210 ? */
    pContext->cardoid.mic_distance = pConfig->mic_distance;
    if ((pHandler->delay_enable == 1U) && ((pConfig->mic_distance < 210U) || (pConfig->mic_distance > 212U)))
    {
      pContext->cardoid.mic_distance = 210U;
      ret |= ACOUSTIC_BF_DISTANCE_ERROR;
    }
  }
  else
  {
    if (pConfig->mic_distance != pContext->cardoid.mic_distance)
    {
      dist_has_changed = 1U;
      if ((pConfig->mic_distance > 0U) && (pConfig->mic_distance <= 212U))
      {
        pContext->cardoid.mic_distance = pConfig->mic_distance;
      }
      else
      {
        pContext->cardoid.mic_distance = 150;
        ret |= ACOUSTIC_BF_DISTANCE_ERROR;
      }
    }
  }

  if ((pConfig->ref_select != ACOUSTIC_BF_CARDOID_REF_DISABLE) && (pHandler->ptr_out_channels < 2U))
  {
    ret |= ACOUSTIC_BF_REF_OUT_ERROR;
  }
  else
  {
    if ((pContext->rear_enable == ACOUSTIC_BF_CARDOID_REAR_DISABLE) && (pConfig->ref_select == ACOUSTIC_BF_CARDOID_REF_OPPOSITE_ANTENNA))
    {
      pContext->ref_select = (uint8_t)ACOUSTIC_BF_CARDOID_REF_DISABLE;
      ret |= ACOUSTIC_BF_ALLOCATION_ERROR;
    }
    else
    {
      pContext->ref_select = pConfig->ref_select;
    }
  }

  s_setRearBf(pContext);

  Cardoid_Config_t cardoid_conf;
  cardoid_conf.mic_distance = pConfig->mic_distance;
  cardoid_conf.rear_enable  = pContext->cardoid.isRearBfNeeded;

  ret |= Cardoid_setConfig(&pContext->cardoid.hdle, &cardoid_conf);

  if ((pContext->isInputPdm == 1U) && ((vol_has_changed == 1U) || (dist_has_changed == 1U)))
  {
    ret |= s_initPdmAndDelayInstances(pContext);
  }

  /************************************/
  float32_t internal_gain = 1.0f;
  if (pContext->M2_gain > 0.0f)
  {
    internal_gain = pContext->M2_gain;
  }
  Cardoid_setGain(&pContext->cardoid.hdle, internal_gain);

  return ret;
}


uint32_t AcousticBF_cardoid_GetConfig(AcousticBF_cardoid_Handler_t *pHandler, AcousticBF_cardoid_Config_t *pConfig)
{
  context_t *const pContext = (context_t *)(pHandler->pInternalMemory);

  Cardoid_Config_t cardoid_config;
  Cardoid_getConfig(&pContext->cardoid.hdle, &cardoid_config);
  if (pContext->M2_gain == 0.0f)
  {
    Cardoid_getGain(&pContext->cardoid.hdle, &pConfig->M2_gain);
  }
  else
  {
    pConfig->M2_gain = pContext->M2_gain;
  }

  pConfig->volume = pContext->overall_gain;
  pConfig->mic_distance = cardoid_config.mic_distance;
  return ACOUSTIC_BF_TYPE_ERROR_NONE;
}


uint32_t AcousticBF_cardoid_GetLibVersion(char *version)
{
  char dest[35] = STR_LIB_NAME;
  (void)strcat(dest, CARDOID_VERSION);
  (void)strcpy(version, dest);
  return strlen(dest);
}


/******************************************************************************/
/********************STATIC FUNCTIONS *****************************************/
/******************************************************************************/


static uint32_t s_storeUserConf(context_t *const pContext, AcousticBF_cardoid_Handler_t *const pAcousticBfHdle)
{
  uint32_t ret = ACOUSTIC_BF_TYPE_ERROR_NONE;
  if (pAcousticBfHdle->ptr_out_channels > 0U)
  {
    pContext->ptr_out_channels = pAcousticBfHdle->ptr_out_channels;
  }
  else
  {
    pContext->ptr_out_channels = 2U;
    ret |= ACOUSTIC_BF_PTR_CHANNELS_ERROR;
  }

  if ((pAcousticBfHdle->rear_enable == ACOUSTIC_BF_CARDOID_REAR_ENABLE) ||
      (pAcousticBfHdle->rear_enable == ACOUSTIC_BF_CARDOID_REAR_DISABLE))
  {
    pContext->rear_enable = pAcousticBfHdle->rear_enable;
  }
  else
  {
    pContext->rear_enable = (uint8_t)ACOUSTIC_BF_CARDOID_REAR_DISABLE;
    ret |= ACOUSTIC_BF_REF_OUT_ERROR;
  }


  /*interleaving mic 1*/
  if (pAcousticBfHdle->ptr_M1_channels > 0U)
  {
    pContext->ptr_M1_channels = pAcousticBfHdle->ptr_M1_channels;
  }
  else
  {
    pContext->ptr_M1_channels = 2;
    ret |= ACOUSTIC_BF_PTR_CHANNELS_ERROR;
  }

  /*interleaving mic 2*/
  if (pAcousticBfHdle->ptr_M2_channels > 0U)
  {
    pContext->ptr_M2_channels = pAcousticBfHdle->ptr_M2_channels;
  }
  else
  {
    pContext->ptr_M2_channels = 2;
    ret |= ACOUSTIC_BF_PTR_CHANNELS_ERROR;
  }

  /*sampling frequency*/
  if ((pAcousticBfHdle->sampling_frequency == 16U)   || (pAcousticBfHdle->sampling_frequency == 768U) ||
      (pAcousticBfHdle->sampling_frequency == 1024U) || (pAcousticBfHdle->sampling_frequency == 256U) ||
      (pAcousticBfHdle->sampling_frequency == 512U)  || (pAcousticBfHdle->sampling_frequency == 384U) ||
      (pAcousticBfHdle->sampling_frequency == 1280U) || (pAcousticBfHdle->sampling_frequency == 2048U))
  {
    pContext->sampling_frequency = pAcousticBfHdle->sampling_frequency;
  }
  else
  {
    pContext->sampling_frequency = 1024;
    ret |= ACOUSTIC_BF_SAMPLING_FREQ_ERROR;
  }

  /*Data Type PDM or PCM*/
  if ((pAcousticBfHdle->data_format == ACOUSTIC_BF_CARDOID_DATA_FORMAT_PDM_LSB) ||
      (pAcousticBfHdle->data_format == ACOUSTIC_BF_CARDOID_DATA_FORMAT_PDM_MSB) ||
      (pAcousticBfHdle->data_format == ACOUSTIC_BF_CARDOID_DATA_FORMAT_PCM))
  {
    pContext->data_format = (uint8_t)pAcousticBfHdle->data_format;
  }
  else
  {
    pContext->data_format = (uint8_t)ACOUSTIC_BF_CARDOID_DATA_FORMAT_PDM_MSB;
    ret |= ACOUSTIC_BF_DATA_FORMAT_ERROR;
  }

  if ((pAcousticBfHdle->data_format == ACOUSTIC_BF_CARDOID_DATA_FORMAT_PDM_LSB) ||
      (pAcousticBfHdle->data_format == ACOUSTIC_BF_CARDOID_DATA_FORMAT_PDM_MSB))
  {
    pContext->isInputPdm     = 1U;
  }

  /*Delay performed*/
  if ((pContext->isInputPdm == 1U) && (pAcousticBfHdle->delay_enable == 1U))
  {
    pContext->delay.type = DELAY_PDM;
  }
  else if ((pContext->isInputPdm == 1U) && (pAcousticBfHdle->delay_enable == 0U))
  {
    pContext->delay.type = DELAY_PDM;
    ret |= ACOUSTIC_BF_DELAY_ERROR;
  }
  else if (pAcousticBfHdle->delay_enable == 1U)
  {
    pContext->delay.type = DELAY_PCM;
  }
  else if (pAcousticBfHdle->delay_enable == 0U)
  {
    pContext->delay.type = DELAY_NONE;
  }
  else
  {
    ret |= ACOUSTIC_BF_DELAY_ERROR;
  }

  s_setRearBf(pContext);
  return ret;
}

static uint32_t s_initContext(context_t *const pContext, AcousticBF_cardoid_Handler_t *const pAcousticBfHdle)
{
  uint32_t ret = ACOUSTIC_BF_TYPE_ERROR_NONE;
  uint32_t  byte_offset = SIZEOF_ALIGN(context_t);

  /* Initialize internal variables */
  pContext->bufferState   = 0;
  pContext->cntSamples8ms = 0;
  pContext->cntSamples1ms = BUFF_8MS_PINGPONG_NB_SPLES / 2U;
  pContext->szBytes1ms    = PCM_SAMPLES_SIZE_BYTES * BUFF_1MS_NB_SPLES;
  pContext->szBytes2ms    = 2UL * pContext->szBytes1ms;
  pContext->szBytes8ms    = 8UL * pContext->szBytes1ms;
  Cardoid_getMemorySize(&pContext->cardoid.hdle);

  pContext->cardoid.hdle.pInternalMemory = (uint32_t *)((uint8_t *)pAcousticBfHdle->pInternalMemory + byte_offset);
  byte_offset += pContext->cardoid.hdle.internal_memory_size;
  ret = Cardoid_init(&pContext->cardoid.hdle);

  if (ret == ACOUSTIC_BF_TYPE_ERROR_NONE)
  {
    pContext->pPcmM18ms = (int16_t *)((uint8_t *)pAcousticBfHdle->pInternalMemory + byte_offset);
    byte_offset += BUFF_8MS_PINGPONG_NB_SPLES * PCM_SAMPLES_SIZE_BYTES;

    pContext->pPcmBeamFront8ms = (int16_t *)((uint8_t *)pAcousticBfHdle->pInternalMemory + byte_offset);
    byte_offset += BUFF_8MS_PINGPONG_NB_SPLES * PCM_SAMPLES_SIZE_BYTES;

    pContext->pPcmBeamRear8ms = (int16_t *)((uint8_t *)pAcousticBfHdle->pInternalMemory + byte_offset);
    byte_offset += BUFF_8MS_PINGPONG_NB_SPLES * PCM_SAMPLES_SIZE_BYTES;

    pContext->pOut8ms = (int16_t *)((uint8_t *)pAcousticBfHdle->pInternalMemory + byte_offset);
    byte_offset += BUFF_8MS_PINGPONG_NB_SPLES * PCM_SAMPLES_SIZE_BYTES;

    pContext->pPcmM1 = (uint16_t *)((uint8_t *)pAcousticBfHdle->pInternalMemory + byte_offset);
    byte_offset += BUFF_1MS_NB_SPLES * PCM_SAMPLES_SIZE_BYTES;

    pContext->pPcmM2 = (uint16_t *)((uint8_t *)pAcousticBfHdle->pInternalMemory + byte_offset);
    byte_offset += BUFF_1MS_NB_SPLES * PCM_SAMPLES_SIZE_BYTES;

    pContext->pPcmBeamFront = (uint16_t *)((uint8_t *)pAcousticBfHdle->pInternalMemory + byte_offset);
    byte_offset += BUFF_1MS_NB_SPLES * PCM_SAMPLES_SIZE_BYTES;

    if (pContext->cardoid.isRearBfNeeded)
    {
      pContext->pPcmBeamRear = (uint16_t *)((uint8_t *)pAcousticBfHdle->pInternalMemory + byte_offset);
      byte_offset += BUFF_1MS_NB_SPLES * PCM_SAMPLES_SIZE_BYTES;
    }

    if (pContext->delay.type != DELAY_NONE)
    {
      /* Instanciate delay.c items */
      pContext->delay.pHdleM2 = (Delay_Handler_t *)((uint8_t *)pAcousticBfHdle->pInternalMemory + byte_offset);
      byte_offset += SIZEOF_ALIGN(Delay_Handler_t);
      pContext->delay.pHdleM2->nb_samples = BUFF_1MS_NB_SPLES;
      Delay_getMemorySize(pContext->delay.pHdleM2);
      pContext->delay.pHdleM2->pInternalMemory = (uint32_t *)((uint8_t *)pAcousticBfHdle->pInternalMemory + byte_offset);
      byte_offset += pContext->delay.pHdleM2->internal_memory_size;                 // internal delay context

      /* pointer to pcm delayed buffer */
      pContext->pPcmM2Delayed = (uint16_t *)((uint8_t *)pAcousticBfHdle->pInternalMemory + byte_offset);
      byte_offset += BUFF_1MS_NB_SPLES  * PCM_SAMPLES_SIZE_BYTES;

      if (pContext->cardoid.isRearBfNeeded)
      {
        pContext->delay.pHdleM1 = (Delay_Handler_t *)((uint8_t *)pAcousticBfHdle->pInternalMemory + byte_offset);
        byte_offset += SIZEOF_ALIGN(Delay_Handler_t);
        pContext->delay.pHdleM1->nb_samples = BUFF_1MS_NB_SPLES;
        Delay_getMemorySize(pContext->delay.pHdleM1);
        pContext->delay.pHdleM1->pInternalMemory = (uint32_t *)((uint8_t *)pAcousticBfHdle->pInternalMemory + byte_offset);
        byte_offset += pContext->delay.pHdleM1->internal_memory_size;                 // internal delay context

        /* pointer to pcm delayed buffer */
        pContext->pPcmM1Delayed = (uint16_t *)((uint8_t *)pAcousticBfHdle->pInternalMemory + byte_offset);
        byte_offset += BUFF_1MS_NB_SPLES * PCM_SAMPLES_SIZE_BYTES;

      }
    }

    if (pContext->isInputPdm == 1U)
    {
      pContext->hwIP = (uint8_t)ACOUSTIC_BF_CARDOID_PDM_IP_SAI_PDM; /* Set default value ; can be overwritten wiz AcousticBF_cardoid_setHWIP */

      pContext->pPdmFilter = (pdm2pcm_instances_t *)((uint8_t *)pAcousticBfHdle->pInternalMemory + byte_offset);
      byte_offset += SIZEOF_ALIGN(pdm2pcm_instances_t);
      pdm2pcm_instances_t *const pPdmFilter = pContext->pPdmFilter;

      s_setEndianess(pContext, pContext->hwIP);

      pContext->pPdmFilter->libBitOrder = (pAcousticBfHdle->data_format == ACOUSTIC_BF_CARDOID_DATA_FORMAT_PDM_LSB) ? PDM2PCM_BIT_ORDER_LSB : PDM2PCM_BIT_ORDER_MSB;

      /* pM1 always used */
      pPdmFilter->m1.pHdle = (PDM2PCM_Handler_t *)((uint8_t *)pAcousticBfHdle->pInternalMemory + byte_offset);
      byte_offset         += SIZEOF_ALIGN(PDM2PCM_Handler_t);
      pPdmFilter->m1.pConf = (PDM2PCM_Config_t *)((uint8_t *)pAcousticBfHdle->pInternalMemory + byte_offset);
      byte_offset         += SIZEOF_ALIGN(PDM2PCM_Config_t);

      if (pContext->delay.type == DELAY_PDM)
      {
        pPdmFilter->m2Delayed.pHdle = (PDM2PCM_Handler_t *)((uint8_t *)pAcousticBfHdle->pInternalMemory + byte_offset);
        byte_offset                += SIZEOF_ALIGN(PDM2PCM_Handler_t);
        pPdmFilter->m2Delayed.pConf = (PDM2PCM_Config_t *)((uint8_t *)pAcousticBfHdle->pInternalMemory + byte_offset);
        byte_offset                += SIZEOF_ALIGN(PDM2PCM_Config_t);

        if (pContext->cardoid.isRearBfNeeded == 1U)
        {
          /* Rear antenna means we need pM2  & pM1 delayed */
          pPdmFilter->m2.pHdle = (PDM2PCM_Handler_t *)((uint8_t *)pAcousticBfHdle->pInternalMemory + byte_offset);
          byte_offset         += SIZEOF_ALIGN(PDM2PCM_Handler_t);
          pPdmFilter->m2.pConf = (PDM2PCM_Config_t *)((uint8_t *)pAcousticBfHdle->pInternalMemory + byte_offset);
          byte_offset         += SIZEOF_ALIGN(PDM2PCM_Config_t);

          pPdmFilter->m1Delayed.pHdle = (PDM2PCM_Handler_t *)((uint8_t *)pAcousticBfHdle->pInternalMemory + byte_offset);
          byte_offset                += SIZEOF_ALIGN(PDM2PCM_Handler_t);
          pPdmFilter->m1Delayed.pConf = (PDM2PCM_Config_t *)((uint8_t *)pAcousticBfHdle->pInternalMemory + byte_offset);
          byte_offset                += SIZEOF_ALIGN(PDM2PCM_Config_t);
        }
        /* Allocation of pdm buffer for delay processing ; cannot be done in place since it would be the user buffer */
        pContext->delay.pPdmBuff = (uint8_t *)pAcousticBfHdle->pInternalMemory + byte_offset;
        byte_offset              +=  256UL  * PDM_SAMPLES_SIZE_BYTES;  // pPdmDelayed
      }
      else
      {
        /* Todo check if can be removed */
        pPdmFilter->m2.pHdle = (PDM2PCM_Handler_t *)((uint8_t *)pAcousticBfHdle->pInternalMemory + byte_offset);
        byte_offset         += SIZEOF_ALIGN(PDM2PCM_Handler_t);
        pPdmFilter->m2.pConf = (PDM2PCM_Config_t *)((uint8_t *)pAcousticBfHdle->pInternalMemory + byte_offset);
        byte_offset         += SIZEOF_ALIGN(PDM2PCM_Config_t);
      }
    }

    while ((++byte_offset % 4U) != 0U)
    {
    }
    if (byte_offset != pAcousticBfHdle->internal_memory_size)
    {
      ret |= ACOUSTIC_BF_ALLOCATION_ERROR;
    }
  }
  return ret;
}

static void s_runSecondStep(context_t *const pContext, int16_t *const pMic, int16_t *const pFront, int16_t *const pRear, int16_t *const pOut)
{
  memcpy(pOut, pFront, pContext->szBytes8ms);
  if (pContext->postProc.pCb != NULL)
  {
    pContext->postProc.pCb(pContext->postProc.pHdle, pMic, pFront, pRear, pOut);
  }
}


static uint32_t s_initPdmFilter(PDM2PCM_Handler_t *const pPdmHdle, PDM2PCM_Config_t *const pPdmConfig, uint16_t bit_order, uint16_t endianness, uint16_t nbChIn, uint16_t nbChOut, int16_t mic_gain, uint16_t decimation_factor)
{
  uint32_t ret = ACOUSTIC_BF_TYPE_ERROR_NONE;

  pPdmHdle->bit_order               = bit_order;
  pPdmHdle->endianness              = endianness;
  pPdmHdle->high_pass_tap           = ACOUSTIC_BF_CARDOID_HIGH_PASS_TAP;
  pPdmHdle->in_ptr_channels         = nbChIn;
  pPdmHdle->out_ptr_channels        = nbChOut;

  ret = PDM2PCM_init(pPdmHdle) << ACOUSTIC_BF_PDM2PCM_ERROR_SHIFT;
  if (ret == 0UL)
  {
    pPdmConfig->output_samples_number = BUFF_1MS_NB_SPLES;
    pPdmConfig->mic_gain              = mic_gain;
    pPdmConfig->decimation_factor     = decimation_factor;
    ret = PDM2PCM_setConfig(pPdmHdle, pPdmConfig) << ACOUSTIC_BF_PDM2PCM_ERROR_SHIFT;
  }
  return ret;
}


static uint32_t s_initPcmAndDelayInstances(context_t *const pContext)
{
  uint32_t ret = ACOUSTIC_BF_TYPE_ERROR_NONE;

  /*PCM INPUT */
  if (pContext->delay.type == DELAY_PCM)
  {
    ret |= Delay_init(pContext->delay.pHdleM2, 1U, BUFF_1MS_NB_SPLES, 1U);

    if (pContext->cardoid.isRearBfNeeded == 1U)
    {
      ret |= Delay_init(pContext->delay.pHdleM1, 1U, BUFF_1MS_NB_SPLES, 1U);
      pContext->Callbacks.FirstStep = s_firstStepPcmDelayedFrontRear;
    }
    else
    {
      pContext->Callbacks.FirstStep = s_firstStepPcmDelayedFront;
    }
  }
  else /* DELAY_NONE */
  {
    if (pContext->cardoid.isRearBfNeeded == 1U)
    {
      pContext->Callbacks.FirstStep = s_firstStepPcmNoDelayFrontRear;
    }
    else
    {
      pContext->Callbacks.FirstStep = s_firstStepPcmNoDelayFront;
    }
  }
  return ret;
}

static uint32_t s_initPdmAndDelayInstances(context_t *const pContext)
{
  uint32_t ret = ACOUSTIC_BF_TYPE_ERROR_NONE;
  /*Get pPdmFilter pointers*/
  pdm2pcm_instances_t *const pPdmFilter = pContext->pPdmFilter;

  pdm2pcm_t *pPdmFilterM1        = &pPdmFilter->m1;
  pdm2pcm_t *pPdmFilterM2        = &pPdmFilter->m2;
  pdm2pcm_t *pPdmFilterM1Delayed = &pPdmFilter->m1Delayed;
  pdm2pcm_t *pPdmFilterM2Delayed = &pPdmFilter->m2Delayed;

  uint16_t decimFactor = s_getPdmDecRatio(pContext->sampling_frequency);
  uint16_t pdmDelay    = 0U;

  if (decimFactor == 0U)
  {
    ret = ACOUSTIC_BF_SAMPLING_FREQ_ERROR;
  }

  if (ret == ACOUSTIC_BF_TYPE_ERROR_NONE)
  {
    errno = 0;
    pdmDelay = (uint16_t)roundf(((float32_t)pContext->sampling_frequency * 1000.0f) * ((float32_t)pContext->cardoid.mic_distance / 10000.0f) / SPEED_OF_SOUND);
    if (errno != 0)
    {
      ret = ACOUSTIC_BF_TYPE_ERROR;
      errno = 0;
    }
  }

  if (ret == ACOUSTIC_BF_TYPE_ERROR_NONE)
  {
    /*PDM to PCM, BF, Delay structure allocation for 1 cardioid case (NONE OR LIGHT) -> always */
    if (pContext->cardoid.mic_distance < 210U)
    {
      /* Pdm input, delay in pdm then pdm to pcm*/
      ret = s_initPdmFilter(pPdmFilterM1->pHdle,
                            pPdmFilterM1->pConf,
                            pPdmFilter->libBitOrder,
                            pPdmFilter->libEndianness,
                            pContext->ptr_M1_channels,
                            1U,
                            pContext->overall_gain,
                            decimFactor);

      if (ret == ACOUSTIC_BF_TYPE_ERROR_NONE)
      {
        ret = s_initPdmFilter(pPdmFilterM2Delayed->pHdle,
                              pPdmFilterM2Delayed->pConf,
                              pPdmFilter->libBitOrder,
                              PDM2PCM_ENDIANNESS_LE,  /* Do not put BE because this is not coming from HW ; it's a recopy; so samples are always in the good order. No need to revert */
                              1U,
                              1U,
                              pContext->overall_gain,
                              decimFactor);
      }
      if (ret == ACOUSTIC_BF_TYPE_ERROR_NONE)
      {
        Delay_init(pContext->delay.pHdleM2, pContext->ptr_M2_channels, (uint16_t)(pContext->sampling_frequency) / 8U, pdmDelay);
        if (pContext->data_format == ACOUSTIC_BF_CARDOID_DATA_FORMAT_PDM_MSB)
        {
          pContext->Callbacks.DelayPdm = Delay_pdmMsb;
        }
        else
        {
          pContext->Callbacks.DelayPdm = Delay_pdmLsb;
        }
        if (pContext->cardoid.isRearBfNeeded == 1U)
        {
          pContext->Callbacks.FirstStep  = s_firstStepPdmDelayPdmFrontRear;
        }
        else
        {
          pContext->Callbacks.FirstStep  = s_firstStepPdmDelayPdmFront;
        }
      }
    }
    else
    {
      pContext->delay.type = DELAY_PCM;

      /* Pdm input, pdm to pcm then delay performed in pcm domain seen the distance is >21 and <23 mm*/
      ret = s_initPdmFilter(pPdmFilterM1->pHdle,
                            pPdmFilterM1->pConf,
                            pPdmFilter->libBitOrder,
                            pPdmFilter->libEndianness,
                            pContext->ptr_M1_channels,
                            1U,
                            pContext->overall_gain,
                            decimFactor);
      if (ret == ACOUSTIC_BF_TYPE_ERROR_NONE)
      {
        ret = s_initPdmFilter(pPdmFilterM2->pHdle,
                              pPdmFilterM2->pConf,
                              pPdmFilter->libBitOrder,
                              pPdmFilter->libEndianness,
                              pContext->ptr_M2_channels,
                              1U,
                              pContext->overall_gain,
                              decimFactor);
      }
      if (ret == ACOUSTIC_BF_TYPE_ERROR_NONE)
      {
        Delay_init(pContext->delay.pHdleM2, 1U, BUFF_1MS_NB_SPLES, 1U);

        pContext->Callbacks.DelayPdm = NULL;

        if (pContext->cardoid.isRearBfNeeded == 1U)
        {
          Delay_init(pContext->delay.pHdleM1, 1U, BUFF_1MS_NB_SPLES, 1U);
          pContext->Callbacks.FirstStep  = s_firstStepPdmDelayPcmFrontRear;
        }
        else
        {
          pContext->Callbacks.FirstStep  = s_firstStepPdmDelayPcmFront;
        }
      }
    }

    /* PDM input and for ACOUSTIC_BF_TYPE_STRONG & ACOUSTIC_BF_TYPE_ASR_READY:
    *   + 2 opposite cardioids needed
    *   + M2 and M1delayed are also needed
    */
    if (pContext->cardoid.isRearBfNeeded == 1U)
    {
      if (pContext->cardoid.mic_distance < 210U)
      {
        ret = s_initPdmFilter(pPdmFilterM2->pHdle,
                              pPdmFilterM2->pConf,
                              pPdmFilter->libBitOrder,
                              pPdmFilter->libEndianness,
                              pContext->ptr_M2_channels,
                              1U,
                              pContext->overall_gain,
                              decimFactor);
        if (ret == ACOUSTIC_BF_TYPE_ERROR_NONE)
        {
          ret = s_initPdmFilter(pPdmFilterM1Delayed->pHdle,
                                pPdmFilterM1Delayed->pConf,
                                pPdmFilter->libBitOrder,
                                PDM2PCM_ENDIANNESS_LE,  /* Do not put BE because this is not coming from HW ; it's a recopy; so samples are always in the good order. No need to revert */
                                1U,
                                1U,
                                pContext->overall_gain,
                                decimFactor);
        }
        if (ret == ACOUSTIC_BF_TYPE_ERROR_NONE)
        {
          Delay_init(pContext->delay.pHdleM1, pContext->ptr_M1_channels, (uint16_t)(pContext->sampling_frequency) / 8U, pdmDelay);

        }
        else
        {
          Delay_init(pContext->delay.pHdleM1, 1U, BUFF_1MS_NB_SPLES, 1U);
        }
      }
    }
  }
  return ret;
}

static void s_setRearBf(context_t  *const pContext)
{
  if ((pContext->ref_select == ACOUSTIC_BF_CARDOID_REF_OPPOSITE_ANTENNA) || (pContext->rear_enable == ACOUSTIC_BF_CARDOID_REAR_ENABLE))
  {
    pContext->cardoid.isRearBfNeeded = 1U;
  }
  else
  {
    pContext->cardoid.isRearBfNeeded = 0U;
  }
}

static uint8_t s_isFrameReady(context_t *const pContext)
{
  uint8_t ret = 0;
  pContext->frameReadyCnt++;
  if (pContext->frameReadyCnt == 8U)
  {
    pContext->frameReadyCnt = 0;
    ret = 1;
  }
  return ret;
}

static uint32_t s_setEndianess(context_t *const pContext, uint32_t hwIP)
{
  uint32_t ret = ACOUSTIC_BF_TYPE_ERROR_NONE;
  pdm2pcm_instances_t *pPdmFilter = pContext->pPdmFilter;
  if ((hwIP == ACOUSTIC_BF_CARDOID_PDM_IP_SPI_I2S) ||
      (hwIP == ACOUSTIC_BF_CARDOID_PDM_IP_SAI_PDM) ||
      (hwIP == ACOUSTIC_BF_CARDOID_PDM_IP_SAI_I2S))
  {
    pContext->hwIP = (uint8_t)hwIP;
    if ((hwIP == ACOUSTIC_BF_CARDOID_PDM_IP_SPI_I2S)
        && (pPdmFilter->libBitOrder == PDM2PCM_BIT_ORDER_MSB))
    {
      pPdmFilter->libEndianness = PDM2PCM_ENDIANNESS_BE;
    }
    else /* ACOUSTIC_BF_CARDOID_PDM_IP_SAI_PDM */
    {
      pPdmFilter->libEndianness = PDM2PCM_ENDIANNESS_LE;
    }
  }
  else
  {
    ret = ACOUSTIC_BF_PROCESSING_ERROR;
  }
  return ret;
}

static void s_storeCardoidsFrontRear(context_t *const pContext, int16_t *pPcmBeamFront, int16_t *pPcmBeamRear, int16_t *pPcmM1,  int16_t *pOut)
{
  int offset = (int) pContext->ptr_out_channels;
  for (int i = 0; i < 16; i++)
  {
    /* Input */
    pContext->pPcmM18ms[pContext->cntSamples8ms] = pPcmM1[i];
    pContext->pPcmBeamFront8ms[pContext->cntSamples8ms] = pPcmBeamFront[i];
    pContext->pPcmBeamRear8ms[pContext->cntSamples8ms] = pPcmBeamRear[i];
    pContext->cntSamples8ms++;
    if (pContext->cntSamples8ms == BUFF_8MS_NB_SPLES)
    {
      pContext->bufferState = 1U;
    }
    else if (pContext->cntSamples8ms == (BUFF_8MS_NB_SPLES * 2U))
    {
      pContext->bufferState = 2U;
      pContext->cntSamples8ms = 0;
    }
    else
    {
      /* other values for Samples Count are not supported */
    }
    /* Output */
    pOut[(i * offset)] = pContext->pOut8ms[pContext->cntSamples1ms];
    if (pContext->ref_select == ACOUSTIC_BF_CARDOID_REF_RAW_MICROPHONE)
    {
      pOut[(i * offset) + 1] = pPcmM1[i];
    }
    if (pContext->ref_select == ACOUSTIC_BF_CARDOID_REF_OPPOSITE_ANTENNA)
    {
      pOut[(i * offset) + 1] = pPcmBeamRear[i];
    }

    pContext->cntSamples1ms++;
    if (pContext->cntSamples1ms == (BUFF_8MS_NB_SPLES * 2U))
    {
      pContext->cntSamples1ms = 0;
    }
  }
}


static void s_storeCardoidsFront(context_t *const pContext, int16_t *pPcmBeamFront, int16_t *pPcmM1,  int16_t *pOut)
{
  int offset = (int) pContext->ptr_out_channels;
  for (int i = 0; i < 16; i++)
  {
    /* Input */
    pContext->pPcmM18ms[pContext->cntSamples8ms] = pPcmM1[i];
    pContext->pPcmBeamFront8ms[pContext->cntSamples8ms] = pPcmBeamFront[i];
    pContext->cntSamples8ms++;
    if (pContext->cntSamples8ms == BUFF_8MS_NB_SPLES)
    {
      pContext->bufferState = 1;
    }
    else if (pContext->cntSamples8ms == (BUFF_8MS_NB_SPLES * 2U))
    {
      pContext->bufferState = 2;
      pContext->cntSamples8ms = 0;
    }
    else
    {
      /* other values for Samples Count are not supported */
    }
    /* Output */
    pOut[(i * offset)] = pContext->pOut8ms[pContext->cntSamples1ms];
    if (pContext->ref_select == ACOUSTIC_BF_CARDOID_REF_RAW_MICROPHONE)
    {
      pOut[(i * offset) + 1] = pPcmM1[i];
    }

    pContext->cntSamples1ms++;
    if (pContext->cntSamples1ms == (BUFF_8MS_NB_SPLES * 2U))
    {
      pContext->cntSamples1ms = 0;
    }
  }
}


static uint16_t s_getPdmDecRatio(uint32_t sampling_frequency)
{
  uint16_t decRatio = 0U;
  switch (sampling_frequency / 16UL)
  {
    case 16UL:
      decRatio = PDM2PCM_DEC_FACTOR_16;
      break;
    case 32UL:
      decRatio = PDM2PCM_DEC_FACTOR_32;
      break;
    case 48UL:
      decRatio = PDM2PCM_DEC_FACTOR_48;
      break;
    case 64UL:
      decRatio = PDM2PCM_DEC_FACTOR_64;
      break;
    case 80UL:
      decRatio = PDM2PCM_DEC_FACTOR_80;
      break;
    case 128UL:
      decRatio = PDM2PCM_DEC_FACTOR_128;
      break;
    default:
      break;
  }
  return decRatio;
}

//void s_delay_one_pcm(uint16_t *pDest, uint16_t *pSrc, size_t sizeBytes, uint16_t idLast)
//{
//  pDest [0]      = pDest[idLast];
//  memcpy(&pDest[1], pSrc, sizeBytes);
//}

static uint8_t s_firstStepPdmDelayPdmFrontRear(context_t *const pContext, void *pM1, void *pM2, int16_t *pOut)
{
  uint8_t  *pPdmM1        = (uint8_t *)pM1;
  uint8_t  *pPdmM2        = (uint8_t *)pM2;
  uint16_t *pPcmM1        = pContext->pPcmM1;
  uint16_t *pPcmM1Delayed = pContext->pPcmM1Delayed;
  uint16_t *pPcmM2        = pContext->pPcmM2;
  uint16_t *pPcmM2Delayed = pContext->pPcmM2Delayed;
  uint8_t  *pPdmDelayed   = pContext->delay.pPdmBuff;
  uint16_t *pPcmBeamFront = pContext->pPcmBeamFront;
  uint16_t *pPcmBeamRear  = pContext->pPcmBeamRear;

  pdm2pcm_instances_t *const pPdmFilter = pContext->pPdmFilter;

  /* Front cardoid */
  pContext->Callbacks.DelayPdm(pContext->delay.pHdleM2, pM2, pPdmDelayed, pContext->sampling_frequency);
  PDM2PCM_process(pPdmFilter->m1.pHdle,        pPdmM1,      pPcmM1);
  PDM2PCM_process(pPdmFilter->m2Delayed.pHdle, pPdmDelayed, pPcmM2Delayed);

  if (pContext->M2_gain == 0.0f)
  {
    Cardoid_updateGain(&pContext->cardoid.hdle, (int16_t *) pPcmM1, (int16_t *) pPcmM2Delayed);
  }
  Cardoid_runFront(&pContext->cardoid.hdle, pPcmM1, pPcmM2Delayed, pPcmBeamFront, BUFF_1MS_NB_SPLES);

  /* Rear cardoid */
  pContext->Callbacks.DelayPdm(pContext->delay.pHdleM1, pM1, pPdmDelayed, pContext->sampling_frequency);
  PDM2PCM_process(pPdmFilter->m2.pHdle,        pPdmM2,      pPcmM2);
  PDM2PCM_process(pPdmFilter->m1Delayed.pHdle, pPdmDelayed, pPcmM1Delayed);

  Cardoid_runRear(&pContext->cardoid.hdle, pPcmM2, pPcmM1Delayed, pPcmBeamRear, BUFF_1MS_NB_SPLES);

  s_storeCardoidsFrontRear(pContext, (int16_t *)pPcmBeamFront, (int16_t *)pPcmBeamRear, (int16_t *)pPcmM1, pOut);

  return s_isFrameReady(pContext);
}

static uint8_t s_firstStepPdmDelayPdmFront(context_t *const pContext, void *pM1, void *pM2, int16_t *pOut)
{
  uint8_t  *pPdmM1        = (uint8_t *)pM1;
  uint16_t *pPcmM1        = pContext->pPcmM1;
  uint16_t *pPcmM2Delayed = pContext->pPcmM2Delayed;
  uint8_t  *pPdmDelayed   = pContext->delay.pPdmBuff;
  uint16_t *pPcmBeamFront = pContext->pPcmBeamFront;

  pdm2pcm_instances_t *const pPdmFilter = pContext->pPdmFilter;

  /* Front cardoid */

  pContext->Callbacks.DelayPdm(pContext->delay.pHdleM2, pM2, pPdmDelayed, pContext->sampling_frequency);
  PDM2PCM_process(pPdmFilter->m1.pHdle,        pPdmM1,      pPcmM1);
  PDM2PCM_process(pPdmFilter->m2Delayed.pHdle, pPdmDelayed, pPcmM2Delayed);

  if (pContext->M2_gain == 0.0f)
  {
    Cardoid_updateGain(&pContext->cardoid.hdle, (int16_t *) pPcmM1, (int16_t *) pPcmM2Delayed);
  }
  Cardoid_runFront(&pContext->cardoid.hdle, pPcmM1, pPcmM2Delayed, pPcmBeamFront, BUFF_1MS_NB_SPLES);
  s_storeCardoidsFront(pContext, (int16_t *)pPcmBeamFront, (int16_t *)pPcmM1, pOut);

  return s_isFrameReady(pContext);
}

static uint8_t s_firstStepPdmDelayPcmFrontRear(context_t *const pContext, void *pM1, void *pM2, int16_t *pOut)
{
  uint8_t  *pPdmM1        = (uint8_t *)pM1;
  uint8_t  *pPdmM2        = (uint8_t *)pM2;
  uint16_t *pPcmM1        = pContext->pPcmM1;
  uint16_t *pPcmM2        = pContext->pPcmM2;

  pdm2pcm_instances_t *const pPdmFilter = pContext->pPdmFilter;

  /* Front cardoid */
  PDM2PCM_process(pPdmFilter->m1.pHdle, pPdmM1, (uint16_t *)pPcmM1);
  PDM2PCM_process(pPdmFilter->m2.pHdle, pPdmM2, (uint16_t *)pPcmM2);

  return s_runPcmDelayedFrontRear(pContext, pPcmM1, pPcmM2, pOut);
}

static uint8_t s_firstStepPdmDelayPcmFront(context_t *const pContext, void *pM1, void *pM2, int16_t *pOut)
{
  uint8_t  *pPdmM1        = (uint8_t *)pM1;
  uint8_t  *pPdmM2        = (uint8_t *)pM2;
  uint16_t *pPcmM1        = pContext->pPcmM1;
  uint16_t *pPcmM2        = pContext->pPcmM2;

  pdm2pcm_instances_t *const pPdmFilter = pContext->pPdmFilter;

  /* Front cardoid */
  PDM2PCM_process(pPdmFilter->m1.pHdle, pPdmM1, (uint16_t *)pPcmM1);
  PDM2PCM_process(pPdmFilter->m2.pHdle, pPdmM2, (uint16_t *)pPcmM2);

  return s_runPcmDelayedFront(pContext, pPcmM1, pPcmM2, pOut);
}


static uint8_t s_firstStepPcmDelayedFrontRear(context_t *const pContext, void *pM1, void *pM2, int16_t *pOut)
{
  uint16_t i;
  uint16_t *pM1_u16       = (uint16_t *)pM1;
  uint16_t *pM2_u16       = (uint16_t *)pM2;
  uint16_t *pPcmM1        = pContext->pPcmM1;
  uint16_t *pPcmM2        = pContext->pPcmM2;
  uint8_t   nbCh1         = pContext->ptr_M1_channels;
  uint8_t   nbCh2         = pContext->ptr_M2_channels;

  /* Deinterleave input data into contextual buffers */
  for (i = 0U; i < BUFF_1MS_NB_SPLES; i++)
  {
    pPcmM1[i] = pM1_u16[i * nbCh1];
    pPcmM2[i] = pM2_u16[i * nbCh2];
  }
  return s_runPcmDelayedFrontRear(pContext, pPcmM1, pPcmM2, pOut);
}


static uint8_t s_firstStepPcmDelayedFront(context_t *const pContext, void *pM1, void *pM2, int16_t *pOut)
{
  uint16_t i;
  uint16_t *pM1_u16       = (uint16_t *)pM1;
  uint16_t *pM2_u16       = (uint16_t *)pM2;
  uint16_t *pPcmM1        = pContext->pPcmM1;
  uint16_t *pPcmM2        = pContext->pPcmM2;
  uint8_t   nbCh1         = pContext->ptr_M1_channels;
  uint8_t   nbCh2         = pContext->ptr_M2_channels;

  /* Deinterleave input data into contextual buffers */
  for (i = 0U; i < BUFF_1MS_NB_SPLES; i++)
  {
    pPcmM1[i] = pM1_u16[i * nbCh1];
    pPcmM2[i] = pM2_u16[i * nbCh2];
  }
  return s_runPcmDelayedFront(pContext, pPcmM1, pPcmM2, pOut);
}

static uint8_t s_firstStepPcmNoDelayFrontRear(context_t *const pContext, void *pM1, void *pM2, int16_t *pOut)
{
  uint16_t i;
  uint16_t *pPcmM1        = pContext->pPcmM1;
  uint16_t *pPcmM2        = pContext->pPcmM2;
  uint16_t *pPcmBeamFront = pContext->pPcmBeamFront;
  uint16_t *pPcmBeamRear  = pContext->pPcmBeamRear;

  /* get microphone data & copy in context */
  for (i = 0U; i < BUFF_1MS_NB_SPLES; i++)
  {
    pPcmM1[i]        = ((uint16_t *)pM1)[i * pContext->ptr_M1_channels];
    pPcmM2[i]        = ((uint16_t *)pM2)[i * pContext->ptr_M2_channels];
  }

  /* Front cardoid */
  if (pContext->M2_gain == 0.0f)
  {
    Cardoid_updateGain(&pContext->cardoid.hdle, pPcmM1, pPcmM2);
  }
  Cardoid_runFront(&pContext->cardoid.hdle, pPcmM1, pPcmM2, pPcmBeamFront, BUFF_1MS_NB_SPLES);

  /* Rear cardoid */
  Cardoid_runRear(&pContext->cardoid.hdle, pPcmM2, pPcmM1, pPcmBeamRear, BUFF_1MS_NB_SPLES);
  s_storeCardoidsFrontRear(pContext, (int16_t *)pPcmBeamFront, (int16_t *)pPcmBeamRear, (int16_t *)pPcmM1, pOut);

  return s_isFrameReady(pContext);
}

static uint8_t s_firstStepPcmNoDelayFront(context_t *const pContext, void *pM1, void *pM2, int16_t *pOut)
{
  uint16_t i;
  uint16_t *pPcmM1        = pContext->pPcmM1;
  uint16_t *pPcmM2        = pContext->pPcmM2;
  uint16_t *pPcmBeamFront = pContext->pPcmBeamFront;

  /* get microphone data & copy in context */
  for (i = 0U; i < BUFF_1MS_NB_SPLES; i++)
  {
    pPcmM1[i]        = ((uint16_t *)pM1)[i * pContext->ptr_M1_channels];
    pPcmM2[i]        = ((uint16_t *)pM2)[i * pContext->ptr_M2_channels];
  }

  /* Front cardoid */
  if (pContext->M2_gain == 0.0f)
  {
    Cardoid_updateGain(&pContext->cardoid.hdle, (int16_t *) pPcmM1, (int16_t *) pPcmM2);
  }
  Cardoid_runFront(&pContext->cardoid.hdle, pPcmM1, pPcmM2, pPcmBeamFront, BUFF_1MS_NB_SPLES);
  s_storeCardoidsFront(pContext, (int16_t *)pPcmBeamFront, (int16_t *)pPcmM1, pOut);

  return s_isFrameReady(pContext);
}

static uint8_t s_runPcmDelayedFrontRear(context_t *const pContext, uint16_t *pPcmM1, uint16_t *pPcmM2, int16_t *pOut)
{
  uint16_t *pPcmM1Delayed = pContext->pPcmM1Delayed;
  uint16_t *pPcmM2Delayed = pContext->pPcmM2Delayed;
  uint16_t *pPcmBeamFront = pContext->pPcmBeamFront;
  uint16_t *pPcmBeamRear  = pContext->pPcmBeamRear;

  /* Front cardoid */
  Delay_one_pcm(pContext->delay.pHdleM2, pPcmM2Delayed, pPcmM2);
  if (pContext->M2_gain == 0.0f)
  {
    Cardoid_updateGain(&pContext->cardoid.hdle, (int16_t *) pPcmM1, (int16_t *) pPcmM2Delayed);
  }
  Cardoid_runFront(&pContext->cardoid.hdle, pPcmM1, pPcmM2Delayed, pPcmBeamFront, BUFF_1MS_NB_SPLES);

  /* Rear cardoid */
  Delay_one_pcm(pContext->delay.pHdleM1, pPcmM1Delayed, pPcmM1);
  Cardoid_runRear(&pContext->cardoid.hdle, pPcmM2, pPcmM1Delayed, pPcmBeamRear, BUFF_1MS_NB_SPLES);
  s_storeCardoidsFrontRear(pContext, (int16_t *)pPcmBeamFront, (int16_t *)pPcmBeamRear, (int16_t *)pPcmM1, pOut);

  return s_isFrameReady(pContext);
}

static uint8_t s_runPcmDelayedFront(context_t *const pContext, uint16_t *pPcmM1, uint16_t *pPcmM2, int16_t *pOut)
{
  uint16_t *pPcmM2Delayed = pContext->pPcmM2Delayed;
  uint16_t *pPcmBeamFront = pContext->pPcmBeamFront;

  /* Front cardoid */
  Delay_one_pcm(pContext->delay.pHdleM2, pPcmM2Delayed, pPcmM2);
  if (pContext->M2_gain == 0.0f)
  {
    Cardoid_updateGain(&pContext->cardoid.hdle, (int16_t *) pPcmM1, (int16_t *) pPcmM2Delayed);
  }
  Cardoid_runFront(&pContext->cardoid.hdle, pPcmM1, pPcmM2Delayed, pPcmBeamFront, BUFF_1MS_NB_SPLES);

  s_storeCardoidsFront(pContext, (int16_t *)pPcmBeamFront, (int16_t *)pPcmM1, pOut);

  return s_isFrameReady(pContext);
}

#endif  /*__ACOUSTIC_BF_CARDOID_C*/

