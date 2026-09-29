/**
******************************************************************************
* @file    cardoid.h
* @author  SRA
* @brief   This file contains Acoustic Beamforming Cardoid definitions.
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
#ifndef __CARDOID_H
#define __CARDOID_H

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
/*cstat -MISRAC2012-* CMSIS not misra compliant */
#include <arm_math.h>
/*cstat +MISRAC2012-* */

#include "acoustic_bf_error.h"

/** @addtogroup MIDDLEWARES
* @{
*/

/** @defgroup CARDOID CARDOID
* @{
*/

/* Exported constants --------------------------------------------------------*/

/** @defgroup CARDOID_Exported_Constants Cardoid Exported Constants
* @{
*/

/** @defgroup CARDOID_version
* @brief    Cardoid version
* @{
*/
#define CARDOID_VERSION                           "v4.0.0"
/**
* @}
*/

/** @defgroup CARDOID_interleaved
* @brief    Cardoid data format
* @{
*/
#define CARDOID_INTERLEAVED_NO                        ((uint8_t)0x00U)
#define CARDOID_INTERLEAVED_YES                       ((uint8_t)0x01U)
/**
* @}
*/

/** @defgroup CARDOID_rear_enabled
* @brief    Cardoid rear antenna activation
* @{
*/
#define CARDOID_REAR_DISABLE                       ((uint8_t)0x00U)
#define CARDOID_REAR_ENABLE                        ((uint8_t)0x01U)
/**
* @}
*/
/**
* @}
*/

/* Exported types ------------------------------------------------------------*/

/** @defgroup CARDOID_Exported_Types Cardoid Exported Types
* @{
*/


/**
 * @brief  Handler. It keeps track of the static parameters
 *         and it handles the internal state of the algorithm.
 */

typedef struct
{
  uint32_t internal_memory_size;                /*!< Keeps track of the amount of memory required for the current setup.
                                                     It's filled by the Beamforming_getMemorySize() function and must be
                                                     used to allocate the right amount of RAM */
  uint32_t *pInternalMemory;                    /*!< Pointer to the internal algorithm memory */
  uint8_t interleaved;                          /*!< Specifies the format , only non interleaved data is supported but this forces the user to pay attention to it!
                                                    This parameter can be a value of @ref CARDOID_interleaved. Default value is CARDOID_INTERLEAVED_NO*/

} Cardoid_Handler_t;

/**
 * @brief  Library dynamic configuration handler. It contains dynamic parameters.
 */
typedef struct
{
  uint16_t mic_distance;                        /*!< Distance between Mic1 and Mic2. It must be specified in tenths of a
                                                     millimeter. For example, if the microphone distance is equal to 4 mm,
                                                     this parameter must be initialized with the value 40. Default value is 150. */
  uint8_t rear_enable;                          /*!< Enable or disable the rear antenna calculation
                                                    This parameter can be a value of @ref CARDOID_reference_channel. Default value is CARDOID_REF_ENABLE*/

}
Cardoid_Config_t;

/**
  * @}
  */

/* Exported macro ------------------------------------------------------------*/
/* Exported define -----------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

/** @defgroup CARDOID_Exported_Functions Cardoid Exported Functions
* @{
*/

/**
 * @brief  Fills the "internal_memory_size" of the pHandler parameter passed as argument with a value representing the
 *         right amount of memory needed by the library, depending on the specific static parameters adopted.
 * @param  pHandler: cardoid_Handler filled with desired parameters.
 * @retval 0 if everything is fine.
 */
uint32_t Cardoid_getMemorySize(Cardoid_Handler_t *pHandler);

/**
 * @brief  Library initialization
 * @param  pHandler: Cardoid_Handler_t filled with desired parameters.
 * @retval 0 if everything is fine.
 *         different from 0 if erroneous parameters have been passed to the Init function and the default value has been used.
 *         The specific error can be recognized by checking the relative bit in the returned word.
 */
uint32_t Cardoid_init(Cardoid_Handler_t *pHandler);

