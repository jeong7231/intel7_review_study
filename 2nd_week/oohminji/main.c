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
#include<stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#ifdef __GNUC__
/* With GCC, small printf (option LD Linker->Libraries->Small printf
   set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

TIM_HandleTypeDef htim4;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
volatile int time4SecFlag = 0;
volatile unsigned int time4SecCnt=0; //1초마다 증가
volatile unsigned int time4Cnt = 0; //1msec마다 증가
volatile int time4CntFlag = 1;
volatile int time4_100msFlag =0; //100ms마다 set
unsigned long debounceDelay = 10; //10ms 마다 버튼이 눌리는지 검사, 채터링이 심할 때 추가
unsigned long lastDebounceTime = 0;
uint32_t pwmValue;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM4_Init(void);
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
	int pluse =0;
	int keyNum;
	int time=1;
	int colorIndex = 0;
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
  MX_USART2_UART_Init();
  MX_ADC1_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */
  printf("start blinker_test\r\n");
  if(HAL_TIM_Base_Start_IT(&htim4) != HAL_OK) //인터럽트 시작
  	Error_Handler();

  if(HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1) != HAL_OK)
		Error_Handler();
  if(HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2) != HAL_OK)
		Error_Handler();
  if(HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3) != HAL_OK)
		Error_Handler();
  HAL_ADC_Start_IT(&hadc1);


  HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);

  uint32_t adcValue = HAL_ADC_GetValue(&hadc1);
  	  printf("%d\r\n",adcValue);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

	  if(time4SecFlag)
	 	  {
	 		  HAL_GPIO_TogglePin(LD2_GPIO_Port , LD2_Pin);
	 		  time4SecFlag = 0;

	 		//  printf("time4SecCnt : %d, pluse : %d \r\n",time4SecCnt,pluse);
	 	  }
	 	  if((HAL_GetTick() - lastDebounceTime) > debounceDelay)
	 	  {
	 		  keyNum = getKeyNum();
	 		  lastDebounceTime = HAL_GetTick();
	 	  	  if(keyNum != 0) //키가 눌리지 않으면 0 눌리면 1~4
	 	  	  {
	 	  		printf("keyNum : %d \r\n",keyNum);

	 	  		if((time == 10 && keyNum ==1) || (time == 1 && keyNum ==2)) // 같은 값을 계속 나오게할 때 펄스 값이 더이상 나오지 않게 continue
	 	  			 	  			continue;
	 	  		if((keyNum == 1) && (time < 10))
	 	  		{
	 	  			time++;
	 	  		}
	 	  		else if((keyNum == 2) && (time > 1))
	 	  		{
	 	  			time--;
	 	  		}
	 	  	  }
	 	  }
	if(time4SecFlag)
	{
		 time4SecFlag = 0;
	 	 if(time4SecCnt % time == 0)
	 	 {
	 	 	 	if (colorIndex == 0)
	 	 	    {
	 	 	 		__HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_1, pwmValue);
	 	 	 	    __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_2, 0);
	 	 	 	    __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_3, 0);
	 	 	    }
	 	        else if (colorIndex == 1)
	 	 	    {
	 	 	 	   __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_1, 0);
	 	 	 	   __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_2, pwmValue);
	 	 	 	   __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_3, 0);
	 	 	 	 }
	 	 	 	 else if (colorIndex == 2)
	 	 	 	{
	 	 	 	   __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_1, 0);
	 	 	 	   __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_2, 0);
	 	 	 	   __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_3, pwmValue);
	 	 	 	  }
	 	 	 	  	colorIndex++;
	 	 	 	  	if (colorIndex > 2) colorIndex = 0;
	 	 }
	}
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
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
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV6;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = ENABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_10;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM4_Init(void)
{

  /* USER CODE BEGIN TIM4_Init 0 */

  /* USER CODE END TIM4_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 84-1;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 1000-1;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.Pulse = 1000;
  if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */

  /* USER CODE END TIM4_Init 2 */
  HAL_TIM_MspPostInit(&htim4);

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
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : plus_bt_Pin minus_bt_Pin */
  GPIO_InitStruct.Pin = plus_bt_Pin|minus_bt_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);

  HAL_NVIC_SetPriority(EXTI3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI3_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
	 void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
	 {
	 	if(htim->Instance == TIM4)
	 	{
	 		time4CntFlag = 1;
	 		time4Cnt++; //1msec마다 증가
	 		if(time4Cnt % 100) //1ms*100 =100ms
	 				time4_100msFlag = 1;
	 		if(time4Cnt >= 1000)
	 		{
	 			time4SecCnt++; //1초마다 증가
	 			time4SecFlag = 1;
	 			time4Cnt = 0;
	 		}
	 	}
	 }

	 int getKeyNum()
	 {
	 	int keyNumber = 0;
	 	int buttonState = 0;
	 	static int keyNumberOld=0;
	 	for(int i = 0; i<2; i++)
	 	{
	 		  buttonState = HAL_GPIO_ReadPin(GPIOC, plus_bt_Pin<<i);
	 		  if(buttonState) //눌리면
	 		  {
	 			  keyNumber = i+1;
	 			  if(keyNumber != keyNumberOld)
	 			  {
	 				  keyNumberOld = keyNumber;
	 				  return keyNumber; //keyNum 1~4
	 			  }
	 			  else
	 				  return 0;
	 		  }
	 	}
	 	keyNumberOld = 0;
	 	return keyNumber; //눌리지 않으면 초기값 0을 return
	 }

	 /**
	   * @brief  Retargets the C library printf function to the USART.
	   * @param  None
	   * @retval None
	   */
	 PUTCHAR_PROTOTYPE
	 {
	   /* Place your implementation of fputc here */
	   /* e.g. write a character to the USART6 and Loop until the end of transmission */
	   HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 0xFFFF);

	   return ch;
	 }

	 void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
	 {
	     if (hadc->Instance == ADC1)   // 사용 중인 ADC 확인
	     {
	         uint32_t adcValue = HAL_ADC_GetValue(hadc);

	         // 12비트 ADC 값(0~4095)을 PWM 범위(0~ARR)로 매핑
	         pwmValue = (adcValue * __HAL_TIM_GET_AUTORELOAD(&htim4)) / 4095;

	         // 다음 변환을 위해 다시 ADC 시작 (연속 변환이 아니면 필요)
	         HAL_ADC_Start_IT(hadc);
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
