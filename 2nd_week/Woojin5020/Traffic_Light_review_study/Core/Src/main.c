/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#ifdef __GNUC__
/* With GCC, small printf (option LD Linker->Libraries->Small printf
   set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
volatile unsigned int time3Cnt = 0;
volatile unsigned int time2Cnt = 0;
volatile uint8_t stateFlag = 0;
volatile uint8_t SegmentFlag = 10;
volatile uint8_t resetSegmentFlag = 0;
volatile uint8_t btn_flag = 0;

uint32_t pulse = 1000;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  if(HAL_TIM_Base_Start_IT(&htim2) != HAL_OK)
      Error_Handler();
  if(HAL_TIM_Base_Start_IT(&htim3) != HAL_OK)
    Error_Handler();

  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);

  HAL_ADC_Start_IT(&hadc1);

  printf("Start main()!!\r\n");

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  switch(stateFlag)
	  {
	  	  case 1: // 0~19s: 교통 Green ON, 보행자 Red ON
	  		  __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_1, pulse); // Green ON
	  		  __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, 0);   // Yellow OFF
	  		  __HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_2, 0);   // Traffic Red OFF
	  		  __HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_3, 0);   // Ped Green OFF
	  		  __HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_4, pulse); // Ped Red ON
	          break;

	  	  case 2: // 19~20s: 교통 Green OFF, Yellow ON
	  		  __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_1, 0);   // Green OFF
	  		  __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, pulse); // Yellow ON
	  		  break;

	      case 3: // 20~27s: 교통 Red ON, 보행자 Green ON
	    	  __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, 0);   // Yellow OFF
	    	  __HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_2, pulse); // Traffic Red ON
	    	  __HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_3, pulse); // Ped Green ON
	    	  __HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_4, 0);   // Ped Red OFF
	    	  break;

	      case 4: // 27~30s: 보행자 Green 점멸
	          if((time3Cnt % 1000) < 500)
	        	  __HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_3, pulse); // ON
	          else
	        	  __HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_3, 0);   // OFF
	              break;

	      case 5: // 30s: 초기 상태로 복귀
	    	  __HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_3, 0);   // Ped Green OFF
	    	  __HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_4, pulse); // Ped Red ON
	    	  __HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_2, 0);   // Traffic Red OFF
	    	  __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_1, pulse); // Traffic Green ON
	          break;
	  }
	  switch(btn_flag)
	  {
	  	  case 1:
	  		if(pulse <= 900)
	  		{
	  			pulse += 100;   // 최대값 제한 필요하면 추가
	  			printf("btn2_pulse : %d\r\n\n",pulse);
	  			btn_flag = 0;  // 플래그 클리어
	  		}
	  		break;
	  	  case 2:
	  		if (pulse >= 100)
	  		{
	  			pulse -= 100;  // 음수 방지
	  			printf("btn1_pulse : %d\r\n\n",pulse);
	  			btn_flag = 0;  // 플래그 클리어
	  		}
	  		break;
	  }
	  if(resetSegmentFlag)
	  {
		  HAL_GPIO_WritePin(GPIOC, Segment_Dp_Pin|Segment_C_Pin|Segment_E_Pin|Segment_D_Pin
		                            |Segment_B_Pin|Segment_F_Pin|Segment_G_Pin|Segment_A_Pin, GPIO_PIN_RESET);
		  resetSegmentFlag = 0;
	  }
	  switch(SegmentFlag)
	  {
	  	  case 1:
	  		  HAL_GPIO_WritePin(GPIOC, Segment_C_Pin|Segment_B_Pin, GPIO_PIN_SET);
	  		  break;
	  	  case 2:
	  		  HAL_GPIO_WritePin(GPIOC, Segment_A_Pin|Segment_B_Pin|Segment_G_Pin|Segment_E_Pin|Segment_D_Pin, GPIO_PIN_SET);
	  		  break;
	  	  case 3:
	  		  HAL_GPIO_WritePin(GPIOC, Segment_A_Pin|Segment_B_Pin|Segment_G_Pin|Segment_C_Pin|Segment_D_Pin, GPIO_PIN_SET);
	  		  break;
	  	  case 4:
	  		  HAL_GPIO_WritePin(GPIOC, Segment_F_Pin|Segment_B_Pin|Segment_G_Pin|Segment_C_Pin, GPIO_PIN_SET);
	  		  break;
	  	  case 5:
	  		  HAL_GPIO_WritePin(GPIOC, Segment_A_Pin|Segment_F_Pin|Segment_G_Pin|Segment_C_Pin|Segment_D_Pin, GPIO_PIN_SET);
	  		  break;
	  	  case 6:
	  		  HAL_GPIO_WritePin(GPIOC, Segment_A_Pin|Segment_F_Pin|Segment_G_Pin|Segment_E_Pin|Segment_C_Pin|Segment_D_Pin, GPIO_PIN_SET);
	  		  break;
	  	  case 7:
	  		  HAL_GPIO_WritePin(GPIOC, Segment_F_Pin|Segment_A_Pin|Segment_B_Pin|Segment_C_Pin, GPIO_PIN_SET);
	  		  break;
	  	  case 8:
	  		  HAL_GPIO_WritePin(GPIOC, Segment_A_Pin|Segment_B_Pin|Segment_G_Pin|Segment_F_Pin|Segment_E_Pin|Segment_C_Pin|Segment_D_Pin, GPIO_PIN_SET);
	  		  break;
	  	  case 9:
	  		  HAL_GPIO_WritePin(GPIOC, Segment_A_Pin|Segment_B_Pin|Segment_G_Pin|Segment_F_Pin|Segment_C_Pin|Segment_D_Pin, GPIO_PIN_SET);
	  		  break;
	  	  case 10:
	  		  HAL_GPIO_WritePin(GPIOC, Segment_A_Pin|Segment_B_Pin|Segment_F_Pin|Segment_E_Pin|Segment_C_Pin|Segment_D_Pin, GPIO_PIN_SET);
	  		  break;

	  }

  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 84;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = ENABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 2;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_4;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Rank = 2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 84;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 1000;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 500-1;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 84;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 1000;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 500-1;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, Segment_Dp_Pin|Segment_C_Pin|Segment_E_Pin|Segment_D_Pin
                          |Segment_B_Pin|Segment_F_Pin|Segment_G_Pin|Segment_A_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : Segment_Dp_Pin Segment_C_Pin Segment_E_Pin Segment_D_Pin
                           Segment_B_Pin Segment_F_Pin Segment_G_Pin Segment_A_Pin */
  GPIO_InitStruct.Pin = Segment_Dp_Pin|Segment_C_Pin|Segment_E_Pin|Segment_D_Pin
                          |Segment_B_Pin|Segment_F_Pin|Segment_G_Pin|Segment_A_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : Light_Up_Pin Light_Down_Pin */
  GPIO_InitStruct.Pin = Light_Up_Pin|Light_Down_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
