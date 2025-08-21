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
#include "stm32f4xx_hal.h"

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

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define Segment_Dp_Pin GPIO_PIN_0
#define Segment_Dp_GPIO_Port GPIOC
#define Segment_C_Pin GPIO_PIN_1
#define Segment_C_GPIO_Port GPIOC
#define Segment_E_Pin GPIO_PIN_2
#define Segment_E_GPIO_Port GPIOC
#define Segment_D_Pin GPIO_PIN_3
#define Segment_D_GPIO_Port GPIOC
#define Traffic_Led1_Pin GPIO_PIN_0
#define Traffic_Led1_GPIO_Port GPIOA
#define Traffic_Led2_Pin GPIO_PIN_1
#define Traffic_Led2_GPIO_Port GPIOA
#define Poten_Pin GPIO_PIN_4
#define Poten_GPIO_Port GPIOA
#define Light_Up_Pin GPIO_PIN_5
#define Light_Up_GPIO_Port GPIOA
#define Light_Up_EXTI_IRQn EXTI9_5_IRQn
#define Light_Down_Pin GPIO_PIN_6
#define Light_Down_GPIO_Port GPIOA
#define Light_Down_EXTI_IRQn EXTI9_5_IRQn
#define Illu_Pin GPIO_PIN_7
#define Illu_GPIO_Port GPIOA
#define Segment_B_Pin GPIO_PIN_4
#define Segment_B_GPIO_Port GPIOC
#define Segment_F_Pin GPIO_PIN_5
#define Segment_F_GPIO_Port GPIOC
#define Segment_G_Pin GPIO_PIN_6
#define Segment_G_GPIO_Port GPIOC
#define Segment_A_Pin GPIO_PIN_7
#define Segment_A_GPIO_Port GPIOC
#define Led1_Pin GPIO_PIN_8
#define Led1_GPIO_Port GPIOC
#define Led2_Pin GPIO_PIN_9
#define Led2_GPIO_Port GPIOC
#define Traffic_Led3_Pin GPIO_PIN_5
#define Traffic_Led3_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
