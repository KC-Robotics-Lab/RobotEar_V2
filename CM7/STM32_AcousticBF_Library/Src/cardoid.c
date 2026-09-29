/**
******************************************************************************
* @file    cardoid.c
* @author  SRA
* @brief   Cardoid core library
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
#ifndef __CARDOID_C_
#define __CARDOID_C_

/* Includes ------------------------------------------------------------------*/
#include "cardoid.h"




/* Private typedef -----------------------------------------------------------*/

typedef struct
{
  int32_t        s1_out_old;
  int32_t        s2_out_old;
  uint16_t       alpha_antifilter;
  uint16_t       gain_antifilter_s1;
  uint16_t       gain_antifilter_s2;
} cardoid_conf_t;

typedef struct
{
  uint32_t       GainSamplesCounter_0;
  float32_t      RMSm1_0;
  float32_t      RMSm2_0;
  float32_t      gain_0;
  float32_t      gain_old_0;
} cardoid_gain_t;

typedef struct
{
  uint8_t        isRearBfNeeded;      // to avoid multiple test, this set at init & config stage
  cardoid_gain_t gain;
} cardoid_ctxt_t;

typedef struct
{
  uint8_t        interleaved;
  uint16_t       mic_distance;
  cardoid_conf_t antennaFront;
  cardoid_conf_t antennaRear;
  cardoid_ctxt_t ctxt;
} cardoid_t;

/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
#define SPEED_OF_SOUND          343.0f
#define MIC_DIST_THRESHOLD_3mm  34U
#define MIC_DIST_THRESHOLD_4mm  40U
#define MIC_DIST_THRESHOLD_5mm  57U
#define MIC_DIST_THRESHOLD_7mm  80U
#define MIC_DIST_THRESHOLD_15mm 150U
#define MIC_DIST_THRESHOLD_21mm 212U
#define GAIN_COMPUTATION_LENGTH 8000U

#define ALIGNED_SIZE            4UL  /*!< alignement size */
#define SIZE_ALIGN(size)        (((size) + ALIGNED_SIZE - 1UL) & (0xFFFFFFFFUL - (ALIGNED_SIZE - 1UL))) /*!< general macro to get alignement size */
#define SIZEOF_ALIGN(object)    SIZE_ALIGN(sizeof(object)) /*!< specific macro to replace all sizeof */


/* Private variables ---------------------------------------------------------*/

/*DMA and ANTIFILTER related*/
/*ALPHA;GAIN*/

static const float32_t coefficients_3mm[]    = {0.85f, 2.4f};
static const float32_t coefficients_4mm[]    = {0.87f, 1.87f};
static const float32_t coefficients_5_65mm[] = {0.6748f, 1.754f};
static const float32_t coefficients_7mm[]    = {0.6f, 1.3f};
static const float32_t coefficients_15mm[]   = {0.9f, 1.05f};
static const float32_t coefficients_21_2mm[] = {0.6f, 1.2f};


/* Private function prototypes -----------------------------------------------*/
static float32_t *s_getCoeff(uint16_t mic_distance);
static void       s_setGain(cardoid_t          *const pContext, float32_t gain);
static void       s_updateGain(cardoid_t       *const pContext, int16_t *pDataS1, int16_t *pDataS2);
static void       s_initBf(cardoid_conf_t            *pConf, float32_t alpha_antifilter, float32_t gain_antifilter_s1, float32_t gain_antifilter_s2);
static void       s_configureBf(cardoid_conf_t       *pConf, float32_t *const pCoeffs, float32_t gain_s1, float32_t gain_s2);
static void       s_runBf(cardoid_conf_t       *const pConf, int16_t *ptrBufferIn1, int16_t *ptrBufferIn2, int16_t *ptrBufferOut, uint32_t nbSamples);


/* Functions Definition ------------------------------------------------------*/

uint32_t Cardoid_init(Cardoid_Handler_t *pHandler)
{
  cardoid_t *const pContext = (cardoid_t *)(pHandler->pInternalMemory);
  uint32_t ret = ACOUSTIC_BF_TYPE_ERROR_NONE;

  if (pContext == NULL)
  {
    ret = ACOUSTIC_BF_ALLOCATION_ERROR;
  }
  else
  {
    (void)memset(pHandler->pInternalMemory, 0, pHandler->internal_memory_size);
    if (pHandler->interleaved == CARDOID_INTERLEAVED_NO)
    {
      pContext->interleaved = pHandler->interleaved;
    }
    else
    {
      pContext->interleaved = CARDOID_INTERLEAVED_NO;
      ret |= ACOUSTIC_BF_TYPE_ERROR;
    }

  }

  /* Initialize internal variables */
  if (ret == ACOUSTIC_BF_TYPE_ERROR_NONE)
  {
    pContext->ctxt.gain.gain_0               = 1.0f;
    pContext->ctxt.gain.gain_old_0           = 1.0f;
  }
  return ret;
}

