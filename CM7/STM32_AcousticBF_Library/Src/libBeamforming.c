/**
******************************************************************************
* @file    libBeamforming.c
* @author  SRA
* @brief   Beamforming core library instanciate cardoid processing & adaptive
*          & denoiser filtering
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
#ifndef __LIB_BEAMFORMING_C
#define __LIB_BEAMFORMING_C

/* Includes ------------------------------------------------------------------*/
#include "acoustic_bf.h"
#include "acoustic_bf_speex.h"
#include <stdio.h>
#include <string.h>
/*cstat -MISRAC2012-* CMSIS not misra compliant */
#include <arm_math.h>
/*cstat +MISRAC2012-* */

/* Private typedef -----------------------------------------------------------*/

typedef struct
{
  float alpha;         // for smoothing
  float lin;           // pow^2 (not normalized)
  float dB;
} energy_t;


typedef struct
{
  uint8_t   enable;
  float     gain;   /* Gain applied to AIc processing and (1- gain) is applied to microphone) */
  float     tLow;   /* Threshold */
  float     tHigh;  /* Threshold */
  size_t    micBuffSzBytes;
  size_t    delaySizeBytes;
  energy_t  hEnergy;
  int16_t *pMicDelayed;
} context_mixer_t;

typedef struct
{
  AcousticBF_cardoid_Handler_t hdle;
  //  AcousticBF_cardoid_Config_t  conf;
} context_cardoid_t;

typedef struct
{
  uint8_t            isAdaptiveUsed;  // to avoid multiple test, this set at init & config stage
  uint8_t            isDenoiserUsed;  // to avoid multiple test, this set at init & config stage
  AcousticBF_speex_t *pHdle;
} context_speex_t;

typedef struct
{
  uint8_t            algorithm_type_init;
  context_cardoid_t  cardoid;
  context_speex_t    speex;
  context_mixer_t   *pMixer;
} context_t;

/* Private defines -----------------------------------------------------------*/
#define BUFF_8MS_NB_SPLES           128U /* 8ms * 16 samples per ms  */
#define BUFF_DELAY_NB_SPLES         (1U*BUFF_8MS_NB_SPLES) /* Delay of overall processing is 3 frames */

/* Private macros ------------------------------------------------------------*/

#define SIZEOF_ALIGN ACOUSTIC_BF_SIZEOF_ALIGN
/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static void     s_setDenoiser(context_t  *const pContext);
static void     s_setAdaptive(context_t  *const pContext);
static uint32_t s_runPostProc(void *const pHdle, int16_t *const pMic, int16_t *const pFront, int16_t *const pRear, int16_t *const pOut);
static uint32_t s_storeUserConf(context_t *const pContext, AcousticBF_Handler_t *const pAcousticBfHdle);
static uint32_t s_initSpeex(AcousticBF_Handler_t *pHandler);
static void s_energy_init(energy_t    *const pHdle, uint32_t const fsHz, uint16_t const smoothingTimeInMs, uint32_t const nbSamples);
static void s_energy_process(energy_t *const pHdle, int16_t const *const pData, uint16_t const nbSamples);
static void s_mixer_init(context_mixer_t         *const pHdle, uint8_t enable, float tLowDb, float tHighDb, uint32_t fs, uint16_t nbSamples, uint16_t smoothMs);
static void s_mixer_process_gain(context_mixer_t *const pHdle,  void *const pMic, uint16_t const nbSamples);
static void s_mixer_process(context_mixer_t      *const pHdle,  int16_t *const pMic, int16_t *const pAfe, int16_t *const pOut, uint16_t const nbSamples);
static inline float s_int16ToFloat(int16_t const x);
static inline int16_t s_floatToInt16(float const x);


/* Functions Definition ------------------------------------------------------*/

