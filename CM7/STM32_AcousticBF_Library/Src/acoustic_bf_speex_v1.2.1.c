///**
//******************************************************************************
//* @file    acoustic_bf_speex.c
//* @author  SRA
//* @brief   Beamforming core library
//******************************************************************************
//* @attention
//*
//* Copyright (c) 2022 STMicroelectronics.
//* All rights reserved.
//*
//* This software is licensed under terms that can be found in the LICENSE file in
//* the root directory of this software component.
//* If no LICENSE file comes with this software, it is provided AS-IS.
//*
//*
//******************************************************************************
//*/
//
///* Define to prevent recursive inclusion -------------------------------------*/
//#ifndef __ACOUSTIC_BF_SPEEX_C
//#define __ACOUSTIC_BF_SPEEX_C
//
///* Includes ------------------------------------------------------------------*/
//#include "acoustic_bf_speex.h"
//
///*cstat -MISRAC2012-* won't apply misra on speex files */
//#include <arm_math.h>
///*cstat +MISRAC2012-* */
//#include "speex/speexdsp_types.h"
//#include "speex_aec/os_support_custom.h"
//#include "speex/speex_echo.h"
//#include "speex/speex_preprocess.h"
//#include "smallft.h"
//#include "filterbank.h"
//
//
///* Private typedef -----------------------------------------------------------*/
//
//typedef struct
//{
//  uint8_t               denoise_init_done;
//  uint8_t               adaptive_init_done;
//  SpeexEchoState       *pAdaptiveHdle;
//  SpeexPreprocessState *pDenoiseHdle;
//} context_t;
//
///* Private defines -----------------------------------------------------------*/
//#define NN 128
//#define TAIL 1
//
///* Private macros ------------------------------------------------------------*/
//#define SIZEOF_ALIGN ACOUSTIC_BF_SIZEOF_ALIGN
///* Global variables ----------------------------------------------------------*/
///* Private function prototypes -----------------------------------------------*/
//uint32_t s_InitDenoiser(SpeexPreprocessState *pDenoise);
//uint32_t s_InitAdaptive(SpeexEchoState       *pAdaptive);
//
///* Functions Definition ------------------------------------------------------*/
//uint32_t AcousticBF_speex_Init(AcousticBF_speex_t *pHandler)
//{
//  uint32_t ret = ACOUSTIC_BF_TYPE_ERROR_NONE;
//  context_t *const pContext = (context_t *)(pHandler->pInternalMemory);
//  if (pHandler->adaptive_enable == 1U)
//  {
//    pContext->pAdaptiveHdle = speex_echo_state_init(NN, TAIL);
//    if (pContext->pAdaptiveHdle == NULL)
//    {
//      ret  = ACOUSTIC_BF_ALLOCATION_ERROR;
//    }
//    else
//    {
//      pContext->adaptive_init_done = 1U;
//    }
//  }
//  if (pHandler->denoise_enable == 1U)
//  {
//    pContext->pDenoiseHdle = speex_preprocess_state_init(NN, 16000);
//    if (pContext->pDenoiseHdle == NULL)
//    {
//      ret  = ACOUSTIC_BF_ALLOCATION_ERROR;
//    }
//    else
//    {
//      pContext->denoise_init_done = 1U;
//    }
//  }
//  return ret;
//}
//
//uint32_t AcousticBF_speex_GetMemorySize(AcousticBF_speex_t *pHandler)
//{
//  uint32_t  byte_offset = SIZEOF_ALIGN(context_t);
//
//  while ((++byte_offset % 4U) != 0U)
//  {
//  }
//  pHandler->internal_memory_size = byte_offset;
//  return ACOUSTIC_BF_TYPE_ERROR_NONE;
//}
//
//
//uint32_t AcousticBF_speex_SetupDenoiser(AcousticBF_speex_t *pHandler)
//{
//  uint32_t ret = ACOUSTIC_BF_TYPE_ERROR_NONE;
//  context_t     *const pContext   = (context_t *)(pHandler->pInternalMemory);
//
//  if (pContext->denoise_init_done == 1U)
//  {
//    int err = speex_preprocess_ctl(pContext->pDenoiseHdle, SPEEX_PREPROCESS_SET_ECHO_STATE, pContext->pAdaptiveHdle);
//    if (err != 0)
//    {
//      ret = ACOUSTIC_BF_TYPE_ERROR;
//    }
//  }
//  else
//  {
//    ret = ACOUSTIC_BF_TYPE_ERROR;
//  }
//  return ret;
//}
//
//uint32_t AcousticBF_speex_RunApadtive(AcousticBF_speex_t *pHandler, int16_t *const pDir1, int16_t *const pDir2, int16_t *const pOut)
//{
//  uint32_t ret = ACOUSTIC_BF_TYPE_ERROR_NONE;
//  context_t     *const pContext   = (context_t *)(pHandler->pInternalMemory);
//  if (pContext->adaptive_init_done == 1U)
//  {
//    speex_echo_cancellation(pContext->pAdaptiveHdle, pDir1, pDir2, pOut);
//  }
//  else
//  {
//    ret = ACOUSTIC_BF_TYPE_ERROR;
//  }
//  return ret;
//}
//
//uint32_t AcousticBF_speex_runDenoiser(AcousticBF_speex_t *pHandler, int16_t *const pOut)
//{
//  uint32_t ret = ACOUSTIC_BF_TYPE_ERROR_NONE;
//  context_t     *const pContext   = (context_t *)(pHandler->pInternalMemory);
//  if (pContext->denoise_init_done == 1U)
//  {
//    speex_preprocess_run(pContext->pDenoiseHdle, pOut);
//  }
//  else
//  {
//    ret = ACOUSTIC_BF_TYPE_ERROR;
//  }
//  return ret;
//}
//
//
//#endif  /*__ACOUSTIC_BF_SPEEX_C*/
//
