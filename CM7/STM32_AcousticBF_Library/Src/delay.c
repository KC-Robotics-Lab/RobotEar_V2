/**
******************************************************************************
* @file    delay.c
* @author  SRA
* @brief   delay implementation
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
#ifndef __DELAY_C_
#define __DELAY_C_

/* Includes ------------------------------------------------------------------*/
#include "delay.h"
/* Private typedef -----------------------------------------------------------*/
typedef struct _context_t
{
  uint8_t   nBytes;
  uint8_t   nBits;
  uint16_t  idLastSamples;
  uint16_t  prevLastPart;
  size_t    mvSizeBytes;
  uint8_t  *pLastPart;
} context_t;

/* Private defines -----------------------------------------------------------*/
//#define DEBUG_DELAY_PDM
/* Private macros ------------------------------------------------------------*/
#ifndef DELAY_UNUSED
  #define DELAY_UNUSED(X) (void)X      /* To avoid gcc/g++ warnings */
#endif
#define SIZEOF_ALIGN                ACOUSTIC_BF_SIZEOF_ALIGN

/* Private variables ---------------------------------------------------------*/
#ifdef DEBUG_DELAY_PDM
  static uint8_t debugIn[512], debugOut[512];
  static int debugIdxIn = 0;
  static int debugIdxOut = 0;
#endif
/* Private function prototypes -----------------------------------------------*/

/* Functions Definition ------------------------------------------------------*/

uint32_t Delay_init(Delay_Handler_t *pHandler, uint16_t channel_offset, uint16_t nb_samples, uint16_t delay)
{
  uint32_t ret = ACOUSTIC_BF_TYPE_ERROR_NONE;
  context_t *const pContext = (context_t *)(pHandler->pInternalMemory);

  uint32_t  byte_offset = SIZEOF_ALIGN(context_t);
  if (pContext == NULL)
  {
    ret = ACOUSTIC_BF_ALLOCATION_ERROR;
  }
  else
  {
    (void)memset(pHandler->pInternalMemory, 0, pHandler->internal_memory_size);

    pHandler->channel_offset = channel_offset;
    pHandler->nb_samples     = nb_samples;
    pHandler->delay          = delay;
    pContext->nBytes         = (uint8_t)(pHandler->delay / 8U);
    pContext->nBits          = (uint8_t)(pHandler->delay % 8U);
    pContext->idLastSamples  = nb_samples - 1U;
    pContext->mvSizeBytes    = (size_t)pContext->idLastSamples * sizeof(int16_t);
    pContext->pLastPart = (uint8_t *)((uint8_t *)pHandler->pInternalMemory + byte_offset);

  }
  return ret;
}

uint32_t Delay_getMemorySize(Delay_Handler_t *pHandler)
{
  uint32_t  byte_offset = SIZEOF_ALIGN(context_t);

  byte_offset += SIZEOF_ALIGN(uint8_t) * pHandler->nb_samples;
  while ((++byte_offset % 4U) != 0U)
  {
  }
  pHandler->internal_memory_size = byte_offset;
  return ACOUSTIC_BF_TYPE_ERROR_NONE;
}

uint32_t Delay_one_pcm(Delay_Handler_t *pHandler, void *pDest, void *pSrc)
{
  context_t *const pContext = (context_t *)(pHandler->pInternalMemory);
  uint16_t *pBuffIn         = (uint16_t *)pSrc;
  uint16_t *pBuffOut        = (uint16_t *)pDest;
  uint16_t curLastPart      = pBuffIn[pContext->idLastSamples * pHandler->channel_offset];

  memmove((void *)(&pBuffOut[1]), pSrc, pContext->mvSizeBytes);
  pBuffOut[0]               = pContext->prevLastPart;
  pContext->prevLastPart    = curLastPart;

  return ACOUSTIC_BF_TYPE_ERROR_NONE;
}