static uint32_t libBeamforming_Init(AcousticBF_Handler_t *pHandler)
{
  context_t *const pContext    = (context_t *)(pHandler->pInternalMemory);
  uint32_t         byte_offset = SIZEOF_ALIGN(context_t);
  uint32_t         ret         = ACOUSTIC_BF_TYPE_ERROR_NONE;
  uint8_t          type        = pHandler->algorithm_type_init;

  if (pHandler->mixer_enable == ACOUSTIC_BF_MIXER_ENABLE)
  {
    ret = ACOUSTIC_BF_MIXER_ERROR;
  }

  if ((ret == ACOUSTIC_BF_TYPE_ERROR_NONE) && (pContext == NULL))
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
    pContext->algorithm_type_init = type;
    AcousticBF_cardoid_Handler_t *pAcousticBfCardoidHdle = &pContext->cardoid.hdle;
    pAcousticBfCardoidHdle->data_format        = pHandler->data_format;
    pAcousticBfCardoidHdle->sampling_frequency = pHandler->sampling_frequency;
    pAcousticBfCardoidHdle->ptr_M1_channels    = pHandler->ptr_M1_channels;
    pAcousticBfCardoidHdle->ptr_M2_channels    = pHandler->ptr_M2_channels;
    pAcousticBfCardoidHdle->ptr_out_channels   = pHandler->ptr_out_channels;
    pAcousticBfCardoidHdle->delay_enable       = pHandler->delay_enable;

    if ((type == ACOUSTIC_BF_TYPE_STRONG) || (type == ACOUSTIC_BF_TYPE_ASR_READY) ||
        (pHandler->ref_mic_enable == ACOUSTIC_BF_REF_OPPOSITE_ANTENNA))
    {
      pAcousticBfCardoidHdle->rear_enable = (uint8_t)ACOUSTIC_BF_CARDOID_REAR_ENABLE;
    }
    else
    {
      pAcousticBfCardoidHdle->rear_enable = (uint8_t)ACOUSTIC_BF_CARDOID_REAR_DISABLE;
    }


    AcousticBF_cardoid_GetMemorySize(pAcousticBfCardoidHdle);
    pAcousticBfCardoidHdle->pInternalMemory = (uint32_t *)((uint8_t *)pHandler->pInternalMemory + byte_offset);
    byte_offset += pAcousticBfCardoidHdle->internal_memory_size;

    ret = AcousticBF_cardoid_Init(pAcousticBfCardoidHdle);
  }

  if (ret == ACOUSTIC_BF_TYPE_ERROR_NONE)
  {
    if ((pContext->speex.isDenoiserUsed == 1U) || (pContext->speex.isAdaptiveUsed == 1U))
    {
      pContext->speex.pHdle = (AcousticBF_speex_t *)((uint8_t *)pHandler->pInternalMemory + byte_offset);
      byte_offset += SIZEOF_ALIGN(AcousticBF_speex_t);

      pContext->speex.pHdle->pInternalMemory = (uint32_t *)((uint8_t *)pHandler->pInternalMemory + byte_offset);
      pContext->speex.pHdle->denoise_enable = pContext->speex.isDenoiserUsed;
      pContext->speex.pHdle->adaptive_enable = pContext->speex.isAdaptiveUsed;
      AcousticBF_speex_GetMemorySize(pContext->speex.pHdle);
      byte_offset += pContext->speex.pHdle->internal_memory_size;
      ret = s_initSpeex(pHandler);
    }
  }
  if (pHandler->mixer_enable == ACOUSTIC_BF_MIXER_ENABLE)
  {
    pContext->pMixer = (context_mixer_t *)((uint8_t *)pHandler->pInternalMemory + byte_offset);
    byte_offset +=  SIZEOF_ALIGN(context_mixer_t);
    pContext->pMixer->pMicDelayed = (int16_t *)((uint8_t *)pHandler->pInternalMemory + byte_offset);
    byte_offset += (BUFF_DELAY_NB_SPLES + BUFF_8MS_NB_SPLES) * sizeof(int16_t);
    pContext->pMixer->enable = pHandler->mixer_enable;

    s_mixer_init(pContext->pMixer, pHandler->mixer_enable, pHandler->thresh_low_db, pHandler->thresh_high_db, 16000UL,  BUFF_8MS_NB_SPLES, 300U);
  }

  if (ret == ACOUSTIC_BF_TYPE_ERROR_NONE)
  {
    if (pContext->algorithm_type_init == ACOUSTIC_BF_TYPE_STRONG)
    {
      AcousticBF_speex_SetupDenoiser(pContext->speex.pHdle);
    }
    while ((++byte_offset % 4U) != 0U)
    {
    }
    if (byte_offset != pHandler->internal_memory_size)
    {
      ret = ACOUSTIC_BF_ALLOCATION_ERROR;
    }

  }

  return ret;
}




