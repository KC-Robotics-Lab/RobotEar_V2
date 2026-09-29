/**
******************************************************************************
* @file    acoustic_bf_cardoid.h
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
#ifndef __ACOUSTIC_BF_CARDOID_H
#define __ACOUSTIC_BF_CARDOID_H

/* Includes ------------------------------------------------------------------*/
#include "acoustic_bf_error.h"

/** @addtogroup MIDDLEWARES
* @{
*/

/** @defgroup ACOUSTIC_BF_CARDOID ACOUSTIC_BF_CARDOID
* @{
*/

/* Exported constants --------------------------------------------------------*/

/** @defgroup ACOUSTIC_BF_CARDOID_Exported_Constants AcousticBF_cardoid Exported Constants
* @{
*/

/** @defgroup ACOUSTIC_BF_CARDOID_data_format
* @brief    Cardoid data format
* @{
*/
#define ACOUSTIC_BF_CARDOID_DATA_FORMAT_PDM_MSB               ((uint32_t)0x00000000)      /* MSB is 0 to keep legacy */
#define ACOUSTIC_BF_CARDOID_DATA_FORMAT_PCM                   ((uint32_t)0x00000001)      /* PCM is 1 to keep legacy */
#define ACOUSTIC_BF_CARDOID_DATA_FORMAT_PDM_LSB               ((uint32_t)0x00000002)      /* LSB PDM samples */

/**
* @}
*/
/** @defgroup ACOUSTIC_BF_CARDOID_hw_ip
* @brief    Cardoid PDM data arrangement can depend on the HW ip chosen for data capture. PDM samples coming from I2S are captured into a 16-bit data register. In case of mono signal, changing the bit order on the I2S configuration will result in a temporal switch of two consecutive samples. The library needs to be aware, so it performs a byte reverting operation.
PDM samples coming from SAIPDM are captured into an 8-bit slots in the data register. In case of mono signal, changing the bit order will NOT result in a temporal switch of two consecutive samples. In this case, the byte reverting operation must NOT be done.
An API is required to support both cases.

* @{
*/
#define ACOUSTIC_BF_CARDOID_PDM_IP_SAI_PDM                   ((uint32_t)0x00000000)     /* Default */
#define ACOUSTIC_BF_CARDOID_PDM_IP_SPI_I2S                   ((uint32_t)0x00000001)
#define ACOUSTIC_BF_CARDOID_PDM_IP_SAI_I2S                   ((uint32_t)0x00000001)
/**
* @}
*/

/** @defgroup ACOUSTIC_BF_CARDOID_reference_channel
* @brief    Cardoid reference channel
* @{
*/
#define ACOUSTIC_BF_CARDOID_REF_DISABLE                       ((uint32_t)0x00000000)
#define ACOUSTIC_BF_CARDOID_REF_ENABLE                        ((uint32_t)0x00000001)
#define ACOUSTIC_BF_CARDOID_REF_RAW_MICROPHONE                ACOUSTIC_BF_CARDOID_REF_ENABLE
#define ACOUSTIC_BF_CARDOID_REF_OPPOSITE_ANTENNA              ((uint32_t)0x00000002)
/**
* @}
*/

/** @defgroup ACOUSTIC_BF_CARDOID_rear_enable
* @brief    Cardoid rear antenna activation
* @{
*/
#define ACOUSTIC_BF_CARDOID_REAR_DISABLE                       ((uint32_t)0x00000000)
#define ACOUSTIC_BF_CARDOID_REAR_ENABLE                        ((uint32_t)0x00000001)
/**
* @}
*/

/** @defgroup ACOUSTIC_BF_CARDOID_delay
* @brief    Cardoid delay
* @{
*/
#define ACOUSTIC_BF_CARDOID_DELAY_DISABLE                       ((uint32_t)0x00000000)
#define ACOUSTIC_BF_CARDOID_DELAY_ENABLE                        ((uint32_t)0x00000001)
/**
* @}
*/

/**
* @}
*/

/* Exported types ------------------------------------------------------------*/

/** @defgroup ACOUSTIC_BF_CARDOID_Exported_Types AcousticBF_cardoid Exported Types
* @{
*/
/**
 * @brief  Library handler. It keeps track of the static parameters
 *         and it handles the internal state of the algorithm.
 */
