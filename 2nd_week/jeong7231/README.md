# STM32F411RE 신호등 제작

## 개요

Timer/Counter, PWM, Interrupt, ADC를 활용하여 신호등을 구현함.

## 주요 기능

* **신호등 FSM**

  * 녹색(10초) → 노란색(3초) → 빨간색(10초) 순서로 상태가 자동 전환됨.
  * `TIM2` 타이머 인터럽트를 10ms 주기로 사용함.

* **LED 밝기 조절(PWM)**

  * `ADC1`을 이용하여 가변저항의 아날로그 값을 읽음.
  * 읽은 ADC 값에 따라 `TIM3`의 PWM 듀티사이클을 조절하여 LED 밝기를 제어함.

* **버튼 인터럽트**

  * 2개의 버튼이 EXTI로 연결되어 있음.
  * 각 버튼 입력 시, 지정된 신호등 상태로 즉시 변경됨.


## 타이머/카운터 설정 (TIM2, TIM3)

### 1. TIM2 (Timer/Counter, 10ms 주기 인터럽트)

* **모드:** Output Compare No Output (채널1만 사용, 나머지 채널 Disable)
* **Prescaler (PSC):** 8399
* **Counter Period (ARR):** 99
* **Counter Mode:** Up
* **Internal Clock Division:** No Division
* **Auto-reload Preload:** Disable
* **TRGO:** Reset (UG bit from TIMx\_EGR)
* **인터럽트 활성화:** TIM2 global interrupt Enable
* **동작:**

  * 84MHz / (8399+1) = 10kHz → 10kHz / (99+1) = 100Hz → **10ms 주기**
  * 10ms마다 인터럽트 발생, 신호등 FSM state\_tick 증가 등에 사용

### 2. TIM3 (PWM 제어)

* **모드:** PWM Generation CH1, CH2, CH3 (각 채널별 PWM, CH4 미사용)
* **Prescaler (PSC):** 83
* **Counter Period (ARR):** 999
* **Counter Mode:** Up
* **Internal Clock Division:** No Division
* **Auto-reload Preload:** Disable
* **TRGO:** Reset (UG bit from TIMx\_EGR)
* **PWM 채널:**

  * Channel 1: PWM Generation CH1
  * Channel 2: PWM Generation CH2
  * Channel 3: PWM Generation CH3
* **인터럽트 활성화:** TIM3 global interrupt Enable (기본적으로 PWM은 인터럽트 없이도 동작, 필요시 활성화 가능)
* **동작:**

  * PWM 주기 = (83+1) x (999+1) / 84MHz ≈ 1ms × 1000 = **1kHz**
  * 각 채널별로 LED 밝기 조절에 사용


## ADC 설정 (ADC1)

* **채널:** IN1 (ADC1\_CH1)
* **Clock Prescaler:** PCLK2 divided by 4
* **Resolution:** 12 bits (0\~4095)
* **Data Alignment:** Right alignment
* **Scan Conversion Mode:** Disabled (단일 채널 변환)
* **Continuous Conversion Mode:** Disabled (단발 변환)
* **Discontinuous Conversion Mode:** Disabled
* **DMA Continuous Requests:** Disabled
* **인터럽트 활성화:** ADC1 global interrupt Enable
* **동작:**

  * 단일 변환 완료 시 인터럽트 발생, ADC 결과값으로 PWM 듀티 조정


## GPIO/EXTI 인터럽트 설정

* **EXTI Line1, Line2 Interrupt Enable:** 활성화됨
* **설정:**

  * BTN\_1: EXTI Line1 (PB1)
  * BTN\_2: EXTI Line2 (PB2)
* **동작:**

  * 각 버튼 입력 시 해당 EXTI 인터럽트 발생
  * HAL\_GPIO\_EXTI\_Callback에서 btn\_1\_flag, btn\_2\_flag 세트


## NVIC 설정

* **활성화된 인터럽트:**

  * EXTI1, EXTI2
  * ADC1
  * TIM2
  * TIM3


## 요약 표

| 기능        | 사용 모듈   | 주요 설정                   | 인터럽트       |
| --------- | ------- | ----------------------- | ---------- |
| FSM 타이머   | TIM2    | Output Compare, 10ms 주기 | Enable     |
| PWM (LED) | TIM3    | PWM CH1\~3, 1kHz 주기     | Enable(선택) |
| 아날로그 입력   | ADC1    | 채널1, 12bit, 단발 변환       | Enable     |
| 버튼        | EXTI1,2 | 각 버튼별 EXTI 매핑           | Enable     |