static uint32_t libBeamforming_getMemorySize(AcousticBF_Handler_t *pHandler)
{
  AcousticBF_cardoid_Handler_t cardoidHandler;
  uint32_t                     byte_offset    = SIZEOF_ALIGN(context_t);
  uint8_t                      type           = pHandler->algorithm_type_init;

  cardoidHandler.data_format        = pHandler->data_format;
  cardoidHandler.sampling_frequency = pHandler->sampling_frequency;
  cardoidHandler.ptr_M1_channels    = pHandler->ptr_M1_channels;
  cardoidHandler.ptr_M2_channels    = pHandler->ptr_M2_channels;
  cardoidHandler.ptr_out_channels   = pHandler->ptr_out_channels;
  cardoidHandler.delay_enable       = pHandler->delay_enable;

  if ((pHandler->algorithm_type_init == ACOUSTIC_BF_TYPE_STRONG) ||
      (pHandler->algorithm_type_init == ACOUSTIC_BF_TYPE_ASR_READY) ||
      (pHandler->ref_mic_enable      == ACOUSTIC_BF_REF_OPPOSITE_ANTENNA))
  {
    cardoidHandler.rear_enable = (uint8_t)ACOUSTIC_BF_CARDOID_REAR_ENABLE;
  }
  else
  {
    cardoidHandler.rear_enable = (uint8_t)ACOUSTIC_BF_CARDOID_REAR_DISABLE;
  }

  AcousticBF_cardoid_GetMemorySize(&cardoidHandler);
  byte_offset += cardoidHandler.internal_memory_size;

  if (pHandler->algorithm_type_init > ACOUSTIC_BF_TYPE_CARDIOID_BASIC)
  {
    AcousticBF_speex_t speexHandler;
    byte_offset += SIZEOF_ALIGN(AcousticBF_speex_t);

    speexHandler.denoise_enable  = (((type == ACOUSTIC_BF_TYPE_STRONG) || (type == ACOUSTIC_BF_TYPE_CARDIOID_DENOISE))) ? 1U : 0U;
    speexHandler.adaptive_enable = (((type == ACOUSTIC_BF_TYPE_STRONG) || (type == ACOUSTIC_BF_TYPE_ASR_READY)))        ? 1U : 0U;
    AcousticBF_speex_GetMemorySize(&speexHandler);
    byte_offset += speexHandler.internal_memory_size;
  }

  if (pHandler->mixer_enable == ACOUSTIC_BF_MIXER_ENABLE)
  {
    byte_offset +=  SIZEOF_ALIGN(context_mixer_t);
    byte_offset += (BUFF_DELAY_NB_SPLES + BUFF_8MS_NB_SPLES) * sizeof(int16_t);
  }
  while ((++byte_offset % 4U) != 0U)
  {
  }
  pHandler->internal_memory_size = byte_offset;
  return ACOUSTIC_BF_TYPE_ERROR_NONE;
}

static uint32_t libBeamforming_FirstStep(void *pM1, void *pM2, void *ptr_Out, AcousticBF_Handler_t *pHandler)
{
  context_t *const pContext = (context_t *)(pHandler->pInternalMemory);
  return AcousticBF_cardoid_FirstStep(pM1, pM2, ptr_Out, &pContext->cardoid.hdle);
}

static uint32_t libBeamforming_SecondStep(AcousticBF_Handler_t *pHandler)
{
  context_t *const pContext = (context_t *)(pHandler->pInternalMemory);
  return AcousticBF_cardoid_SecondStep(&pContext->cardoid.hdle);
}

