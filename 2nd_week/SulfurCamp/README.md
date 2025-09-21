from pathlib import Path

readme_content = """
# 🚦 STM32 Register-Level Traffic Light System

## ✍️ Purpose of Study

This project aims to implement a traffic light control system **without using the STM32 HAL libraries**, relying solely on **direct register access**. Through this process, I studied how microcontroller peripherals like **GPIO, ADC, EXTI, TIM, NVIC**, and **SysTick** work under the hood by directly manipulating registers.

---

## 📌 Main Features

- **Pedestrian Traffic Lights (PA0, PA1)**
  - RED = PA0, GREEN = PA1
- **Driver Traffic Lights (PB0, PB1, PB8)**
  - RED = PB0, YELLOW = PB1, GREEN (PWM controlled) = PB8 (TIM4_CH3)
- **PWM Brightness Control**
  - Analog voltage from potentiometer (PA6) read via ADC1 and used to control PWM duty cycle.
- **External Interrupt Button (PA10)**
  - EXTI10 triggers pedestrian light switch on button press.
- **Debouncing and Interrupt Flag Handling**
- **Custom delay_ms() using SysTick without HAL**

---

## 🧠 Study Approach

- **Reference Manual & Datasheet Analysis**
  - Studied bit-level operations of registers such as `SYSCFG_EXTICR`, `EXTI->PR`, `TIM4->CCMR2`, `ADC->SQR3`, etc.
  - Learned to use `RCC`, `GPIO`, `TIM`, and `ADC` modules manually without HAL.

- **Minimal HAL Skeleton Usage**
  - Kept HAL's `main.c` and initialization structure, but moved all peripheral configurations to register-level code.

- **Debug-Oriented Learning**
  - Used register view in STM32CubeIDE to verify settings.
  - Verified LED toggling and PWM duty with physical testing.
  - Solved ADC instability by adjusting `SMPR2` for longer sampling time.

---

## 🔍 Debugging & Testing

- **Used LED & button testing** for real-time confirmation.
- **Confirmed PWM effects** by varying potentiometer and observing brightness.
- Used `SysTick->CTRL` and `SysTick->VAL` to create millisecond delay without HAL.

---

## 💡 Lessons Learned

This register-level implementation helped me understand **timing, synchronization**, and **interrupt safety**. By building from the bottom up, I became more confident in handling low-level MCU operations beyond what HAL abstracts away.

### 🧪 Next Steps

- Add prioritized pedestrian request logic.
- Output status via UART.
- Use TIM-based asynchronous events.

---

## 🛠 Hardware & Tools

- **MCU**: STM32F411RE (Nucleo)
- **IDE**: STM32CubeIDE
- **Debugging**: ST-Link V2
- **Programming Method**: CMSIS & register-level only

---

## 📁 File Overview

| File              | Description |
|-------------------|-------------|
| `main.c`          | Main logic with register-level implementation |
| `main.h`          | Header declarations |
| `README.md`       | This documentation |
| `STM32CubeMX.ioc` | Used only for pin mapping reference |
"""

# Save to file
output_path = Path("README.md")
output_path.write_text(readme_content.strip(), encoding="utf-8")
output_path.resolve()