/**
 * @brief  Library data input/output
 * @param  pHandler: pointer to the handler of the current Beamforming instance running.
 * @param  pM1: pointer to an array that contains PCM or PDM samples of the first channel (1 millisecond).
 * @param  pM2: pointer to an array that contains PCM or PDM samples of the second channel (1 millisecond).
 * @param  ptr_Out: pointer to an array that will contain PCM samples of output data (1 millisecond). If the reference channel
 *         is activated, two channels will be written in the output array.
 * @param  nbSamples: Number fo samples to treat.
 * @retval 1 if data collection is finished and Beamforming_SecondStep must be called, 0 otherwise.
 * @note   Input/output function reads and write samples expect non interleaved data.
 */
uint32_t Cardoid_runFront(Cardoid_Handler_t *pHandler, void *pM1, void *pM2, void *ptr_Out, uint32_t nbSamples);

/**
 * @brief  Library data input/output
 * @param  pHandler: pointer to the handler of the current Beamforming instance running.
 * @param  pM1: pointer to an array that contains PCM or PDM samples of the first channel (1 millisecond).
 * @param  pM2: pointer to an array that contains PCM or PDM samples of the second channel (1 millisecond).
 * @param  ptr_Out: pointer to an array that will contain PCM samples of output data (1 millisecond). If the reference channel
 *         is activated, two channels will be written in the output array.
 * @param  nbSamples: Number fo samples to treat.
 * @retval 1 if data collection is finished and Beamforming_SecondStep must be called, 0 otherwise.
 * @note   Input/output function reads and write samples expect non interleaved data.
 */
uint32_t Cardoid_runRear(Cardoid_Handler_t *pHandler, void *pM1, void *pM2, void *ptr_Out, uint32_t nbSamples);

/**
 * @brief  Library set gain value
 * @param  pHandler: pointer to the handler of the current Beamforming instance running.
 * @param  gain: new gain.
 * @retval 0 if OK.
 * @note   Input/output function reads and write samples expect non interleaved data.
 */
uint32_t Cardoid_setGain(Cardoid_Handler_t *pHandler, float32_t gain);

/**
 * @brief  Library return current gain value
 * @param  pHandler: pointer to the handler of the current Beamforming instance running.
 * @param  gain: current gain value.
 * @retval 0 if OK.
 * @note   Input/output function reads and write samples expect non interleaved data.
 */
uint32_t Cardoid_getGain(Cardoid_Handler_t *pHandler, float32_t *pGain);


/**
 * @brief  Library process & set gain value
 * @param  pHandler: pointer to the handler of the current Beamforming instance running.
 * @param  pM1: pointer to an array that contains PCM or PDM samples of the first channel (1 millisecond).
 * @param  pM2: pointer to an array that contains PCM or PDM samples of the second channel (1 millisecond).
 * @retval 0 if OK.
 * @note   Input/output function reads and write samples expect non interleaved data.
 */
uint32_t Cardoid_updateGain(Cardoid_Handler_t *pHandler, void *pM1, void *pM2);

/**
 * @brief  Library setup function, it sets the values for dynamic parameters. It can be called at runtime to change
 *         dynamic parameters.
 * @param  pHandler: pointer to the handler of the current Beamforming instance running.
 * @param  pConfig: pointer to the dynamic parameters handler containing the new library configuration.
 * @retval 0 if everything is fine.
 *         different from 0 if erroneous parameters have been passed to the setConfig function and the default
 *         value has been used. The specific error can be recognized by checking the relative bit in the returned word.
 */
uint32_t Cardoid_setConfig(Cardoid_Handler_t *pHandler, Cardoid_Config_t *pConfig);

/**
 * @brief  Fills the pConfig structure with the actual dynamic parameters as they are used inside the library.
 * @param  pHandler: pointer to the handler of the current Beamforming instance running.
 * @param  pConfig: pointer to the dynamic parameters handler that will be filled with the current library configuration.
 * @retval 0 if everything is fine.
 */
uint32_t Cardoid_getConfig(Cardoid_Handler_t *pHandler, Cardoid_Config_t *pConfig);

/**
 * @brief  To be used to retrieve version information.
 * @param  version char array to be filled with the current library version
 * @retval 0 if everything is fine.
*/
uint32_t Cardoid_getLibVersion(char *version);


/**
  * @}
  */

/**
* @}
*/

/**
  * @}
  */
#endif  /*__CARDOID_H*/