static uint32_t libBeamforming_setConfig(AcousticBF_Handler_t *pHandler, AcousticBF_Config_t *pConfig)
{
  context_t *const pContext = (context_t *)(pHandler->pInternalMemory);
  uint32_t ret = ACOUSTIC_BF_TYPE_ERROR_NONE;

  if (pContext == NULL)
  {
    ret = ACOUSTIC_BF_ALLOCATION_ERROR;
  }
  else
  {
    uint8_t                     type = pHandler->algorithm_type_init;
    AcousticBF_cardoid_Config_t cardoid_conf;
    cardoid_conf.mic_distance = pConfig->mic_distance;
    cardoid_conf.volume       = pConfig->volume;
    cardoid_conf.M2_gain      = pConfig->M2_gain;
    cardoid_conf.ref_select   = (uint8_t)pHandler->ref_mic_enable;

    ret = AcousticBF_cardoid_SetConfig(&pContext->cardoid.hdle, &cardoid_conf);

    if (ret == ACOUSTIC_BF_TYPE_ERROR_NONE)
    {
      if (pContext->algorithm_type_init != pConfig->algorithm_type)
      {
        if ((pContext->algorithm_type_init == ACOUSTIC_BF_TYPE_CARDIOID_BASIC) && (pConfig->algorithm_type > ACOUSTIC_BF_TYPE_CARDIOID_BASIC))
        {
          pContext->algorithm_type_init = ACOUSTIC_BF_TYPE_CARDIOID_BASIC;
          ret |= ACOUSTIC_BF_ALLOCATION_ERROR;
        }
        else
        {
          /* Reject adaptive requtes if memory was not allocated at init*/
          if ((pContext->speex.isAdaptiveUsed == 0U) &&
              ((type == ACOUSTIC_BF_TYPE_STRONG) || (type == ACOUSTIC_BF_TYPE_ASR_READY)))
          {
            pContext->algorithm_type_init = ACOUSTIC_BF_TYPE_CARDIOID_BASIC;
            ret |= ACOUSTIC_BF_ALLOCATION_ERROR;
          }
          /* Reject denoising requtes if memory was not allocated at init*/
          if ((pContext->speex.isDenoiserUsed == 0U) &&
              ((type == ACOUSTIC_BF_TYPE_STRONG) || (type == ACOUSTIC_BF_TYPE_CARDIOID_DENOISE)))
          {
            pContext->algorithm_type_init = ACOUSTIC_BF_TYPE_CARDIOID_BASIC;
            ret |= ACOUSTIC_BF_ALLOCATION_ERROR;
          }
        }
      }
    }

    if (ret == ACOUSTIC_BF_TYPE_ERROR_NONE)
    {
      /* Memory at init was sufficient for new request => apply new configuration */
      pContext->algorithm_type_init = (uint8_t)pConfig->algorithm_type;

      s_setDenoiser(pContext);
      s_setAdaptive(pContext);

      if ((pContext->speex.isDenoiserUsed == 1U) || (pContext->speex.isAdaptiveUsed == 1U))
      {
        AcousticBF_cardoid_postProc_t cardoid_post_proc;
        cardoid_post_proc.pHdle = (void *) pContext;
        cardoid_post_proc.pCb = s_runPostProc;
        ret = AcousticBF_cardoid_RegisterPostProc(&pContext->cardoid.hdle, &cardoid_post_proc);
      }
    }
  }
  return ret;
}


static uint32_t libBeamforming_setHWIP(AcousticBF_Handler_t *pHandler, uint32_t hwIP)
{
  context_t *const pContext = (context_t *)(pHandler->pInternalMemory);
  uint32_t ret = ACOUSTIC_BF_TYPE_ERROR_NONE;

  if (pContext == NULL)
  {
    ret = ACOUSTIC_BF_ALLOCATION_ERROR;
  }
  else
  {
    ret = AcousticBF_cardoid_SetHWIP(&pContext->cardoid.hdle, hwIP);

  }
  return ret;
}

static uint32_t libBeamforming_getConfig(AcousticBF_Handler_t *pHandler, AcousticBF_Config_t *pConfig)
{
  uint32_t ret = ACOUSTIC_BF_TYPE_ERROR_NONE;
  context_t *const pContext = (context_t *)(pHandler->pInternalMemory);

  if ((pContext == NULL) || (pConfig == NULL))
  {
    ret = ACOUSTIC_BF_ALLOCATION_ERROR;
  }
  else
  {

    AcousticBF_cardoid_Config_t cardoid_config;
    AcousticBF_cardoid_GetConfig(&pContext->cardoid.hdle, &cardoid_config);

    pConfig->mic_distance   = cardoid_config.mic_distance;
    pConfig->algorithm_type = pContext->algorithm_type_init;
    pConfig->volume         = cardoid_config.volume;
    pConfig->M2_gain        = cardoid_config.M2_gain;
  }
  return ret;
}

static uint32_t libBeamforming_getControl(AcousticBF_Handler_t *pHandler, AcousticBF_Control_t *pControl)
{
  uint32_t ret = ACOUSTIC_BF_TYPE_ERROR_NONE;
  context_t *const pContext = (context_t *)(pHandler->pInternalMemory);

  if ((pContext == NULL) || (pControl == NULL))
  {
    ret = ACOUSTIC_BF_ALLOCATION_ERROR;
  }
  else
  {
    context_mixer_t *const pMixer = pContext->pMixer;
    if (pMixer == NULL)
    {
      ret = ACOUSTIC_BF_ALLOCATION_ERROR;
    }
    else  if (pMixer->enable == ACOUSTIC_BF_MIXER_ENABLE)

    {
      pControl->energy_mic_db = pMixer->hEnergy.dB;
    }
  }
  return ret;
}

