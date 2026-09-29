#include "PCMD3180.h"
#include "i2c.h"
#include "gpio.h"
#include <stdlib.h>
#include <stdint.h>

//__attribute__((section(".ram_d3"), aligned(32))) uint8_t read_buf[200];

HAL_StatusTypeDef PCMD3180_Write8(uint16_t reg, uint8_t val, uint8_t flag)
{
	if(flag == 0)
		return HAL_I2C_Mem_Write(&hi2c2, PCMD3180_L_I2C_ADDR_8B, (uint16_t)reg, I2C_MEMADD_SIZE_8BIT, &val, 1, PCMD3180_I2C_TIMEOUT_MS);
	else
		return HAL_I2C_Mem_Write(&hi2c4, PCMD3180_R_I2C_ADDR_8B, (uint16_t)reg, I2C_MEMADD_SIZE_8BIT, &val, 1, PCMD3180_I2C_TIMEOUT_MS);
}

HAL_StatusTypeDef PCMD3180_Read8(uint16_t reg, uint8_t *val, uint8_t flag)
{
	if(flag == 0)
		return HAL_I2C_Mem_Read(&hi2c2, PCMD3180_L_I2C_ADDR_8B, (uint16_t)reg, I2C_MEMADD_SIZE_8BIT, val, 1, PCMD3180_I2C_TIMEOUT_MS);
	else
		return HAL_I2C_Mem_Read(&hi2c4, PCMD3180_R_I2C_ADDR_8B, (uint16_t)reg, I2C_MEMADD_SIZE_8BIT, val, 1, PCMD3180_I2C_TIMEOUT_MS);
}

HAL_StatusTypeDef PCMD3180_WriteBytes(uint16_t reg, const uint8_t *buf, uint16_t len, uint8_t flag)
{
	if(flag == 0)
		return HAL_I2C_Mem_Write(&hi2c2, PCMD3180_L_I2C_ADDR_8B, reg, I2C_MEMADD_SIZE_8BIT, (uint8_t*)buf, len, PCMD3180_I2C_TIMEOUT_MS);
	else
		return HAL_I2C_Mem_Write(&hi2c4, PCMD3180_R_I2C_ADDR_8B, reg, I2C_MEMADD_SIZE_8BIT, (uint8_t*)buf, len, PCMD3180_I2C_TIMEOUT_MS);
}

HAL_StatusTypeDef PCMD3180_ReadBytes(uint16_t reg, uint8_t *buf, uint16_t len, uint8_t flag)
{
	if(flag == 0)
		return HAL_I2C_Mem_Read(&hi2c2, PCMD3180_L_I2C_ADDR_8B, reg, I2C_MEMADD_SIZE_8BIT, buf, len, PCMD3180_I2C_TIMEOUT_MS);
	else
		return HAL_I2C_Mem_Read(&hi2c4, PCMD3180_R_I2C_ADDR_8B, reg, I2C_MEMADD_SIZE_8BIT, buf, len, PCMD3180_I2C_TIMEOUT_MS);
}

void pcmd3180_init(void)
{
	HAL_GPIO_WritePin(PCMD_L_RST_GPIO_Port, PCMD_L_RST_Pin, GPIO_PIN_RESET);
	HAL_Delay(1);
	HAL_GPIO_WritePin(PCMD_L_RST_GPIO_Port, PCMD_L_RST_Pin, GPIO_PIN_SET);
	HAL_Delay(1);

	PCMD3180_Write8(0x00, 0x00, 0);
	PCMD3180_Write8(0x02, 0x81, 0);
	HAL_Delay(1);

	PCMD3180_Write8(0x07, 0x00, 0);

	PCMD3180_Write8(0x0B, 0x00, 0); // CH1 -> L0
	PCMD3180_Write8(0x0C, 0x01, 0); // CH2 -> R0
	PCMD3180_Write8(0x0D, 0x02, 0); // CH3 -> L1
	PCMD3180_Write8(0x0E, 0x03, 0); // CH4 -> R1

	PCMD3180_Write8(0x3C, 0x40, 0);
	PCMD3180_Write8(0x41, 0x40, 0);
	PCMD3180_Write8(0x46, 0x40, 0);
	PCMD3180_Write8(0x4B, 0x40, 0);

	PCMD3180_Write8(0x3E, 0xEB, 0);
	PCMD3180_Write8(0x43, 0xEB, 0);
	PCMD3180_Write8(0x48, 0xEB, 0);
	PCMD3180_Write8(0x4D, 0xEB, 0);

	PCMD3180_Write8(0x22, 0x41, 0);
	PCMD3180_Write8(0x23, 0x41, 0);

	PCMD3180_Write8(0x2B, 0x45, 0);

	PCMD3180_Write8(0x73, 0xF0, 0);
	PCMD3180_Write8(0x74, 0xF0, 0);

	PCMD3180_Write8(0x75, 0xE0, 0);

	HAL_GPIO_WritePin(PCMD_R_RST_GPIO_Port, PCMD_R_RST_Pin, GPIO_PIN_RESET);
	HAL_Delay(1);
	HAL_GPIO_WritePin(PCMD_R_RST_GPIO_Port, PCMD_R_RST_Pin, GPIO_PIN_SET);
	HAL_Delay(1);

	PCMD3180_Write8(0x00, 0x00, 1);
	PCMD3180_Write8(0x02, 0x81, 1);
	HAL_Delay(1);

	PCMD3180_Write8(0x07, 0x00, 1);

	PCMD3180_Write8(0x0B, 0x00, 1); // CH1 -> L0
	PCMD3180_Write8(0x0C, 0x01, 1); // CH2 -> R0
	PCMD3180_Write8(0x0D, 0x02, 1); // CH3 -> L1
	PCMD3180_Write8(0x0E, 0x03, 1); // CH4 -> R1

	PCMD3180_Write8(0x3C, 0x40, 1);
	PCMD3180_Write8(0x41, 0x40, 1);
	PCMD3180_Write8(0x46, 0x40, 1);
	PCMD3180_Write8(0x4B, 0x40, 1);

	PCMD3180_Write8(0x3E, 0xEB, 1);
	PCMD3180_Write8(0x43, 0xEB, 1);
	PCMD3180_Write8(0x48, 0xEB, 1);
	PCMD3180_Write8(0x4D, 0xEB, 1);

	PCMD3180_Write8(0x22, 0x41, 1);
	PCMD3180_Write8(0x23, 0x41, 1);

	PCMD3180_Write8(0x2B, 0x45, 1);

	PCMD3180_Write8(0x73, 0xF0, 1);
	PCMD3180_Write8(0x74, 0xF0, 1);

	PCMD3180_Write8(0x75, 0xE0, 1);

//	PCMD3180_ReadBytes(0x00, read_buf, 128, 0);
}