uint32_t Cardoid_runFront(Cardoid_Handler_t *pHandler, void *pM1, void *pM2, void *ptr_Out, uint32_t nbSamples)
{
  cardoid_t *const pContext = (cardoid_t *)(pHandler->pInternalMemory);
  s_runBf(&pContext->antennaFront, pM1, pM2, ptr_Out, nbSamples);
  return ACOUSTIC_BF_TYPE_ERROR_NONE;
}

uint32_t Cardoid_runRear(Cardoid_Handler_t *pHandler, void *pM1, void *pM2, void *ptr_Out, uint32_t nbSamples)
{
  cardoid_t *const pContext = (cardoid_t *)(pHandler->pInternalMemory);
  s_runBf(&pContext->antennaRear, pM1, pM2, ptr_Out, nbSamples);
  return ACOUSTIC_BF_TYPE_ERROR_NONE;
}

uint32_t Cardoid_setGain(Cardoid_Handler_t *pHandler, float32_t gain)
{
  cardoid_t *const pContext = (cardoid_t *)(pHandler->pInternalMemory);
  s_setGain(pContext, gain);

  return ACOUSTIC_BF_TYPE_ERROR_NONE;
}

uint32_t Cardoid_getGain(Cardoid_Handler_t *pHandler, float32_t *pGain)
{
  cardoid_t *const pContext = (cardoid_t *)(pHandler->pInternalMemory);
  pGain[0] = pContext->ctxt.gain.gain_0;
  return ACOUSTIC_BF_TYPE_ERROR_NONE;
}

uint32_t Cardoid_updateGain(Cardoid_Handler_t *pHandler, void *pM1, void *pM2)
{
  cardoid_t *const pContext = (cardoid_t *)(pHandler->pInternalMemory);
  s_updateGain(pContext, pM1, pM2);

  return ACOUSTIC_BF_TYPE_ERROR_NONE;
}



uint32_t Cardoid_setConfig(Cardoid_Handler_t *pHandler, Cardoid_Config_t *pConfig)
{
  cardoid_t *const pContext = (cardoid_t *)(pHandler->pInternalMemory);
  uint32_t ret = ACOUSTIC_BF_TYPE_ERROR_NONE;

  pContext->mic_distance        = pConfig->mic_distance;
  pContext->ctxt.isRearBfNeeded = pConfig->rear_enable;

  return ret;
}

uint32_t Cardoid_getConfig(Cardoid_Handler_t *pHandler, Cardoid_Config_t *pConfig)
{
  cardoid_t *const pContext = (cardoid_t *)(pHandler->pInternalMemory);
  pConfig->mic_distance = pContext->mic_distance;
  pConfig->rear_enable  = pContext->ctxt.isRearBfNeeded;
  return ACOUSTIC_BF_TYPE_ERROR_NONE;
}

uint32_t Cardoid_getMemorySize(Cardoid_Handler_t *pHandler)
{
  uint32_t  byte_offset = SIZEOF_ALIGN(cardoid_t);
  pHandler->internal_memory_size = byte_offset;
  return ACOUSTIC_BF_TYPE_ERROR_NONE;
}

/******************************************************************************/
/********************STATIC FUNCTIONS *****************************************/
/******************************************************************************/

static void s_updateGain(cardoid_t *const pCardoid, int16_t *pDataS1, int16_t *pDataS2)
{
  uint32_t GainSamplesCounter_0 = pCardoid->ctxt.gain.GainSamplesCounter_0;
  float32_t RMSm1_0             = pCardoid->ctxt.gain.RMSm1_0;
  float32_t RMSm2_0             = pCardoid->ctxt.gain.RMSm2_0;
  float32_t gain_0              = pCardoid->ctxt.gain.gain_0;
  float32_t gain_old_0          = pCardoid->ctxt.gain.gain_old_0;


  for (int i = 0; i < 16; i++)
  {
    RMSm1_0 += (float32_t)pDataS1[i] * (float32_t)pDataS1[i];
    RMSm2_0 += (float32_t)pDataS2[i] * (float32_t)pDataS2[i];
    GainSamplesCounter_0++;
  }

  if (GainSamplesCounter_0 == GAIN_COMPUTATION_LENGTH)
  {
    if (RMSm1_0 < 300000000.0f)
    {
      float32_t RMSgain = RMSm1_0 / RMSm2_0;
      if (RMSgain >= 0.0f)
      {
        /* errno = 0;*/
        gain_0 = ((sqrtf(RMSgain)) * 0.2f) + (gain_old_0 * 0.8f); /*cstat !MISRAC2012-Rule-22.8 not misra compliant because of errno, would waste CPU cycles */
      }
      if (gain_0 < 1.0f)
      {
        gain_0 = 1.0f;
      }
      pCardoid->ctxt.gain.gain_0 = gain_0;
      s_setGain(pCardoid, pCardoid->ctxt.gain.gain_0);
      gain_old_0 = gain_0;
    }
    GainSamplesCounter_0 = 0;
    RMSm1_0 = 0.0f;
    RMSm2_0 = 0.0f;
  }

  pCardoid->ctxt.gain.GainSamplesCounter_0 = GainSamplesCounter_0;
  pCardoid->ctxt.gain.RMSm1_0              = RMSm1_0;
  pCardoid->ctxt.gain.RMSm2_0              = RMSm2_0;
  pCardoid->ctxt.gain.gain_old_0           = gain_old_0;
}