typedef struct
{
  uint32_t data_format;                         /*!< Specifies the data format for input: PDM or PCM. This parameter can be a value of @ref ACOUSTIC_BF_CARDOID_data_format. Default value is ACOUSTIC_BF_CARDOID_DATA_FORMAT_PDM */
  uint32_t sampling_frequency;                  /*!< Specifies the sampling frequency in KHz - can be 16 Hz for PCM,
                                                     1024 for PDM. This parameter can be a value of @ref ACOUSTIC_BF_CARDOID_data_format. Default value is ACOUSTIC_BF_CARDOID_DATA_FORMAT_PDM */
  uint8_t ptr_M1_channels;                      /*!< Number of channels in the stream of Microphone 1. Can be any integer > 0. Defualt value is 2*/
  uint8_t ptr_M2_channels;                      /*!< Number of channels in the stream of Microphone 2. Can be any integer > 0. Defualt value is 2 */
  uint8_t ptr_out_channels;                     /*!< Number of channels in the output stream. Can be any integer > 0. Defualt value is 2 */
  uint8_t rear_enable;                          /*!< Enable or disable the opposite antenna processing. This parameter can be a value of @ref ACOUSTIC_BF_CARDOID_rear_enable. Default value is ACOUSTIC_BF_CARDOID_REAR_DISABLE*/
  uint8_t delay_enable;                         /*! Enable the delay performed inside the library. In case of PDM input it MUST BE enabled. If PCM input and delay is disable, the user must make sure to align M1 & M2 samples */
  uint32_t internal_memory_size;                /*!< Keeps track of the amount of memory required for the current setup.
                                                     It's filled by the Beamforming_getMemorySize() function and must be
                                                     used to allocate the right amount of RAM */
  uint32_t *pInternalMemory;                    /*!< Pointer to the internal algorithm memory */

} AcousticBF_cardoid_Handler_t;

/**
 * @brief  Library dynamic configuration handler. It contains dynamic parameters.
 */
typedef struct
{
  uint16_t mic_distance;                        /*!< Distance between Mic1 and Mic2. It must be specified in tenths of a
                                                     millimeter. For example, if the microphone distance is equal to 4 mm,
                                                     this parameter must be initialized with the value 40. Default value is 150. */
  int16_t volume;                               /*!< Overall gain of the algorithm. It specifies the amound of gain added to the microphones, in dB.
                                                     It's used only when PDM input is chosen.*/
  uint8_t ref_select;                           /*!< Enable or disable the omnidirectional microphone reference or the opposite antenna
                                                     in the output stream. This parameter can be a value of @ref ACOUSTIC_BF_CARDOID_reference_channel. Default value is ACOUSTIC_BF_CARDOID_REF_ENABLE*/
  float M2_gain;                                /*!< Gain to be applied to the second microphone respect to the first one.
                                                     If set to 0, automatic gain is used */

}
AcousticBF_cardoid_Config_t;


/**
 * @brief  Library dynamic configuration handler. It contains dynamic parameters.
 */
typedef struct
{
  void      *pHdle;                                /*!< Post processing handler */
  uint32_t (*pCb)(void *const pPostProcHdle, int16_t *const pMic, int16_t *const pFront, int16_t *const pRear, int16_t *const pOut);  /*!< Post processing callback */

}
AcousticBF_cardoid_postProc_t;

/**
  * @}
  */

/* Exported macro ------------------------------------------------------------*/
/* Exported define -----------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

/** @defgroup ACOUSTIC_BF_CARDOID_Exported_Functions AcousticBF_cardoid Exported Functions
* @{
*/

/**
 * @brief  Fills the "internal_memory_size" of the pHandler parameter passed as argument with a value representing the
 *         right amount of memory needed by the library, depending on the specific static parameters adopted.
 * @param  pHandler: libBeamforming_Handler filled with desired parameters.
 * @retval 0 if everything is fine.
 */
uint32_t AcousticBF_cardoid_GetMemorySize(AcousticBF_cardoid_Handler_t *pHandler);

/**
 * @brief  Library initialization
 * @param  pHandler: AcousticBF_cardoid_Handler_t filled with desired parameters.
 * @retval 0 if everything is fine.
 *         different from 0 if erroneous parameters have been passed to the Init function and the default value has been used.
 *         The specific error can be recognized by checking the relative bit in the returned word.
 */
