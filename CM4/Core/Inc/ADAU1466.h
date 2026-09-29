#ifndef INC_ADAU1466_H_
#define INC_ADAU1466_H_

#include "main.h"

#define ADAU1466_I2C_TIMEOUT_MS      	1000U

#define ADAU1466_ADDR7(addr1, addr0)	(uint16_t)(0x38U | (((addr1) & 0x1U) << 1) | ((addr0) & 0x1U))

#define ADAU1466_L_ADDR1             0U
#define ADAU1466_L_ADDR0             0U
#define ADAU1466_R_ADDR1             0U
#define ADAU1466_R_ADDR0             1U

#define ADAU1466_L_I2C_ADDR_8B       (ADAU1466_ADDR7(ADAU1466_L_ADDR1, ADAU1466_L_ADDR0) << 1)
#define ADAU1466_R_I2C_ADDR_8B       (ADAU1466_ADDR7(ADAU1466_R_ADDR1, ADAU1466_R_ADDR0) << 1)

HAL_StatusTypeDef ADAU1466_Write16(uint16_t reg, uint16_t val, uint8_t flag);
HAL_StatusTypeDef ADAU1466_Read16(uint16_t reg, uint16_t *val, uint8_t flag);
HAL_StatusTypeDef ADAU1466_WriteBytes(uint16_t reg, const uint8_t *buf, uint16_t len, uint8_t flag);
HAL_StatusTypeDef ADAU1466_ReadBytes(uint16_t reg, uint8_t *buf, uint16_t len, uint8_t flag);
HAL_StatusTypeDef ADAU1466_WaitPllLock(uint8_t flag, uint32_t timeout_ms);
HAL_StatusTypeDef ADAU1466_Init_4ch_16k_TDM(uint8_t flag);
HAL_StatusTypeDef ADAU1466_Init_FromSigmaStudio(uint8_t flag);
void ADAU1466_HwReset(uint8_t flag);
void adau1466_init(void);
void pll_read(void);

#endif /* INC_ADAU1466_H_ */
