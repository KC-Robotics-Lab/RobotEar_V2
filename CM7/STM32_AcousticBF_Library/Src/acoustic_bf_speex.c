/**
******************************************************************************
* @file    acoustic_bf_speex.c
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
#ifndef __ACOUSTIC_BF_SPEEX_C
#define __ACOUSTIC_BF_SPEEX_C

/* Includes ------------------------------------------------------------------*/
#include "acoustic_bf_speex.h"
#include "defines.h"

/*cstat -MISRAC2012-* won't apply misra on speex files */
#include "filterbank.c"
#include "adaptive.c"
#include "denoiser.c"
#include "smallft.c"
/*cstat +MISRAC2012-* */

/* Private typedef -----------------------------------------------------------*/

typedef struct
{
  SpeexPreprocessState hdle;       /*!< speex process handler*/
  FilterBank           filterBank; /*!< filter tap memory */
  drft_lookup          table;  /*!< fft coefficient memory for denoiser */
} denoise_context_t;

typedef struct
{
  SpeexEchoState       hdle;         /*!< speex echo state */
  drft_lookup          table;      /*!< fft coefficient memory for adaptive filtering */
} adaptive_context_t;


typedef struct
{
  uint8_t denoise_init_done;
  uint8_t adaptive_init_done;
  adaptive_context_t *pAdaptive;
  denoise_context_t  *pDenoise;
} context_t;

/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
#define SIZEOF_ALIGN ACOUSTIC_BF_SIZEOF_ALIGN
/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
uint32_t s_InitDenoiser(denoise_context_t  *pDenoise);
uint32_t s_InitAdaptive(adaptive_context_t *pAdaptive);

/* Functions Definition ------------------------------------------------------*/
uint32_t AcousticBF_speex_Init(AcousticBF_speex_t *pHandler)
{
  uint32_t ret = ACOUSTIC_BF_TYPE_ERROR_NONE;
  context_t *const pContext = (context_t *)(pHandler->pInternalMemory);
  uint32_t byte_offset = SIZEOF_ALIGN(context_t);
  if (pHandler->adaptive_enable == 1U)
  {
    pContext->pAdaptive = (adaptive_context_t *)((uint8_t *)pHandler->pInternalMemory + byte_offset);
    byte_offset += SIZEOF_ALIGN(adaptive_context_t);
    ret |= s_InitAdaptive(pContext->pAdaptive);
    if (ret == ACOUSTIC_BF_TYPE_ERROR_NONE)
    {
      pContext->adaptive_init_done = 1U;
    }
  }
  if (pHandler->denoise_enable == 1U)
  {
    pContext->pDenoise = (denoise_context_t *)((uint8_t *)pHandler->pInternalMemory + byte_offset);
    byte_offset += SIZEOF_ALIGN(denoise_context_t);
    ret |= s_InitDenoiser(pContext->pDenoise);
    if (ret == ACOUSTIC_BF_TYPE_ERROR_NONE)
    {
      pContext->denoise_init_done = 1U;
    }
  }
  return ret;
}

uint32_t s_InitDenoiser(denoise_context_t  *pDenoise)
{
  uint32_t ret = ACOUSTIC_BF_TYPE_ERROR_NONE;
  /********** DENOISER (FOR LIGHT OR STRONG VERSIONS)********/
  fft_init(&pDenoise->table, (int32_t)NN_MAX * 2);
  pDenoise->hdle.fft_lookup = (drft_lookup *)&pDenoise->table;
  denoiserstate_init((SpeexPreprocessState *)&pDenoise->hdle, NN, 16000);
  filterbank_new((FilterBank *) &pDenoise->filterBank, NB_BANDS, 16000.0f, NN_MAX, 1);
  pDenoise->hdle.bank = (FilterBank *) &pDenoise->filterBank;
  return ret;
}

uint32_t s_InitAdaptive(adaptive_context_t *pAdaptive)
{
  uint32_t ret = ACOUSTIC_BF_TYPE_ERROR_NONE;
  /********** ADAPTIVE (FOR ASR OR STRONG VERSIONS)********/
  fft_init(&pAdaptive->table, (int32_t)NN_MAX * 2);
  pAdaptive->hdle.fft_table = &pAdaptive->table;
  adaptivestate_init_mc((SpeexEchoState *) &pAdaptive->hdle, NN, TAIL, 1, 1);
  return ret;
}

