/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "i2c.h"
#include "sai.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "shared_audio.h"
#include "PCMD3180.h"
//#include "audio_bf_pcm.h"
#include <string.h>
#include "ADAU1466.h"
#include "CLI.h"
#include "arm_math.h"
#include "adau1466_sigmadsp_image.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* DUAL_CORE_BOOT_SYNC_SEQUENCE: Define for dual core boot synchronization    */
/*                             demonstration code based on hardware semaphore */
/* This define is present in both CM7/CM4 projects                            */
/* To comment when developping/debugging on a single core                     */
#define DUAL_CORE_BOOT_SYNC_SEQUENCE

#if defined(DUAL_CORE_BOOT_SYNC_SEQUENCE)
#ifndef HSEM_ID_0
#define HSEM_ID_0 (0U) /* HW semaphore 0*/
#endif
#endif /* DUAL_CORE_BOOT_SYNC_SEQUENCE */

#define HSEM_AUDIO_ID   (1U) // 오디오 프레임 ready 신호용

/*
 * CM4 CMSIS-DSP digital gain.
 * 100 = 1.0x (0 dB), 200 = 2.0x (about +6 dB).
 * Start conservatively because clipping degrades SRP-PHAT/GCC-PHAT.
 */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#if (USE_VOL == 1)
#define CM4_SAI_VOLUME_DEFAULT_PERCENT   (500U)
#define CM4_SAI_VOLUME_MAX_PERCENT       (800U)
#endif
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
#define SAI_RX_BUF_LEN               (SHARED_HOP_WORDS * 2U)
//uint16_t SAI_L_Rx_Buffer[SAI_RX_BUF_LEN];
//uint16_t SAI_R_Rx_Buffer[SAI_RX_BUF_LEN];
uint16_t SAI_L_Rx_Buffer[SAI_RX_BUF_LEN] __attribute__((aligned(32)));
uint16_t SAI_R_Rx_Buffer[SAI_RX_BUF_LEN] __attribute__((aligned(32)));

__attribute__((section(".shared"), aligned(32))) shared_audio_frame_t g_shared_audio;

#if (USE_VOL == 1)
/* CMSIS-DSP arm_scale_q15 parameters. */
static volatile q15_t  g_cm4_sai_volume_scale_fract = (q15_t)0x4000; /* 0.5 */
static volatile int8_t g_cm4_sai_volume_shift       = 2;            /* 0.5 * 2^2 = 2.0x */
static volatile uint16_t g_cm4_sai_volume_percent   = CM4_SAI_VOLUME_DEFAULT_PERCENT;
static volatile uint8_t  g_cm4_sai_volume_enabled   = 1U;
#endif

/* Debug watch variable: HAL_OK means the full SigmaStudio image loaded. */
volatile HAL_StatusTypeDef g_adau1466_sigma_load_L_status = HAL_ERROR;
volatile HAL_StatusTypeDef g_adau1466_sigma_load_R_status = HAL_ERROR;

/* Runtime board/stream state for CubeIDE Watch. */
volatile uint32_t g_audio_active_stream_mask = 0U;
volatile uint32_t g_r_mic_board_detected = 0U;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int __io_getchar(void)
{
	unsigned char ch;
	HAL_UART_Receive(&huart4, (uint8_t *)&ch, 1, 0xFFFF);
	return ch;
}

int __io_putchar(int ch)
{
     (void) HAL_UART_Transmit(&huart4, (uint8_t*) &ch, 1, HAL_MAX_DELAY);
     return ch;
}

/* USER CODE BEGIN 0 */
#if (USE_VOL == 1)
void CM4_AudioVolume_Init(void)
{
    CM4_AudioVolume_SetPercent(CM4_SAI_VOLUME_DEFAULT_PERCENT);
    CM4_AudioVolume_Enable(1U);
}

