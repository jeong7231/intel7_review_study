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
/* Includes ------------------------------------------------------------------ */
#include "main.h"

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
volatile uint8_t g_button_irq = 0;  // PA10 버튼 인터럽트 플래그
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

void delay_ms(uint32_t ms);
void Setup_EXTI_PA10(void);

// int is_button_pressed(void); // 폴링 함수 사용 안함

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
  * @brief SysTick 타이머를 사용한 밀리초 단위 지연 함수
  * @param ms: 지연할 시간 (밀리초)
  */
void delay_ms(uint32_t ms)
{
    // SysTick 타이머 설정 (84MHz 클럭 기준)
    // 84,000,000 Hz / 84,000 = 1000 Hz (1ms 마다 인터럽트 발생)
    SysTick->LOAD = 84000 - 1;  // 1ms마다 카운트가 0이 되도록 설정
    SysTick->VAL = 0;           // 카운터 초기화
    SysTick->CTRL = 5;          // SysTick 클럭 소스(AHB) 설정 및 카운터 활성화 (0b0101)

    for(uint32_t i = 0; i < ms; i++)
    {
        // COUNTFLAG 비트가 1이 될 때까지 대기 (카운터가 0에 도달했음을 의미)
        while (!(SysTick->CTRL & (1 << 16)));
    }
    SysTick->CTRL = 0; // SysTick 타이머 비활성화
}

// 폴링 방식 사용 인터럽트사용중인 경우 쓰지 않음
//int is_button_pressed(void)
//...

/**
  * @brief PA10 핀에 대한 외부 인터럽트(EXTI) 설정
  *        버튼(PA10)이 눌렸을 때(Falling edge) 인터럽트가 발생하도록 설정합니다.
  */
