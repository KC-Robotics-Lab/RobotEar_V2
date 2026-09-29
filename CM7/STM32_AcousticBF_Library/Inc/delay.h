/**
******************************************************************************
* @file    delay.h
* @author  SRA
* @brief   This file contains Acoustic Beamforming Delay definitions.
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
#ifndef __DELAY_H
#define __DELAY_H

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
/*cstat -MISRAC2012-* CMSIS not misra compliant */
#include <arm_math.h>
/*cstat +MISRAC2012-* */
#include "acoustic_bf_error.h"
/** @addtogroup MIDDLEWARES
* @{
*/

/** @defgroup DELAY DELAY
* @{
*/

/* Exported constants --------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/

/**
 * @brief  Enum for type of delay.
 */
typedef enum
{
  DELAY_NONE,                    /*!< No Delay applied*/
  DELAY_PCM,                     /*!< Delay is apllied in PCM sample domain*/
  DELAY_PDM                      /*!< Delay is apllied in PDM sample domain*/
} Delay_Type_t;


/**
 * @brief  Handler. It keeps track of the static parameters
 *         and it handles the internal state of the algorithm.
 */

typedef struct
{
  uint16_t channel_offset;        /*!< channel offset inside buffer */
  uint16_t nb_samples;            /*!< buffer length */
  uint16_t delay;                 /*!< Delay to be applied */
  uint32_t internal_memory_size;  /*!< Keeps track of the amount of memory required for the current setup.
                                      It's filled by the Beamforming_getMemorySize() function and must be
                                      used to allocate the right amount of RAM */
  uint32_t *pInternalMemory;      /*!< Pointer to the internal algorithm memory */
} Delay_Handler_t;


/**
  * @}
  */

/* Exported macro ------------------------------------------------------------*/
/* Exported define -----------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

/** @defgroup DELAY_Exported_Functions Delay Exported Functions
* @{
*/

/**
 * @brief  Fills the "internal_memory_size" of the pHandler parameter passed as argument with a value representing the
 *         right amount of memory needed by the library, depending on the specific static parameters adopted.
 * @param  pHandler: delay_Handler filled with desired parameters.
 * @retval 0 if everything is fine.
 */
uint32_t Delay_getMemorySize(Delay_Handler_t *pHandler);

/**
 * @brief  Library initialization
 * @param  pHandler: Delay_Handler_t filled with desired parameters.
 * @param  channel_offset: channels offset in case of mult channel
 * @param  nb_samples: number of samples.
 * @param  delay: Delay to be applied
 * @retval 0 if everything is fine.
 *         different from 0 if erroneous parameters have been passed to the Init function and the default value has been used.
 *         The specific error can be recognized by checking the relative bit in the returned word.
 */
uint32_t Delay_init(Delay_Handler_t *pHandler, uint16_t channel_offset, uint16_t nb_samples, uint16_t delay);

/**
 * @brief  Library data input/output
 * @param  pHandler: pointer to the handler of the current Beamforming instance running.
 * @param  pSrc: pointer to an array that contains PCM samples (1 millisecond).
 * @param  pDest: pointer to an array that contains out delayed PCM (1 millisecond).
 * @retval 1 if data collection is finished and Beamforming_SecondStep must be called, 0 otherwise.
 * @note   Input/output function reads and write samples expect non interleaved data.
 */
uint32_t Delay_one_pcm(Delay_Handler_t *pHandler, void *pDest, void *pSrc);

/**
 * @brief  Library data input/output
 * @param  pHandler: pointer to the handler of the current Beamforming instance running.
 * @param  buffer_in: pointer to an array that contains PDM samples (1 millisecond).
 * @param  buffer_out: pointer to an array that contains out delayed PDM (1 millisecond).
 * @param  sampling_frequency: Number fo samples to treat.
 * @retval 1 if data collection is finished and Beamforming_SecondStep must be called, 0 otherwise.
 * @note   Input/output function reads and write samples expect non interleaved data.
 */
uint32_t Delay_pdmLsb(Delay_Handler_t *pHandler, void *buffer_in, void *buffer_out, uint16_t sampling_frequency);

/**
 * @brief  Library data input/output
 * @param  pHandler: pointer to the handler of the current Beamforming instance running.
 * @param  buffer_in: pointer to an array that contains PDM samples (1 millisecond).
 * @param  buffer_out: pointer to an array that contains out delayed PDM (1 millisecond).
 * @param  sampling_frequency: Number fo samples to treat.
 * @retval 1 if data collection is finished and Beamforming_SecondStep must be called, 0 otherwise.
 * @note   Input/output function reads and write samples expect non interleaved data.
 */
uint32_t Delay_pdmMsb(Delay_Handler_t *pHandler, void *buffer_in, void *buffer_out, uint16_t sampling_frequency);

/**
  * @}
  */

/**
* @}
*/

/**
  * @}
  */
#endif  /*__DELAY_H*/