void CM4_AudioVolume_SetPercent(uint16_t gain_percent)
{
    uint64_t gain_q15;
    q15_t scale_fract;
    int8_t shift = 0;
    uint32_t primask;

    if (gain_percent > CM4_SAI_VOLUME_MAX_PERCENT)
    {
        gain_percent = CM4_SAI_VOLUME_MAX_PERCENT;
    }

    if (gain_percent == 0U)
    {
        scale_fract = 0;
        shift = 0;
    }
    else
    {
        /*
         * Convert percent to a Q15-like value that may be greater than 1.0.
         * arm_scale_q15 effective gain is:
         *      scale_fract / 32768 * 2^shift
         */
        gain_q15 = (((uint64_t)gain_percent * 32768ULL) + 50ULL) / 100ULL;

        while ((gain_q15 > 32767ULL) && (shift < 15))
        {
            gain_q15 = (gain_q15 + 1ULL) >> 1;
            shift++;
        }

        if (gain_q15 > 32767ULL)
        {
            gain_q15 = 32767ULL;
        }

        scale_fract = (q15_t)gain_q15;
    }

    /* Keep coefficient and shift consistent against DMA callbacks. */
    primask = __get_PRIMASK();
    __disable_irq();
    g_cm4_sai_volume_scale_fract = scale_fract;
    g_cm4_sai_volume_shift = shift;
    g_cm4_sai_volume_percent = gain_percent;
    if (primask == 0U)
    {
        __enable_irq();
    }
}

uint16_t CM4_AudioVolume_GetPercent(void)
{
    return g_cm4_sai_volume_percent;
}

void CM4_AudioVolume_Enable(uint8_t enable)
{
    g_cm4_sai_volume_enabled = (enable != 0U) ? 1U : 0U;
}

uint8_t CM4_AudioVolume_IsEnabled(void)
{
    return g_cm4_sai_volume_enabled;
}

static void CM4_AudioVolume_ProcessQ15(const uint16_t *src,
                                      volatile uint16_t *dst,
                                      uint32_t sample_count)
{
    q15_t scale_fract;
    int8_t shift;

    if ((src == NULL) || (dst == NULL) || (sample_count == 0U))
    {
        return;
    }

    if ((g_cm4_sai_volume_enabled == 0U) ||
        (g_cm4_sai_volume_percent == 100U))
    {
        memcpy((void *)dst, (const void *)src, sizeof(uint16_t) * sample_count);
        return;
    }

    if (g_cm4_sai_volume_percent == 0U)
    {
        memset((void *)dst, 0, sizeof(uint16_t) * sample_count);
        return;
    }

    scale_fract = g_cm4_sai_volume_scale_fract;
    shift = g_cm4_sai_volume_shift;

    /*
     * SAI words are signed 16-bit PCM (Q15 bit pattern).
     * arm_scale_q15 uses the Cortex-M4 DSP instructions and saturates to
     * -32768...32767, preventing integer wraparound.
     */
    arm_scale_q15((const q15_t *)src,
                  scale_fract,
                  shift,
                  (q15_t *)(void *)dst,
                  sample_count);
}
#endif

static void AudioShared_SetActiveStreamMask(uint32_t mask)
{
    mask &= AUDIO_STREAM_MASK_ALL;
    g_audio_active_stream_mask = mask;
    g_shared_audio.active_stream_mask = mask;
    __DMB();
    g_shared_audio.config_seq++;
    __DMB();
}

static void SAI1_DMA_Start_L(void)
{
    if (HAL_SAI_Receive_DMA(&hsai_BlockA1,
                            (uint8_t *)SAI_L_Rx_Buffer,
                            SAI_RX_BUF_LEN) != HAL_OK)
    {
        Error_Handler();
    }
}

static void SAI1_DMA_Start_R(void)
{
    if (HAL_SAI_Receive_DMA(&hsai_BlockB1,
                            (uint8_t *)SAI_R_Rx_Buffer,
                            SAI_RX_BUF_LEN) != HAL_OK)
    {
        Error_Handler();
    }
}

void SAI1_DMA_Start(void)
{
    /* Legacy two-board start. B is synchronized to A, so arm B first. */
    AudioShared_SetActiveStreamMask(AUDIO_STREAM_MASK_ALL);
    SAI1_DMA_Start_R();
    SAI1_DMA_Start_L();
}

static void AudioShared_NotifyCM7(void)
{
    /*
     * HSEM is used only as a notification.
     */
    if (HAL_HSEM_FastTake(HSEM_AUDIO_ID) == HAL_OK)
    {
        HAL_HSEM_Release(HSEM_AUDIO_ID, 0);
    }
}

