/**
******************************************************************************
* @file    acoustic_bf_speex.h
* @author  SRA
* @brief   This file contains Acoustic Beamforming library definitions.
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
#ifndef __ACOUSTIC_BF_SPEEX_H
#define __ACOUSTIC_BF_SPEEX_H

/* Includes ------------------------------------------------------------------*/
#include "acoustic_bf_error.h"

/** @addtogroup MIDDLEWARES
* @{
*/

/** @defgroup ACOUSTIC_BF_SPEEX ACOUSTIC_BF_SPEEX
* @{
*/

/* Exported constants --------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/

/** @defgroup ACOUSTIC_BF_SPEEX_Exported_Types AcousticBF Exported Types
* @{
*/
/**
 * @brief  Handler. It keeps track of the static parameters
 *         and it handles the internal state of the algorithm.
 */
typedef struct
{
  uint8_t denoise_enable;                       /*!< Enable or disable the denoiser processing. */
  uint8_t adaptive_enable;                      /*!< Enable or disable the adaptive processing. */
  uint32_t internal_memory_size;                /*!< Keeps track of the amount of memory required for the current setup.
                                                     It's filled by the Beamforming_getMemorySize() function and must be
                                                     used to allocate the right amount of RAM */
  uint32_t *pInternalMemory;                    /*!< Pointer to the internal algorithm memory */
} AcousticBF_speex_t;


/* Exported macro ------------------------------------------------------------*/
/* Exported define -----------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

/** @defgroup ACOUSTIC_BF_SPEEX_Exported_Functions AcousticBF Exported Functions
* @{
*/

/**
 * @brief  Init denoiser
 * @param  pHandler: speex wrapper handler filled with desired parameters.
 * @retval 0 if everything is fine.
 */
uint32_t AcousticBF_speex_Init(AcousticBF_speex_t *pHandler);
/**
 * @brief  Fills the "internal_memory_size" of the pHandler parameter passed as argument with a value representing the
 *         right amount of memory needed by the library, depending on the specific static parameters adopted.
 * @param  pHandler: speex wrapper handler filled with desired parameters.
 * @retval 0 if everything is fine.
 */
uint32_t AcousticBF_speex_GetMemorySize(AcousticBF_speex_t *pHandler);
/**
 * @brief  set up denoiser
 * @param  pHandler: speex wrapper handler filled with desired parameters.
 * @retval 0 if everything is fine.
 */
uint32_t AcousticBF_speex_SetupDenoiser(AcousticBF_speex_t *pHandler);
/**
 * @brief  run adaptive filter
 * @param  pHandler: speex wrapper handler filled with desired parameters.
 * @param  pDir1: pointer on input samples front antenna.
 * @param  pDir2: pointer on input samples rear antenna.
 * @param  pOut: pointer on output samples.
 * @retval 0 if everything is fine.
 */
uint32_t AcousticBF_speex_RunApadtive(AcousticBF_speex_t *pHandler, int16_t *const pDir1, int16_t *const pDir2, int16_t *const pOut);
/**
 * @brief  run denoiser
 * @param  pHandler: speex wrapper handler filled with desired parameters.
 * @param  pOut: pointer on output samples.
 * @retval 0 if everything is fine.
 */
uint32_t AcousticBF_speex_runDenoiser(AcousticBF_speex_t *pHandler, int16_t *const pOut);


/**
* @}
*/

/**
  * @}
  */
#endif  /*__ACOUSTIC_BF_SPEEX_H*/