static float32_t *s_getCoeff(uint16_t mic_distance)
{
  float32_t *pCoeffs = NULL;
  if (mic_distance <= MIC_DIST_THRESHOLD_3mm)
  {
    pCoeffs = (float32_t *) coefficients_3mm;
  }
  else if (mic_distance <= MIC_DIST_THRESHOLD_4mm)
  {
    pCoeffs = (float32_t *)coefficients_4mm;
  }
  else if (mic_distance <= MIC_DIST_THRESHOLD_5mm)
  {
    pCoeffs = (float32_t *)coefficients_5_65mm;
  }
  else if (mic_distance <= MIC_DIST_THRESHOLD_7mm)
  {
    pCoeffs = (float32_t *)coefficients_7mm;
  }
  else if (mic_distance <= MIC_DIST_THRESHOLD_15mm)
  {
    pCoeffs = (float32_t *)coefficients_15mm;
  }
  else if (mic_distance <= MIC_DIST_THRESHOLD_21mm)
  {
    pCoeffs = (float32_t *)coefficients_21_2mm;
  }
  else
  {
    /* wrong distance: >212 is not supported*/
    pCoeffs = NULL;
  }
  return pCoeffs;
}


static void s_initBf(cardoid_conf_t *pConf, float32_t alpha_antifilter, float32_t gain_antifilter_s1, float32_t gain_antifilter_s2)
{
  /* TODO JO: add comment about antifilter values processing */
  pConf->s1_out_old         = 0;
  pConf->s2_out_old         = 0;
  pConf->alpha_antifilter   = (uint16_t)alpha_antifilter;
  pConf->gain_antifilter_s1 = (uint16_t)gain_antifilter_s1;
  pConf->gain_antifilter_s2 = (uint16_t)gain_antifilter_s2;
}

static void s_configureBf(cardoid_conf_t *pConf, float32_t *const pCoeffs, float32_t gain_s1, float32_t gain_s2)
{
  if (pCoeffs != NULL)
  {
    float32_t alpha_antifilter, gain_antifilter_s1, gain_antifilter_s2;
    alpha_antifilter   = pCoeffs[0] * 256.0f;
    gain_antifilter_s1 = pCoeffs[1] * gain_s1 * 256.0f;
    gain_antifilter_s2 = pCoeffs[1] * gain_s2 * 256.0f;
    /* Init */
    s_initBf(pConf, alpha_antifilter, gain_antifilter_s1, gain_antifilter_s2);
  }
}

static void s_runBf(cardoid_conf_t *const pConf, int16_t *ptrBufferIn1, int16_t *ptrBufferIn2, int16_t *ptrBufferOut, uint32_t nbSamples)
{
  int32_t s1_out;
  int32_t s2_out;
  int32_t Z1;

  int32_t alpha_antifilter   = (int32_t) pConf->alpha_antifilter;
  int32_t gain_antifilter_s1 = (int32_t) pConf->gain_antifilter_s1;
  int32_t gain_antifilter_s2 = (int32_t) pConf->gain_antifilter_s2;
  s1_out = pConf->s1_out_old;
  s2_out = pConf->s2_out_old;

  /* TODO JO: shouldn't it be done in floating point */
  for (uint32_t i = 0UL; i < nbSamples; i++)
  {
    Z1 = (int32_t) ptrBufferIn1[i];
    s1_out = ((alpha_antifilter * s1_out) + (gain_antifilter_s1 * Z1)) / 256;
    Z1 = (int32_t) ptrBufferIn2[i];
    s2_out = ((alpha_antifilter * s2_out) + (gain_antifilter_s2 * Z1)) / 256;
    ptrBufferOut[i] = (int16_t) __SSAT(__QSUB(s1_out, s2_out), 16);
  }
  pConf->s1_out_old = s1_out;
  pConf->s2_out_old = s2_out;
}

static void s_setGain(cardoid_t *const pCardoid, float32_t gain)
{
  float32_t *const pCoeffs = s_getCoeff(pCardoid->mic_distance);
  /* Configure gain for front beam */
  s_configureBf(&pCardoid->antennaFront, pCoeffs, 1.0f, gain);

  /* Configure gain for rear beam if necessary */
  if (pCardoid->ctxt.isRearBfNeeded == 1U)
  {
    s_configureBf(&pCardoid->antennaRear, pCoeffs, gain, 1.0f);
  }
}




#endif
