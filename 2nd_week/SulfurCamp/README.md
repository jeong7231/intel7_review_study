readme_korean = """
# 🚦 STM32 레지스터 기반 신호등 시스템

## ✍️ 학습 목적

이 프로젝트는 STM32 HAL 라이브러리를 사용하지 않고, **레지스터 직접 접근 방식**만으로 신호등 제어 시스템을 구현하는 것이 목표입니다. 이를 통해 **GPIO, ADC, EXTI, TIM, NVIC, SysTick** 등 다양한 주변장치의 동작 원리를 직접 다루며 마이크로컨트롤러의 구조에 대한 이해를 높이고자 했습니다.

---

## 📌 주요 기능

- **보행자 신호등 제어 (PA0, PA1)**
  - RED: PA0, GREEN: PA1
- **차량 신호등 제어 (PB0, PB1, PB8)**
  - RED: PB0, YELLOW: PB1, GREEN(PWM): PB8 (TIM4_CH3)
- **PWM 밝기 제어**
  - 가변저항(PA6)의 아날로그 입력을 ADC1으로 읽어 PWM 듀티 사이클에 반영
- **버튼 인터럽트 (PA10)**
  - EXTI10 인터럽트를 이용하여 보행자 요청을 처리
- **디바운싱 및 인터럽트 플래그 처리**
- **SysTick을 활용한 밀리초 단위 delay 함수 구현 (HAL 미사용)**

---

## 🧠 학습 방식

- **레퍼런스 매뉴얼과 데이터시트 분석**
  - `SYSCFG_EXTICR`, `EXTI->PR`, `TIM4->CCMR2`, `ADC->SQR3` 등 각 레지스터의 비트 구조를 분석
  - HAL 없이 RCC, GPIO, TIM, ADC 등을 직접 제어하는 방법 학습

- **최소한의 HAL 구조 활용**
  - `main.c` 및 초기화 구조는 HAL의 틀을 유지하면서, 실제 설정은 모두 직접 레지스터 접근으로 작성

- **디버깅 중심의 반복 학습**
  - STM32CubeIDE의 레지스터 뷰를 활용해 설정 확인
  - LED, 버튼 테스트를 통한 실시간 동작 확인
  - ADC의 불안정 문제를 `SMPR2` 샘플링 시간 조정으로 해결

---

## 🔍 디버깅 및 테스트

- LED 깜빡임 및 버튼 반응 확인을 통해 동작 검증
- PWM 듀티 사이클 변화에 따라 LED 밝기가 부드럽게 조절됨을 확인
- SysTick 타이머를 활용해 HAL 없이 정확한 `delay_ms()` 구현

---

## 💡 느낀 점

이번 프로젝트를 통해 HAL 추상화 없이 직접 MCU를 제어하며, **타이밍, 인터럽트 동기화, 주변장치 연동**에 대한 깊은 이해를 얻었습니다. 레지스터 기반 설계를 직접 구현해보며 MCU 내부 동작 흐름을 더 정확히 파악할 수 있었습니다.

### 🧪 향후 계획

- 보행자 요청 우선순위 기능 추가
- UART를 이용한 상태 출력
- 타이머 인터럽트를 활용한 비동기 이벤트 처리

---

## 🛠 사용 환경

- **MCU**: STM32F411RE (Nucleo 보드)
- **IDE**: STM32CubeIDE
- **디버깅 도구**: ST-Link V2
- **방식**: CMSIS 및 레지스터 직접 제어 방식

---

## 📁 파일 구성

| 파일명             | 설명 |
|--------------------|------|
| `main.c`           | 레지스터 기반 신호등 구현 메인 코드 |
| `main.h`           | 헤더 파일 |
| `README.md`        | 프로젝트 설명서 (현재 문서) |
| `STM32CubeMX.ioc`  | 핀 매핑 참고용 설정 파일 (선택 사항) |
"""

# Save Korean version of README.md
readme_path_kr = Path("README_한글.md")
readme_path_kr.write_text(readme_korean.strip(), encoding="utf-8")
readme_path_kr.resolve()