void Setup_EXTI_PA10(void)
{
    // 1. SYSCFG(System Configuration Controller) 클럭 활성화
    // SYSCFG는 외부 인터럽트 라인을 특정 GPIO 포트에 매핑하는 데 필요합니다.
    RCC->APB2ENR |= (1 << 14); // APB2 버스의 SYSCFG 클럭 활성화

    // 2. EXTI 라인에 GPIO 포트 매핑
    // EXTI10을 Port A에 연결합니다. SYSCFG_EXTICR3의 [3:0] 비트를 0000으로 설정.
    // EXTICR은 4개씩 배열로 관리되며, EXTICR[2]는 EXTI8~EXTI11을 담당합니다.
    // EXTI10은 EXTICR[2]의 [11:8] 비트에 해당합니다.
    SYSCFG->EXTICR[2] &= ~(0xF << 8); // PA10을 EXTI10에 연결 (0000: PA[x])

    // 3. EXTI 인터럽트 설정
    EXTI->IMR  |=  (1 << 10);  // 인터럽트 마스크 레지스터: 라인 10의 인터럽트를 활성화 (마스크 해제)
    EXTI->RTSR &= ~(1 << 10);  // Rising Trigger Selection Register: 라인 10의 상승 엣지 트리거 비활성화
    EXTI->FTSR |=  (1 << 10);  // Falling Trigger Selection Register: 라인 10의 하강 엣지 트리거 활성화

    // 4. 인터럽트 Pending 비트 클리어
    // 이전에 발생했을 수 있는 인터럽트 요청을 초기화합니다.
    EXTI->PR = (1 << 10); // Pending Register: 라인 10의 인터럽트 요청을 클리어 (1을 써서 클리어)

    // 5. NVIC(Nested Vectored Interrupt Controller) 설정
    // EXTI10은 EXTI15_10_IRQn 핸들러를 공유합니다.
    NVIC_SetPriority(EXTI15_10_IRQn, 5); // 인터럽트 우선순위 설정
    NVIC_EnableIRQ(EXTI15_10_IRQn);      // NVIC에서 해당 IRQ를 활성화
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  // 인터럽트 방식 코드
    // === 클럭 설정 (RCC: Reset and Clock Control) ===
    // 필요한 주변장치(GPIO, ADC, Timer)에 클럭을 공급합니다.
    RCC->AHB1ENR |= (1 << 0) | (1 << 1);  // AHB1 버스의 GPIOA, GPIOB 클럭 활성화
    RCC->APB2ENR |= (1 << 8);             // APB2 버스의 ADC1 클럭 활성화
    RCC->APB1ENR |= (1 << 2);             // APB1 버스의 TIM4 클럭 활성화

    // === GPIO 핀 기능 설정 ===

    // [보행자 신호등] PA0(RED), PA1(GREEN)을 출력 모드로 설정
    GPIOA->MODER &= ~((3 << (0 * 2)) | (3 << (1 * 2))); // PA0, PA1 핀의 현재 설정 초기화 (00: Input)
    GPIOA->MODER |=  ((1 << (0 * 2)) | (1 << (1 * 2))); // PA0, PA1 핀을 General purpose output mode (01)로 설정

    // [차량 신호등] PB0(RED), PB1(YELLOW)을 출력 모드로 설정
    GPIOB->MODER &= ~((3 << (0 * 2)) | (3 << (1 * 2))); // PB0, PB1 핀의 현재 설정 초기화
    GPIOB->MODER |=  ((1 << (0 * 2)) | (1 << (1 * 2))); // PB0, PB1 핀을 General purpose output mode (01)로 설정

    // === TIM4 (PWM) 설정: 차량용 녹색등 밝기 조절용 ===
    // PWM 주파수: 84MHz / (PSC+1) / (ARR+1) = 84,000,000 / 84 / 1000 = 1000 Hz (1ms 주기)
    TIM4->PSC = 83;            // Prescaler: 타이머 클럭을 84분주하여 1MHz로 만듦
    TIM4->ARR = 999;           // Auto-Reload Register: 0부터 999까지 카운트 (1000 스텝)
    TIM4->CCMR2 &= ~(7 << 4);  // Capture/Compare Mode Register 2: CH3(OC3M) 설정 초기화
    TIM4->CCMR2 |=  (6 << 4);  // OC3M 비트를 'PWM mode 1' (110)으로 설정
    TIM4->CCMR2 |=  (1 << 3);  // OC3PE (Output Compare 3 Preload Enable) 활성화. CCR3 값 즉시 반영 안함.
    TIM4->CCER   |=  (1 << 8); // Capture/Compare Enable Register: CH3(CC3E) 활성화. TIM4_CH3 출력이 핀으로 나감.
    TIM4->CR1    |=  (1 << 7) | (1 << 0);  // Control Register 1: ARPE(Auto-reload preload enable), CEN(Counter enable) 활성화

    // [PWM 핀] PB8을 Alternate Function 모드로 설정 (TIM4_CH3)
    GPIOB->MODER &= ~(3 << (8 * 2));       // PB8 핀의 현재 설정 초기화
    GPIOB->MODER |=  (2 << (8 * 2));       // PB8 핀을 Alternate function mode (10)로 설정
    GPIOB->AFR[1] &= ~(0xF << 0);          // Alternate Function Register High (AFRH): PB8의 AF 설정 초기화
    GPIOB->AFR[1] |=  (2 << 0);            // PB8 핀을 AF2 (TIM4_CH3)로 매핑

    // [버튼] PA10을 입력 및 풀업(Pull-up) 모드로 설정
    GPIOA->MODER &= ~(3 << (10 * 2));      // PA10 핀을 Input mode (00)로 설정
    GPIOA->PUPDR &= ~(3 << (10 * 2));      // PA10 핀의 Pull-up/Pull-down 설정 초기화
    GPIOA->PUPDR |=  (1 << (10 * 2));      // PA10 핀에 Pull-up 저항 활성화 (10)

    // === ADC1 설정: 가변저항 값 읽기용 ===
    ADC1->CR2 &= ~(1 << 0);        // Control Register 2: ADON 비트를 0으로 하여 ADC 비활성화 (설정 전 OFF)
    ADC1->CR2 |= (1 << 0);         // ADON 비트를 1로 하여 ADC 활성화
    ADC1->SQR3 = 6;                // Sequence Register 3: 변환 시퀀스의 첫 번째를 채널 6 (PA6)으로 설정
    ADC1->SMPR2 |= (7 << (6 * 3)); // Sample Time Register 2: 채널 6의 샘플링 시간을 최대로(480 사이클) 설정

    // [ADC 핀] PA6을 아날로그 모드로 설정
    GPIOA->MODER |=  (3 << (6 * 2));      // PA6 핀을 Analog mode (11)로 설정
    GPIOA->PUPDR &= ~(3 << (6 * 2));      // PA6 핀의 Pull-up/Pull-down 비활성화 (아날로그 모드 권장사항)

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

  // PA10에 대한 외부 인터럽트 설정 함수 호출
  Setup_EXTI_PA10();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      uint16_t adc_val = 0;

      // --- 초기 상태: 보행자 RED, 차량 GREEN (PWM으로 밝기 조절) ---
      GPIOA->ODR |=  (1 << 0);  // PA0 ON (보행자 적색등 켜기)
      GPIOA->ODR &= ~(1 << 1);  // PA1 OFF (보행자 녹색등 끄기)
      GPIOB->ODR &= ~((1 << 0) | (1 << 1)); // PB0, PB1 OFF (차량 적색/황색등 끄기)

      // 약 8초 동안 대기하며 ADC 값으로 PWM 제어 및 버튼 입력 확인
      for (int i = 0; i < 80; i++)
      {
          ADC1->CR2 |= (1 << 30);             // SWSTART: ADC 변환 시작
          while (!(ADC1->SR & (1 << 1)));     // EOC(End of Conversion) 비트가 1이 될 때까지 대기
          adc_val = ADC1->DR & 0xFFF;         // Data Register에서 12비트 변환 결과 읽기

          if (adc_val < 100) adc_val = 100;   // 최소 밝기 보정
          // ADC 값(0~4095)을 PWM ডিউ티 사이클(0~999)로 변환하여 CCR3 레지스터에 씀
          TIM4->CCR3 = (adc_val * 1000) / 4095;

          // 버튼 인터럽트 발생 여부 확인
          if (g_button_irq) {
              g_button_irq = 0;   // 인터럽트 플래그 처리 완료
              break;              // 대기 상태 종료하고 신호 변경 로직으로 이동
          }

          delay_ms(100); // 100ms 대기
      }

      // --- 상태 변경: 차량 YELLOW ---
      TIM4->CCR3 = 0;           // PWM 출력 중지 (차량 녹색등 끄기)
      GPIOB->ODR &= ~(1 << 0);  // PB0 OFF (차량 적색등 끄기)
      GPIOB->ODR |=  (1 << 1);  // PB1 ON (차량 황색등 켜기)
      delay_ms(1000);           // 1초 대기

      // --- 상태 변경: 차량 RED, 보행자 GREEN ---
      GPIOB->ODR &= ~(1 << 1);  // PB1 OFF (차량 황색등 끄기)
      GPIOB->ODR |=  (1 << 0);  // PB0 ON (차량 적색등 켜기)
      GPIOA->ODR &= ~(1 << 0);  // PA0 OFF (보행자 적색등 끄기)
      GPIOA->ODR |=  (1 << 1);  // PA1 ON (보행자 녹색등 켜기)
      delay_ms(4000);           // 4초 대기

      // --- 상태 변경: 보행자 GREEN 깜빡임 ---
      for (int i = 0; i < 4; i++)
      {
          GPIOA->ODR ^= (1 << 1);  // PA1(보행자 녹색등) 상태를 토글 (XOR 연산)
          delay_ms(500);
      }

      // --- 상태 변경: 보행자 RED, 차량 YELLOW ---
      GPIOA->ODR &= ~(1 << 1);  // PA1 OFF (보행자 녹색등 끄기)
      GPIOA->ODR |=  (1 << 0);   // PA0 ON (보행자 적색등 켜기)
      GPIOB->ODR &= ~(1 << 0);  // PB0 OFF (차량 적색등 끄기)
      GPIOB->ODR |=  (1 << 1);   // PB1 ON (차량 황색등 켜기)
      delay_ms(1000);           // 1초 대기

      // 모든 차량 신호등 끄고 초기 상태로 복귀 준비
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
/*
  EXTI Line[15:10] IRQ Handler
  이 핸들러는 EXTI10부터 EXTI15까지의 인터럽트를 모두 처리합니다.
*/
void EXTI15_10_IRQHandler(void)
{
    // 발생한 인터럽트가 EXTI10 라인인지 확인
    if (EXTI->PR & (1 << 10)) {
        // 인터럽트 펜딩(Pending) 비트 클리어
        // 중요: 1을 써서 해당 비트를 클리어(Write 1 to Clear)합니다.
        EXTI->PR = (1 << 10);

        // 간단한 소프트웨어 디바운싱 (채터링 방지)
        // 버튼이 눌리고 안정화될 때까지 잠시 대기합니다.
        for (volatile int i = 0; i < 20000; i++);

        // 디바운스 후에도 버튼이 여전히 눌려있는지(Low 상태인지) 확인
        if ((GPIOA->IDR & (1 << 10)) == 0) {
            // 유효한 버튼 눌림으로 판단하고, 메인 루프에서 처리할 수 있도록 플래그 설정
            g_button_irq = 1;
        }
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