uint32_t AcousticBF_cardoid_Init(AcousticBF_cardoid_Handler_t *pHandler);

/**
 * @brief  Library data input/output
 * @param  pM1: pointer to an array that contains PCM or PDM samples of the first channel (1 millisecond).
 * @param  pM2: pointer to an array that contains PCM or PDM samples of the second channel (1 millisecond).
 * @param  pOut: pointer to an array that will contain PCM samples of output data (1 millisecond). If the reference channel
 *         is activated, two channels will be written in the output array.
 * @param  pHandler: pointer to the handler of the current Beamforming instance running.
 * @retval 1 if data collection is finished and Beamforming_SecondStep must be called, 0 otherwise.
 * @note   Input/output function reads and write samples skipping the required number of values depending on the
 *         ptr_Mx_channels configuration.
 */
uint32_t AcousticBF_cardoid_FirstStep(void *pM1, void *pM2, void *pOut, AcousticBF_cardoid_Handler_t *pHandler);

/**
 * @brief  Library run function, performs audio analysis when all required data has been collected.
 * @param  pHandler: pointer to the handler of the current beamforming instance running.
 * @retval 0 if everything is ok.
 */
uint32_t AcousticBF_cardoid_SecondStep(AcousticBF_cardoid_Handler_t *pHandler);

/**
 * @brief  Library setup function, it sets the values for dynamic parameters. It can be called at runtime to change
 *         dynamic parameters.
 * @param  pHandler: pointer to the handler of the current Beamforming instance running.
 * @param  pConfig: pointer to the dynamic parameters handler containing the new library configuration.
 * @retval 0 if everything is fine.
 *         different from 0 if erroneous parameters have been passed to the setConfig function and the default
 *         value has been used. The specific error can be recognized by checking the relative bit in the returned word.
 */
uint32_t AcousticBF_cardoid_SetConfig(AcousticBF_cardoid_Handler_t *pHandler, AcousticBF_cardoid_Config_t *pConfig);

/**
 * @brief  Fills the pConfig structure with the actual dynamic parameters as they are used inside the library.
 * @param  pHandler: pointer to the handler of the current Beamforming instance running.
 * @param  pConfig: pointer to the dynamic parameters handler that will be filled with the current library configuration.
 * @retval 0 if everything is fine.
 */
uint32_t AcousticBF_cardoid_GetConfig(AcousticBF_cardoid_Handler_t *pHandler, AcousticBF_cardoid_Config_t *pConfig);

/**
 * @brief  To be used to retrieve version information.
 * @param  version char array to be filled with the current library version
 * @retval 0 if everything is fine.
*/
uint32_t AcousticBF_cardoid_GetLibVersion(char *version);

/**
 * @brief  Register post filtering info if needed (optional).
 * @param  pHandler: pointer to the handler of the current Beamforming instance running.
 * @param  pPostProc: pointer to the post filtering handler & callback that will be called after cardoid is processed.
 * @retval 0 if everything is fine.
 */
uint32_t AcousticBF_cardoid_RegisterPostProc(AcousticBF_cardoid_Handler_t *pHandler, AcousticBF_cardoid_postProc_t *pPostProc);

/**
 * @brief  Specifies which HW IP is used (useless in case of PCM input otherwize optional in case SAI_PDM is used since it is the default setting).
 * Cardoid PDM data arrangement can depend on the HW ip chosen for data capture. PDM samples coming from I2S are captured into a 16-bit data register.
 * In case of mono signal, changing the bit order on the I2S configuration will result in a temporal switch of two consecutive samples. The library needs to be aware, so it performs a byte reverting operation.
 * PDM samples coming from SAIPDM are captured into an 8-bit slots in the data register. In case of mono signal, changing the bit order will NOT result in a temporal switch of two consecutive samples. In this case, the byte reverting operation must NOT be done.
 * @param  pHandler: pointer to the handler of the current Beamforming instance running.
 * @param  hwIP: pointer to the post filtering handler & callback that will be called after cardoid is processed.
 * @retval 0 if everything is fine.
 */
uint32_t AcousticBF_cardoid_SetHWIP(AcousticBF_cardoid_Handler_t *pHandler, uint32_t hwIP);



/**
  * @}
  */

/**
* @}
*/

/**
  * @}
  */
#endif  /*__ACOUSTIC_BF_CARDOID_H*/

