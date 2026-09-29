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

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */
#define PCMD3180					0
#define ADAU1466					1
#define Audio_chip_sel				ADAU1466

#define USE_VOL						0

/*
 * Microphone board mode.
 * AUTO: L is required, R is optional and auto-detected at boot.
 * L_ONLY: never probe/start R even if it is connected.
 */
#define MIC_BOARD_MODE_AUTO             0U
#define MIC_BOARD_MODE_L_ONLY           1U
#ifndef MIC_BOARD_MODE
#define MIC_BOARD_MODE                  MIC_BOARD_MODE_L_ONLY
#endif

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
#if (USE_VOL == 1)
void CM4_AudioVolume_Init(void);
void CM4_AudioVolume_SetPercent(uint16_t gain_percent);
uint16_t CM4_AudioVolume_GetPercent(void);
void CM4_AudioVolume_Enable(uint8_t enable);
uint8_t CM4_AudioVolume_IsEnabled(void);
#endif
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define Red_LED_Pin GPIO_PIN_14
#define Red_LED_GPIO_Port GPIOB
#define PCMD_L_RST_Pin GPIO_PIN_5
#define PCMD_L_RST_GPIO_Port GPIOD
#define PCMD_R_RST_Pin GPIO_PIN_6
#define PCMD_R_RST_GPIO_Port GPIOD

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
