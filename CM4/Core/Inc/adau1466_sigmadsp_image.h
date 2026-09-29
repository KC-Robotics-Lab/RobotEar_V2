#ifndef ADAU1466_SIGMADSP_IMAGE_H
#define ADAU1466_SIGMADSP_IMAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"
#include <stdint.h>

#ifndef ADAU1466_SIGMA_I2C_TIMEOUT_MS
#define ADAU1466_SIGMA_I2C_TIMEOUT_MS  1000U
#endif

/* Debug variables: inspect these if the download fails. */
extern volatile uint16_t g_adau1466_sigma_last_address;
extern volatile HAL_StatusTypeDef g_adau1466_sigma_last_status;

/**
 * @brief Load the complete SigmaStudio image through the ADAU1466 I2C port.
 *
 * @param hi2c STM32 HAL I2C handle connected to the ADAU1466.
 * @param device_address_8bit STM32 HAL-style address (7-bit address << 1).
 * @return HAL_OK on success; otherwise the first HAL error returned.
 */
HAL_StatusTypeDef ADAU1466_LoadSigmaStudioImage(I2C_HandleTypeDef *hi2c, uint16_t device_address_8bit, uint8_t flag);

#ifdef __cplusplus
}
#endif

#endif /* ADAU1466_SIGMADSP_IMAGE_H */