PUTCHAR_PROTOTYPE
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the USART6 and Loop until the end of transmission */
  HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 0xFFFF);

  return ch;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{

	if(htim->Instance == TIM3)
	    {
	        time3Cnt++;

	        if(time3Cnt <= 19000-1) // 0 ~ 19Sec
	            stateFlag = 1;
	        else if(time3Cnt >= 19000-1 && time3Cnt <= 20000-1) // 19 ~ 20Sec
	            stateFlag = 2;
	        else if(time3Cnt >= 20000-1 && time3Cnt <= 27000-1) // 20 ~ 27Sec
	            stateFlag = 3;
	        else if(time3Cnt >= 27000-1 && time3Cnt <= 30000-1) // 27 ~ 30Sec
	            stateFlag = 4;
	        else if(time3Cnt >= 30000-1)
	        {
	            time3Cnt = 0;
	            stateFlag = 5;
	        }
	    }
	else if(htim->Instance == TIM2)
	{
		time2Cnt++;
		if(time2Cnt >= 1000)
		{
			time2Cnt = 0;
			SegmentFlag--;
			resetSegmentFlag = 1;
			if(SegmentFlag < 1) SegmentFlag = 10;
		}
	}
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == Light_Up_Pin) {
        btn_flag = 1;   // 버튼1 눌림
    }
    else if (GPIO_Pin == Light_Down_Pin) {
        btn_flag = 2;   // 버튼2 눌림
    }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    if(hadc->Instance == ADC1)
    {
        uint32_t adc_val = HAL_ADC_GetValue(hadc);
        pulse = (adc_val * (1000 - 100) / 4095) + 100;
    }
}
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

#ifdef  USE_FULL_ASSERT
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