static void AudioShared_PublishSlot(uint32_t stream_id, uint32_t slot, const uint16_t *src)
{
    uint32_t offset;
    volatile shared_audio_stream_t *stream;

    if ((src == NULL) || (stream_id >= AUDIO_STREAM_COUNT))
    {
        return;
    }

    if (slot == SHARED_SLOT_HALF)
    {
        offset = SHARED_HALF_OFFSET;
    }
    else if (slot == SHARED_SLOT_FULL)
    {
        offset = SHARED_FULL_OFFSET;
    }
    else
    {
        return;
    }

    stream = &g_shared_audio.stream[stream_id];

    stream->slot_guard[slot]++;
    __DMB();

#if (USE_VOL == 0)
    memcpy((void *)&stream->samples[offset], (const void *)src, sizeof(uint16_t) * SHARED_HOP_WORDS);
#else
    CM4_AudioVolume_ProcessQ15(src, &stream->samples[offset], SHARED_HOP_WORDS);
#endif

    __DMB();

    stream->slot_seq[slot]++;
    stream->slot_guard[slot]++;
    stream->notify_seq++;

    __DMB();

    AudioShared_NotifyCM7();
}

void HAL_SAI_RxHalfCpltCallback(SAI_HandleTypeDef *hsai)
{
    if ((hsai->Instance == SAI1_Block_A) &&
        AUDIO_STREAM_MASK_HAS(g_audio_active_stream_mask, AUDIO_STREAM_L))
    {
        AudioShared_PublishSlot(AUDIO_STREAM_L, SHARED_SLOT_HALF, &SAI_L_Rx_Buffer[0]);
    }
    else if ((hsai->Instance == SAI1_Block_B) &&
             AUDIO_STREAM_MASK_HAS(g_audio_active_stream_mask, AUDIO_STREAM_R))
    {
        AudioShared_PublishSlot(AUDIO_STREAM_R, SHARED_SLOT_HALF, &SAI_R_Rx_Buffer[0]);
    }
}