uint32_t AcousticBF_speex_GetMemorySize(AcousticBF_speex_t *pHandler)
{
  uint32_t  byte_offset = SIZEOF_ALIGN(context_t);
  if (pHandler->adaptive_enable == 1U)
  {
    byte_offset += SIZEOF_ALIGN(adaptive_context_t);
  }
  if (pHandler->denoise_enable == 1U)
  {
    byte_offset += SIZEOF_ALIGN(denoise_context_t);
  }

  while ((++byte_offset % 4U) != 0U)
  {
  }
  pHandler->internal_memory_size = byte_offset;
  return ACOUSTIC_BF_TYPE_ERROR_NONE;
}


uint32_t AcousticBF_speex_SetupDenoiser(AcousticBF_speex_t *pHandler)
{
  uint32_t ret = ACOUSTIC_BF_TYPE_ERROR_NONE;
  context_t     *const pContext   = (context_t *)(pHandler->pInternalMemory);

  if (pContext->denoise_init_done == 1U)
  {
    denoiser_setup((SpeexPreprocessState *)&pContext->pDenoise->hdle, SPEEX_PREPROCESS_SET_ECHO_STATE, (SpeexEchoState *)&pContext->pAdaptive->hdle);
  }
  else
  {
    ret = ACOUSTIC_BF_TYPE_ERROR;
  }
  return ret;
}

uint32_t AcousticBF_speex_RunApadtive(AcousticBF_speex_t *pHandler, int16_t *const pDir1, int16_t *const pDir2, int16_t *const pOut)
{
  uint32_t ret = ACOUSTIC_BF_TYPE_ERROR_NONE;
  context_t     *const pContext   = (context_t *)(pHandler->pInternalMemory);
  if (pContext->adaptive_init_done == 1U)
  {
    adaptive_A_run((SpeexEchoState *)&pContext->pAdaptive->hdle, pDir1, pDir2, pOut);
  }
  else
  {
    ret = ACOUSTIC_BF_TYPE_ERROR;
  }
  return ret;
}

uint32_t AcousticBF_speex_runDenoiser(AcousticBF_speex_t *pHandler, int16_t *const pOut)
{
  uint32_t ret = ACOUSTIC_BF_TYPE_ERROR_NONE;
  context_t     *const pContext   = (context_t *)(pHandler->pInternalMemory);
  if (pContext->denoise_init_done == 1U)
  {
    denoiser_A_run((SpeexPreprocessState *)&pContext->pDenoise->hdle, pOut);
  }
  else
  {
    ret = ACOUSTIC_BF_TYPE_ERROR;
  }
  return ret;
}


/**
******************************************************************************
* Copyright (C) 2005-2006 Jean-Marc Valin
* File: fftwrap.c
*
* Wrapper for various FFTs
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
* - Redistributions of source code must retain the above copyright notice, this list
*   of conditions and the following disclaimer.
* - Redistributions in binary form must reproduce the above copyright notice, this
*   list of conditions and the following disclaimer in the documentation and/or
*   other materials provided with the distribution.
* - Neither the name of the Xiph.org Foundation nor the names of its contributors
*   may be used to endorse or promote products derived from this software without
*   specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************
*/

static void fft_init(drft_lookup *table, int32_t size)
{
  drft_init((drft_lookup *)table, size);
}

static void fft(drft_lookup *table, float32_t *in, float32_t *out)
{
  int32_t i;
  float32_t scale = 1.0f / ((float32_t)(table->n));
  for (i = 0; i < ((drft_lookup *)table)->n; i++)
  {
    out[i] = scale * in[i];
  }
  drft_forward((drft_lookup *)table, out);
}

static void ifft(drft_lookup *table, float32_t *in, float32_t *out)
{
  int32_t i;
  for (i = 0; i < ((drft_lookup *)table)->n; i++)
  {
    out[i] = in[i];
  }
  drft_backward((drft_lookup *)table, out);
}


#endif  /*__ACOUSTIC_BF_SPEEX_C*/

