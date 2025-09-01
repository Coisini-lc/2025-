/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

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

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define Blue_TX_Pin GPIO_PIN_2
#define Blue_TX_GPIO_Port GPIOA
#define Blue_RX_Pin GPIO_PIN_3
#define Blue_RX_GPIO_Port GPIOA
#define K1_Pin GPIO_PIN_4
#define K1_GPIO_Port GPIOA
#define K2_Pin GPIO_PIN_5
#define K2_GPIO_Port GPIOA
#define K3_Pin GPIO_PIN_6
#define K3_GPIO_Port GPIOA
#define K4_Pin GPIO_PIN_7
#define K4_GPIO_Port GPIOA
#define K5_Pin GPIO_PIN_0
#define K5_GPIO_Port GPIOB
#define K6_Pin GPIO_PIN_1
#define K6_GPIO_Port GPIOB
#define K7_Pin GPIO_PIN_2
#define K7_GPIO_Port GPIOB
#define LED3_Pin GPIO_PIN_13
#define LED3_GPIO_Port GPIOB
#define LED2_Pin GPIO_PIN_14
#define LED2_GPIO_Port GPIOB
#define LED1_Pin GPIO_PIN_15
#define LED1_GPIO_Port GPIOB
#define TFT_TX_Pin GPIO_PIN_9
#define TFT_TX_GPIO_Port GPIOA
#define TFT_RX_Pin GPIO_PIN_10
#define TFT_RX_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