/* Static private functions */
static uint32_t s_storeUserConf(context_t *const pContext, AcousticBF_Handler_t *const pAcousticBfHdle)
{
  uint32_t ret = ACOUSTIC_BF_TYPE_ERROR_NONE;
  /*SUPPORT VARIABLE USED FOR MEMORY ALLOCATION*/
  if (pAcousticBfHdle->algorithm_type_init <= ACOUSTIC_BF_TYPE_STRONG)
  {
    pContext->algorithm_type_init = pAcousticBfHdle->algorithm_type_init;
  }
  else
  {
    pContext->algorithm_type_init = ACOUSTIC_BF_TYPE_CARDIOID_BASIC;
    ret |= ACOUSTIC_BF_TYPE_ERROR;
  }

  s_setDenoiser(pContext);
  s_setAdaptive(pContext);
  return ret;
}


static uint32_t s_runPostProc(void *const pHdle, int16_t *const pMic, int16_t *const pFront, int16_t *const pRear, int16_t *const pOut)
{
  context_t       *const pContext   = (context_t *)pHdle;
  context_speex_t *const pSpeexCtxt = (context_speex_t *)&pContext->speex;
  context_mixer_t *const pMixer     = pContext->pMixer;
  if (pSpeexCtxt->isAdaptiveUsed == 1U)
  {
    AcousticBF_speex_RunApadtive(pSpeexCtxt->pHdle, pFront, pRear, pOut);
  }
  if (pSpeexCtxt->isDenoiserUsed == 1U)
  {
    AcousticBF_speex_runDenoiser(pSpeexCtxt->pHdle, pOut);
  }

  if ((pMixer != NULL) && (pMixer->enable == ACOUSTIC_BF_MIXER_ENABLE))
  {
    /* Store mic delayed data  before mix */
    memmove(pMixer->pMicDelayed, &pMixer->pMicDelayed[BUFF_8MS_NB_SPLES], pMixer->delaySizeBytes);
    memcpy(&pMixer->pMicDelayed[BUFF_DELAY_NB_SPLES], pMic, pMixer->micBuffSzBytes);

    s_mixer_process_gain(pMixer, pMixer->pMicDelayed, BUFF_8MS_NB_SPLES);
    s_mixer_process(pMixer, pMixer->pMicDelayed, pOut, pOut, BUFF_8MS_NB_SPLES);
  }
  return ACOUSTIC_BF_TYPE_ERROR_NONE;
}




static void s_setDenoiser(context_t  *const pContext)
{
  uint8_t type = pContext->algorithm_type_init;

  if ((type == ACOUSTIC_BF_TYPE_STRONG) || (type == ACOUSTIC_BF_TYPE_CARDIOID_DENOISE))
  {
    pContext->speex.isDenoiserUsed = 1U;
  }
  else
  {
    pContext->speex.isDenoiserUsed = 0U;
  }
}

static void s_setAdaptive(context_t  *const pContext)
{
  uint8_t type = pContext->algorithm_type_init;

  if ((type == ACOUSTIC_BF_TYPE_STRONG) || (type == ACOUSTIC_BF_TYPE_ASR_READY))
  {
    pContext->speex.isAdaptiveUsed = 1U;
  }
  else
  {
    pContext->speex.isAdaptiveUsed = 0U;
  }
}

static uint32_t s_initSpeex(AcousticBF_Handler_t *pHandler)
{
  uint32_t         ret         = ACOUSTIC_BF_TYPE_ERROR_NONE;
  context_t *const pContext    = (context_t *)(pHandler->pInternalMemory);

  if ((pContext->speex.isDenoiserUsed == 1U) || (pContext->speex.isAdaptiveUsed == 1U))
  {
    AcousticBF_speex_Init(pContext->speex.pHdle);
  }

  if (pContext->algorithm_type_init == ACOUSTIC_BF_TYPE_STRONG)
  {
    AcousticBF_speex_SetupDenoiser(pContext->speex.pHdle);
  }
  return ret;
}


