/**
******************************************************************************
* @file    acoustic_bf_error.h
* @author  SRA
* @brief   This file contains Acoustic Beamforming library error definitions.
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
#ifndef __ACOUSTIC_BF_ERROR_H
#define __ACOUSTIC_BF_ERROR_H

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>


/** @addtogroup MIDDLEWARES
* @{
*/

/** @defgroup ACOUSTIC_BF ACOUSTIC_BF
* @{
*/

/* Exported constants --------------------------------------------------------*/

/** @defgroup ACOUSTIC_BF_Exported_Constants AcousticBF Exported Constants
* @{
*/


/** @defgroup ACOUSTIC_BF_errors
* @brief    Beam Forming errors
* @{
*/
#define ACOUSTIC_BF_TYPE_ERROR_NONE                   ((uint32_t)0x00000000)
#define ACOUSTIC_BF_TYPE_ERROR                        ((uint32_t)0x00000001)
#define ACOUSTIC_BF_PTR_CHANNELS_ERROR                ((uint32_t)0x00000002)
#define ACOUSTIC_BF_DATA_FORMAT_ERROR                 ((uint32_t)0x00000004)
#define ACOUSTIC_BF_SAMPLING_FREQ_ERROR               ((uint32_t)0x00000008)
#define ACOUSTIC_BF_M2_GAIN_ERROR                     ((uint32_t)0x00000010)
#define ACOUSTIC_BF_DISTANCE_ERROR                    ((uint32_t)0x00000020)
#define ACOUSTIC_BF_REF_OUT_ERROR                     ((uint32_t)0x00000040)
#define ACOUSTIC_BF_DELAY_ERROR                       ((uint32_t)0x00000080)
#define ACOUSTIC_BF_MIXER_ERROR                       ((uint32_t)0x00000081)
#define ACOUSTIC_BF_PROCESSING_ERROR                  ((uint32_t)0x00000100)
#define ACOUSTIC_BF_ALLOCATION_ERROR                  ((uint32_t)0x00000200)
#define ACOUSTIC_BF_PDM2PCM_ERROR_SHIFT               10
#define ACOUSTIC_BF_PDM2PCM_INIT_ERROR                (PDM2PCM_INIT_ERROR           << ACOUSTIC_BF_PDM2PCM_ERROR_SHIFT)
#define ACOUSTIC_BF_PDM2PCM_CONFIG_ERROR              (PDM2PCM_CONFIG_ERROR         << ACOUSTIC_BF_PDM2PCM_ERROR_SHIFT)
#define ACOUSTIC_BF_PDM2PCM_ENDIANNESS_ERROR          (PDM2PCM_ENDIANNESS_ERROR     << ACOUSTIC_BF_PDM2PCM_ERROR_SHIFT)
#define ACOUSTIC_BF_PDM2PCM_BIT_ORDER_ERROR           (PDM2PCM_BIT_ORDER_ERROR      << ACOUSTIC_BF_PDM2PCM_ERROR_SHIFT)
#define ACOUSTIC_BF_PDM2PCM_CRC_LOCK_ERROR            (PDM2PCM_CRC_LOCK_ERROR       << ACOUSTIC_BF_PDM2PCM_ERROR_SHIFT)
#define ACOUSTIC_BF_PDM2PCM_DECIMATION_ERROR          (PDM2PCM_DECIMATION_ERROR     << ACOUSTIC_BF_PDM2PCM_ERROR_SHIFT)
#define ACOUSTIC_BF_PDM2PCM_GAIN_ERROR                (PDM2PCM_GAIN_ERROR           << ACOUSTIC_BF_PDM2PCM_ERROR_SHIFT)
#define ACOUSTIC_BF_PDM2PCM_SAMPLES_NUMBER_ERROR      (PDM2PCM_SAMPLES_NUMBER_ERROR << ACOUSTIC_BF_PDM2PCM_ERROR_SHIFT)


#ifndef ACOUSTIC_LOCK_ERROR
  #define ACOUSTIC_LOCK_ERROR                           ((uint32_t)0x10000000)
#endif
/**
* @}
*/


/**
* @}
*/

/* Exported types ------------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/

/** @defgroup ACOUSTIC_BF_Exported_macros AcousticBF Exported Macros
* @{
*/
/** @defgroup ACOUSTIC_BF_alignement
* @brief    Beam Forming macros to avoid alignement issue when calling *_getMemory() routines
* @{
*/
#define ACOUSTIC_BF_ALIGNED_SIZE                4UL                                                                                             /*!< alignement size */
#define ACOUSTIC_BF_SIZE_ALIGN(size)            (((size) + ACOUSTIC_BF_ALIGNED_SIZE - 1UL) & (0xFFFFFFFFUL - (ACOUSTIC_BF_ALIGNED_SIZE - 1UL))) /*!< general macro to get alignement size */
#define ACOUSTIC_BF_SIZEOF_ALIGN(object)        ACOUSTIC_BF_SIZE_ALIGN(sizeof(object))                                                          /*!< specific macro to replace all sizeof */
/**
* @}
*/
/**
  * @}
  */

/* Exported define -----------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

/**
* @}
*/

/**
  * @}
  */
#endif  /*__ACOUSTIC_BF_ERROR_H*/