void HAL_SAI_RxCpltCallback(SAI_HandleTypeDef *hsai)
{
    if ((hsai->Instance == SAI1_Block_A) &&
        AUDIO_STREAM_MASK_HAS(g_audio_active_stream_mask, AUDIO_STREAM_L))
    {
        AudioShared_PublishSlot(AUDIO_STREAM_L, SHARED_SLOT_FULL, &SAI_L_Rx_Buffer[SHARED_HOP_WORDS]);
    }
    else if ((hsai->Instance == SAI1_Block_B) &&
             AUDIO_STREAM_MASK_HAS(g_audio_active_stream_mask, AUDIO_STREAM_R))
    {
        AudioShared_PublishSlot(AUDIO_STREAM_R, SHARED_SLOT_FULL, &SAI_R_Rx_Buffer[SHARED_HOP_WORDS]);
    }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

/* USER CODE BEGIN Boot_Mode_Sequence_1 */
#if defined(DUAL_CORE_BOOT_SYNC_SEQUENCE)
  /*HW semaphore Clock enable*/
  __HAL_RCC_HSEM_CLK_ENABLE();
  /* Activate HSEM notification for Cortex-M4*/
  HAL_HSEM_ActivateNotification(__HAL_HSEM_SEMID_TO_MASK(HSEM_ID_0));
  /*
  Domain D2 goes to STOP mode (Cortex-M4 in deep-sleep) waiting for Cortex-M7 to
  perform system initialization (system clock config, external memory configuration.. )
  */
  HAL_PWREx_ClearPendingEvent();
  HAL_PWREx_EnterSTOPMode(PWR_MAINREGULATOR_ON, PWR_STOPENTRY_WFE, PWR_D2_DOMAIN);
  /* Clear HSEM flag */
  __HAL_HSEM_CLEAR_FLAG(__HAL_HSEM_SEMID_TO_MASK(HSEM_ID_0));

#endif /* DUAL_CORE_BOOT_SYNC_SEQUENCE */
/* USER CODE END Boot_Mode_Sequence_1 */
  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_SAI1_Init();
  MX_I2C2_Init();
  MX_I2C4_Init();
  MX_UART4_Init();
  /* USER CODE BEGIN 2 */
  UART4_CLI_Start();
#if (USE_VOL == 1)
  CM4_AudioVolume_Init();
#endif

#if(Audio_chip_sel == PCMD3180)
  UART4_CLI_SendString("\r\n[PCMD3180] init start\r\n");
  pcmd3180_init();
  SAI1_DMA_Start();
  UART4_CLI_SendString("[PCMD3180] init done\r\n");

  HAL_GPIO_WritePin(Red_LED_GPIO_Port, Red_LED_Pin, GPIO_PIN_RESET);
#else
//  adau1466_init();
//  SAI1_DMA_Start();
//  HAL_GPIO_WritePin(Red_LED_GPIO_Port, Red_LED_Pin, GPIO_PIN_SET);

  HAL_GPIO_WritePin(Red_LED_GPIO_Port, Red_LED_Pin, GPIO_PIN_RESET);
  UART4_CLI_SendString("\r\n[ADAU1466] SigmaStudio I2C load start\r\n");
  /* L is mandatory. R is optional in AUTO mode. */
  AudioShared_SetActiveStreamMask(0U);
  g_r_mic_board_detected = 0U;
  g_adau1466_sigma_load_L_status = ADAU1466_Init_FromSigmaStudio(0);

  if (g_adau1466_sigma_load_L_status == HAL_OK)
  {
      uint32_t active_mask = AUDIO_STREAM_MASK_L;

#if (MIC_BOARD_MODE == MIC_BOARD_MODE_AUTO)
      UART4_CLI_SendString("[ADAU1466] probing optional R board...\r\n");
      g_adau1466_sigma_load_R_status = ADAU1466_Init_FromSigmaStudio(1);

      if (g_adau1466_sigma_load_R_status == HAL_OK)
      {
          active_mask |= AUDIO_STREAM_MASK_R;
          g_r_mic_board_detected = 1U;
          UART4_CLI_SendString("[ADAU1466] R board detected -> L+R mode\r\n");
      }
      else
      {
          UART4_CLI_SendString("[ADAU1466] R board not detected -> L-only mode\r\n");
      }
#else
      g_adau1466_sigma_load_R_status = HAL_ERROR;
      UART4_CLI_SendString("[ADAU1466] forced L-only mode\r\n");
#endif

      /* Publish mode before DMA starts so callbacks are accepted immediately. */
      AudioShared_SetActiveStreamMask(active_mask);

      /* B is synchronized to A. In L+R mode arm B first, then A. */
      if (AUDIO_STREAM_MASK_HAS(active_mask, AUDIO_STREAM_R))
      {
          SAI1_DMA_Start_R();
      }
      SAI1_DMA_Start_L();

      UART4_CLI_SendString("[ADAU1466] SAI1 DMA started\r\n");
  }
  else
  {
      AudioShared_SetActiveStreamMask(0U);
      HAL_GPIO_WritePin(Red_LED_GPIO_Port, Red_LED_Pin, GPIO_PIN_SET);
      UART4_CLI_SendString("[ADAU1466] L board image load FAILED - audio disabled\r\n");
  }
#endif

  UART4_CLI_PrintPrompt();


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
#if(Audio_chip_sel == ADAU1466)
	  UART4_CLI_Process();
#endif
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SAI1;
  PeriphClkInitStruct.PLL3.PLL3M = 8;
  PeriphClkInitStruct.PLL3.PLL3N = 28;
  PeriphClkInitStruct.PLL3.PLL3P = 55;
  PeriphClkInitStruct.PLL3.PLL3Q = 2;
  PeriphClkInitStruct.PLL3.PLL3R = 2;
  PeriphClkInitStruct.PLL3.PLL3RGE = RCC_PLL3VCIRANGE_3;
  PeriphClkInitStruct.PLL3.PLL3VCOSEL = RCC_PLL3VCOWIDE;
  PeriphClkInitStruct.PLL3.PLL3FRACN = 1006;
  PeriphClkInitStruct.Sai1ClockSelection = RCC_SAI1CLKSOURCE_PLL3;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
