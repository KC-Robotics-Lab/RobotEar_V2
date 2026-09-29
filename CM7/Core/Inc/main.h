/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
typedef struct
{
	uint32_t	Flag_1ms	:1;
	uint32_t	Flag_10ms	:1;
	uint32_t	Flag_20ms	:1;
	uint32_t	Flag_40ms	:1;
	uint32_t	Flag_50ms	:1;
	uint32_t	Flag_60ms	:1;
	uint32_t	Flag_80ms	:1;
	uint32_t	Flag_100ms	:1;
	uint32_t	Flag_200ms	:1;
	uint32_t	Flag_250ms	:1;
	uint32_t	Flag_500ms	:1;
	uint32_t	Flag_1000ms	:1;
	uint32_t	Count_1ms;
} systick;
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
uint8_t CDC_Transmit_FS_Blocking(uint8_t *Buf, uint16_t Len, uint32_t timeout_ms);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define Green_LED_Pin GPIO_PIN_0
#define Green_LED_GPIO_Port GPIOB
#define LED_L_135_B_Pin GPIO_PIN_9
#define LED_L_135_B_GPIO_Port GPIOE
#define LED_R_135_R_Pin GPIO_PIN_10
#define LED_R_135_R_GPIO_Port GPIOE
#define LED_180_R_Pin GPIO_PIN_12
#define LED_180_R_GPIO_Port GPIOE
#define LED_L_45_R_Pin GPIO_PIN_13
#define LED_L_45_R_GPIO_Port GPIOE
#define LED_L_135_R_Pin GPIO_PIN_14
#define LED_L_135_R_GPIO_Port GPIOE
#define LED_L_90_R_Pin GPIO_PIN_15
#define LED_L_90_R_GPIO_Port GPIOE
#define LED_R_90_B_Pin GPIO_PIN_10
#define LED_R_90_B_GPIO_Port GPIOD
#define LED_R_90_R_Pin GPIO_PIN_11
#define LED_R_90_R_GPIO_Port GPIOD
#define LED_R_45_R_Pin GPIO_PIN_12
#define LED_R_45_R_GPIO_Port GPIOD
#define LED_0_R_Pin GPIO_PIN_13
#define LED_0_R_GPIO_Port GPIOD
#define LED_L_90_B_Pin GPIO_PIN_15
#define LED_L_90_B_GPIO_Port GPIOD
#define LED_L_45_B_Pin GPIO_PIN_6
#define LED_L_45_B_GPIO_Port GPIOG
#define LED_180_B_Pin GPIO_PIN_7
#define LED_180_B_GPIO_Port GPIOG
#define LED_R_135_B_Pin GPIO_PIN_8
#define LED_R_135_B_GPIO_Port GPIOG
#define LED_0_B_Pin GPIO_PIN_14
#define LED_0_B_GPIO_Port GPIOG
#define LED_R_45_B_Pin GPIO_PIN_0
#define LED_R_45_B_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