static void s_mixer_init(context_mixer_t *const pHdle, uint8_t enable, float tLowDb, float tHighDb, uint32_t fs, uint16_t nbSamples, uint16_t smoothMs)
{
  pHdle->enable = enable;
  pHdle->tLow   = powf(10.0f, tLowDb / 10.0f);  /*cstat !MISRAC2012-Rule-22.8 no issue with powf(10, ...) => errno check is useless*/
  pHdle->tHigh  = powf(10.0f, tHighDb / 10.0f); /*cstat !MISRAC2012-Rule-22.8 no issue with powf(10, ...) => errno check is useless*/
  pHdle->gain   = 1.0f;                         /* set to max means by default we output only AFE, no omni mic signal */
  pHdle->micBuffSzBytes = BUFF_8MS_NB_SPLES * sizeof(int16_t) ;
  pHdle->delaySizeBytes = BUFF_DELAY_NB_SPLES * sizeof(int16_t) ;

  if (enable == 1U)
  {
    s_energy_init(&pHdle->hEnergy, fs, smoothMs, nbSamples);
  }

}

static void s_mixer_process_gain(context_mixer_t *const pHdle,  void *const pMic, uint16_t const nbSamples)
{
  energy_t  *const pEnergy = &pHdle->hEnergy;

  /* Monitor energy value of omni microphone's out of band signal, to get an idea how noisy is the signal */
  s_energy_process(pEnergy, pMic, nbSamples);

  if (pEnergy->lin >= pHdle->tHigh)
  {
    pHdle->gain = 1.0f;
  }
  else if (pEnergy->lin < pHdle->tLow) /* no = to support case threshold_low = tHigh*/
  {
    pHdle->gain = 0.0f;
  }
  else
  {
    pHdle->gain = (pEnergy->lin - pHdle->tLow) / (pHdle->tHigh - pHdle->tLow);
  }
  // pHdle->gain = 0.0f; // hack to output delayed mic for test, todo remove
}

static void s_mixer_process(context_mixer_t *const pHdle,  int16_t *const pMic, int16_t *const pAfe, int16_t *const pOut, uint16_t const nbSamples)
{
  for (uint16_t i = 0U; i < nbSamples; i++)
  {
    pOut[i] = s_floatToInt16(((1.0f - pHdle->gain) * s_int16ToFloat(pMic[i])) + (pHdle->gain * s_int16ToFloat(pAfe[i])));
  }
}


static void s_energy_init(energy_t *const pHdle, uint32_t const fsHz, uint16_t const smoothingTimeInMs, uint32_t const nbSamples)
{
  pHdle->alpha = 1.0f - expf((-1000.0f * (float)nbSamples) / ((float)fsHz * (float)smoothingTimeInMs)); /* x1000 because smoothing is in ms and fs in Hz*/ /*cstat !MISRAC2012-Rule-22.8 errno check is useless*/
}


static void s_energy_process(energy_t *const pHdle, int16_t const *const pData, uint16_t const nbSamples)
{
  float xn, energy;
  float sum_mag_xn = 0.0f;
  uint32_t div;

  energy = pHdle->lin;

  for (uint16_t i = 0U; i < nbSamples; i++)
  {
    xn          = s_int16ToFloat(pData[i]);
    sum_mag_xn += (xn * xn) ;
  }
  div = (uint32_t)nbSamples ;
  sum_mag_xn /= (float)div;
  energy += pHdle->alpha * (sum_mag_xn - energy);
  pHdle->lin = energy;
  pHdle->dB  = 10.0f * log10f(energy); /* todo: remove when done; for debug while testing */ /*cstat !MISRAC2012-Rule-22.8 !MISRAC2012-Dir-4.11_a energy is > 0*/
}


/**
  * @brief  16 bits fixed-point to floating-point sample conversion
  * @param  x: 16 bits fixed-point input sample
  * @retval floating-point output sample
  */
static inline float s_int16ToFloat(int16_t const x)
{
  return (float)x / 32768.0f;
}


/**
  * @brief  floating-point to 16 bits fixed-point sample conversion
  * @param  x: floating-point input sample
  * @retval 16 bits fixed-point output sample
  */
static inline int16_t s_floatToInt16(float const x)
{
  float const dataFloat = 32768.0f * x;
  int16_t     dataClampedInt16;

  if (dataFloat < -32768.0f)
  {
    dataClampedInt16 = (int16_t)(-32768);
  }
  else if (dataFloat > 32767.0f)
  {
    dataClampedInt16 = 32767;
  }
  else
  {
    dataClampedInt16 = (int16_t)dataFloat;
  }
  return dataClampedInt16;
}
#endif  /*__LIB_BEAMFORMING_C*/

