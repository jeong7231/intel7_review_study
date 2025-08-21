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
#include "stdio.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

void delay_ms(uint32_t ms);
int is_button_pressed(void);
//uint16_t read_adc_value(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void delay_ms(uint32_t ms)
{
    SysTick->LOAD = 168000 - 1;
    SysTick->VAL = 0;
    SysTick->CTRL = 5;
    for(uint32_t i = 0; i < ms; i++)
    {
        while (!(SysTick->CTRL & (1 << 16)));
    }
    SysTick->CTRL = 0;
}

int is_button_pressed(void)
{
    static uint8_t prev_state = 1;
    uint8_t curr = (GPIOA->IDR & (1 << 10)) ? 1 : 0;

    if (prev_state == 1 && curr == 0)
    {
        delay_ms(10);
        curr = (GPIOA->IDR & (1 << 10)) ? 1 : 0;
        if (curr == 0)
        {
            prev_state = 0;
            return 1;
        }
    }
    else if (curr == 1)
    {
        prev_state = 1;
    }
    return 0;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
	// RCC 설정
	    RCC->AHB1ENR |= (1 << 0) | (1 << 1);  // GPIOA, GPIOB
	    RCC->APB2ENR |= (1 << 8);             // ADC1
	    RCC->APB1ENR |= (1 << 2);             // TIM4

	    // === GPIO 설정 ===

	    // [ADC] PA6 - Analog
	    GPIOA->MODER |=  (3 << (6 * 2));      // Analog mode
	    GPIOA->PUPDR &= ~(3 << (6 * 2));

	    // [보행자] PA0(RED), PA1(GREEN) - Output
	    GPIOA->MODER &= ~((3 << (0 * 2)) | (3 << (1 * 2)));
	    GPIOA->MODER |=  ((1 << (0 * 2)) | (1 << (1 * 2)));

	    // [주행자] PB0(RED), PB1(YELLOW) - Output
	    GPIOB->MODER &= ~((3 << (0 * 2)) | (3 << (1 * 2)));
	    GPIOB->MODER |=  ((1 << (0 * 2)) | (1 << (1 * 2)));

	    // [PWM] PB8 - AF2 (TIM4_CH3)
	    GPIOB->MODER &= ~(3 << (8 * 2));
	    GPIOB->MODER |=  (2 << (8 * 2));       // Alternate function
	    GPIOB->AFR[1] &= ~(0xF << 0);          // AFRH[0] = PB8
	    GPIOB->AFR[1] |=  (2 << 0);            // AF2 (TIM4_CH3)

	    // [BUTTON] PA10 - Input + Pull-up
	    GPIOA->MODER &= ~(3 << (10 * 2));      // Input mode
	    GPIOA->PUPDR &= ~(3 << (10 * 2));
	    GPIOA->PUPDR |=  (1 << (10 * 2));      // Pull-up

	    // === TIM4 (PWM) 설정 ===
	    TIM4->PSC = 83;            // 84MHz / (83+1) = 1MHz
	    TIM4->ARR = 999;           // PWM 주기: 1ms
	    TIM4->CCMR2 &= ~(7 << 4);
	    TIM4->CCMR2 |=  (6 << 4);  // PWM 모드 1
	    TIM4->CCMR2 |=  (1 << 3);  // Preload enable
	    TIM4->CCER   |=  (1 << 8); // CH3 Enable
	    TIM4->CR1    |=  (1 << 7) | (1 << 0);  // ARPE, CEN

	    // === ADC 설정 ===
	    ADC1->CR2 &= ~(1 << 0);        // ADON = 0
	    ADC1->SQR3 = 6;                // 채널 6 (PA6)
	    ADC1->SMPR2 |= (7 << (6 * 3)); // 최대 샘플링 시간
	    ADC1->CR2 |= (1 << 0);         // ADON = 1
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
  /* USER CODE BEGIN 2 */
  GPIOB->MODER &= ~(3 << (2 * 2));
  GPIOB->MODER |=  (2 << (2 * 2));    // AF mode
  GPIOB->AFR[0] &= ~(0xF << (2 * 4));
  GPIOB->AFR[0] |=  (2 << (2 * 4));   // AF2
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  uint16_t adc_val = 0;
	          char buf[64];

	          // 보행자 RED, 주행자 GREEN (PWM)
	          GPIOA->ODR |=  (1 << 0);  // PA0 ON (보행자 RED)
	          GPIOA->ODR &= ~(1 << 1);  // PA1 OFF (보행자 GREEN)
	          GPIOB->ODR &= ~((1 << 0) | (1 << 1)); // PB0, PB1 OFF

	          for (int i = 0; i < 80; i++) // 약 8초 대기
	          {
	              ADC1->CR2 |= (1 << 30);             // SWSTART
	              while (!(ADC1->SR & (1 << 1)));     // EOC 대기
	              adc_val = ADC1->DR & 0xFFF;

	              if (adc_val < 100) adc_val = 100;
	              TIM4->CCR3 = (adc_val * 1000) / 4095;

	              if (is_button_pressed()) break;

	              delay_ms(100);
	          }

	          // (디버깅용) UART 출력
	          sprintf(buf, "ADC: %d\r\n", adc_val);
	          for (char *p = buf; *p; p++) {
	              while (!(USART2->SR & (1 << 7))); // TXE
	              USART2->DR = *p;
	          }

	          // === 주행자 YELLOW
	          TIM4->CCR3 = 0;
	          GPIOB->ODR &= ~(1 << 0);  // RED OFF
	          GPIOB->ODR |=  (1 << 1);  // YELLOW ON
	          delay_ms(1000);

	          // === 주행자 RED, 보행자 GREEN
	          GPIOB->ODR &= ~(1 << 1);  // YELLOW OFF
	          GPIOB->ODR |=  (1 << 0);  // RED ON
	          GPIOA->ODR &= ~(1 << 0);  // 보행자 RED OFF
	          GPIOA->ODR |=  (1 << 1);  // 보행자 GREEN ON
	          delay_ms(4000);

	          // === 보행자 GREEN 깜빡임
	          for (int i = 0; i < 4; i++)
	          {
	              GPIOA->ODR ^= (1 << 1);  // Toggle
	              delay_ms(500);
	          }

	          // === 보행자 RED, 주행자 YELLOW
	          GPIOA->ODR &= ~(1 << 1);
	          GPIOA->ODR |=  (1 << 0);
	          GPIOB->ODR &= ~(1 << 0);
	          GPIOB->ODR |=  (1 << 1);
	          delay_ms(1000);

	          GPIOB->ODR &= ~((1 << 0) | (1 << 1));
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

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
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