uint32_t Delay_pdmMsb(Delay_Handler_t *pHandler, void *buffer_in, void *buffer_out, uint16_t sampling_frequency)
{
  context_t *const pContext = (context_t *)(pHandler->pInternalMemory);
  uint16_t i;
  uint8_t *pIn             = (uint8_t *)buffer_in;
  uint8_t *pOut            = (uint8_t *)buffer_out;
  uint16_t channel_offset  = pHandler->channel_offset;
  uint16_t nBytesTotal     = sampling_frequency / 8U;
  uint16_t nBytes          = (uint16_t)pContext->nBytes;
  uint8_t  nBits           = pContext->nBits;
  uint8_t  temp;
  uint8_t  temp1;

  /*Old Last Part in outBuff*/
  for (i = 0U; i < nBytes; i++)
  {
    pOut[i] = pContext->pLastPart[i];
    #ifdef DEBUG_DELAY_PDM
    debugOut[debugIdxOut] = pOut[i];
    debugIdxOut = (debugIdxOut + 1) % 512;
    #endif
  }

  if (nBits == 0U)
  {
    /*New OutBuff Part*/
    for (; i < nBytesTotal; i++)
    {
      #ifdef DEBUG_DELAY_PDM
      debugIn[debugIdxIn] = *pIn;
      debugIdxIn = (debugIdxIn + 1) % 512;
      #endif
      pOut[i] = *pIn;
      pIn          += channel_offset;
      #ifdef DEBUG_DELAY_PDM
      debugOut[debugIdxOut] = pOut[i];
      debugIdxOut = (debugIdxOut + 1) % 512;
      #endif
    }

    /*New Last Part*/
    for (i = 0U; i < nBytes; i++)
    {
      #ifdef DEBUG_DELAY_PDM
      debugIn[debugIdxIn] = *pIn;
      debugIdxIn = (debugIdxIn + 1) % 512;
      #endif
      pContext->pLastPart[i] = *pIn;
      pIn                   += channel_offset;
    }
  }
  else
  {
    uint8_t leftShift = (nBits > 8U) ? 0U : (8U - nBits);
    /*New OutBuff Part*/
    temp = pContext->pLastPart[nBytes];
    for (; i < nBytesTotal; i++)
    {
      #ifdef DEBUG_DELAY_PDM
      debugIn[debugIdxIn] = *pIn;
      debugIdxIn = (debugIdxIn + 1) % 512;
      #endif
      temp1         = *pIn;
      pIn          += channel_offset;
      pOut[i] = temp | (temp1 >> nBits);
      temp          = (temp1 << leftShift);
      #ifdef DEBUG_DELAY_PDM
      debugOut[debugIdxOut] = pOut[i];
      debugIdxOut = (debugIdxOut + 1) % 512;
      #endif
    }

    /*New Last Part*/
    for (i = 0U; i < nBytes; i++)
    {
      #ifdef DEBUG_DELAY_PDM
      debugIn[debugIdxIn] = *pIn;
      debugIdxIn = (debugIdxIn + 1) % 512;
      #endif
      temp1                      = *pIn;
      pIn                       += channel_offset;
      pContext->pLastPart[i] = temp | (temp1 >> nBits);
      temp                       = (temp1 << leftShift);
    }
    pContext->pLastPart[nBytes] = temp;
  }
  return ACOUSTIC_BF_TYPE_ERROR_NONE;
}


uint32_t Delay_pdmLsb(Delay_Handler_t *pHandler, void *buffer_in, void *buffer_out, uint16_t sampling_frequency)
{
  context_t *const pContext = (context_t *)(pHandler->pInternalMemory);
  uint16_t i;
  uint8_t *pIn         = (uint8_t *)buffer_in;
  uint8_t *pOut        = (uint8_t *)buffer_out;
  uint16_t channel_offset     = pHandler->channel_offset;
  uint16_t nBytesTotal = sampling_frequency / 8U;
  uint16_t nBytes      = (uint16_t)pContext->nBytes;
  uint8_t  nBits       = pContext->nBits;
  uint8_t  temp;
  uint8_t  temp1;

  /*Old Last Part in outBuff*/
  for (i = 0U; i < nBytes; i++)
  {
    pOut[i] = pContext->pLastPart[i];
    #ifdef DEBUG_DELAY_PDM
    debugOut[debugIdxOut] = pOut[i];
    debugIdxOut = (debugIdxOut + 1) % 512;
    #endif
  }



  if (nBits == 0U)
  {
    /*New OutBuff Part*/
    for (; i < nBytesTotal; i++)
    {
      #ifdef DEBUG_DELAY_PDM
      debugIn[debugIdxIn] = *pIn;
      debugIdxIn = (debugIdxIn + 1) % 512;
      #endif
      pOut[i] = *pIn;
      pIn          += channel_offset;
      #ifdef DEBUG_DELAY_PDM
      debugOut[debugIdxOut] = pOut[i];
      debugIdxOut = (debugIdxOut + 1) % 512;
      #endif
    }



    /*New Last Part*/
    for (i = 0U; i < nBytes; i++)
    {
      #ifdef DEBUG_DELAY_PDM
      debugIn[debugIdxIn] = *pIn;
      debugIdxIn = (debugIdxIn + 1) % 512;
      #endif
      pContext->pLastPart[i] = *pIn;
      pIn                       += channel_offset;
    }
  }
  else
  {
    uint8_t leftShift = (nBits > 8U) ? 0U : (8U - nBits);
    /*New OutBuff Part*/
    temp = pContext->pLastPart[nBytes];
    for (; i < nBytesTotal; i++)
    {
      #ifdef DEBUG_DELAY_PDM
      debugIn[debugIdxIn] = *pIn;
      debugIdxIn = (debugIdxIn + 1) % 512;
      #endif
      temp1         = *pIn;
      pIn          += channel_offset;
      pOut[i] = temp | (temp1 << leftShift);
      temp          = (temp1 >> nBits);
      #ifdef DEBUG_DELAY_PDM
      debugOut[debugIdxOut] = pOut[i];
      debugIdxOut = (debugIdxOut + 1) % 512;
      #endif
    }

    /*New Last Part*/
    for (i = 0U; i < nBytes; i++)
    {
      #ifdef DEBUG_DELAY_PDM
      debugIn[debugIdxIn] = *pIn;
      debugIdxIn = (debugIdxIn + 1) % 512;
      #endif
      temp1                      = *pIn;
      pIn                       += channel_offset;
      pContext->pLastPart[i] = temp | (temp1 << leftShift);
      temp                       = (temp1 >> nBits);
    }
    pContext->pLastPart[nBytes] = temp;
  }
  return ACOUSTIC_BF_TYPE_ERROR_NONE;
}




#endif
