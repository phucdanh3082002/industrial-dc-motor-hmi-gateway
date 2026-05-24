# Hardware Pinout & Configuration (STM32F411CEU6 Black Pill)

> **Lưu ý:** Xem chi tiết kết nối hệ thống (bao gồm cả ESP32 và sơ đồ mạng lưới điện) tại [wiring-diagram.md](wiring-diagram.md).

This document tracks the hardware pin assignments for the STM32F411 firmware.

## Core Peripherals

| Component | Pin | Peripheral / Function | Notes |
|-----------|-----|-----------------------|-------|
| **User LED** | PC13 | GPIO Output | Active LOW (Black Pill standard) |
| **Buzzer** | PB12 | GPIO Output | Active HIGH |
| **LM35 Temp Sensor** | PA0 | ADC1_IN0 | LM35 Output (10mV/°C). VREF=3.3V |
| **I2C Bus (Sensors)** | PB6 (SCL)<br>PB7 (SDA) | I2C1 | Connects to INA219 (0x40) & BMP280 (0x76/0x77). Requires 4.7kΩ pull-ups if not on module. |
| **Motor PWM** | PA8 | TIM1_CH1 | 20kHz PWM output to MOSFET gate |

## Communication (UART)
*Configured using Option 1 (Dedicated USARTs).*

| Function | TX Pin | RX Pin | DIR/DE Pin | Peripheral | Baudrate |
|----------|--------|--------|------------|------------|----------|
| **Debug Log** | PA9 | PA10 | - | USART1 | 115200 |
| **Modbus RTU**| PA2 | PA3 | PA4 | USART2 | 9600 |

## Power Supply Notes
*   **LM35**: Needs 4-30V typically, can be powered from 5V pin on Black Pill. Output at 150°C is 1.5V (safe for 3.3V ADC).
*   **I2C Sensors**: Power from 3.3V.
*   **Motor**: Separate 12V supply. Ensure common ground (GND) between motor supply and Black Pill.

## Danh sách linh kiện (Bill of Materials)

| Linh kiện | Vai trò |
|-----------|---------|
| STM32F411 | Field controller, điều khiển motor, đọc cảm biến |
| ESP32-WROOM-32 | HMI gateway, MQTT client, TinyML inference sau này |
| Raspberry Pi 4 | Edge server, MQTT broker, database, dashboard |
| TFT 3.5 inch 320x480 SPI ILI9488 | Màn hình HMI |
| INA219 | Đo dòng và điện áp motor |
| GY-BMP280 | Đo nhiệt độ môi trường và áp suất |
| LM35 | Đo nhiệt độ gần motor hoặc MOSFET |
| 2 module TTL-RS485 | Giao tiếp RS-485 giữa STM32 và ESP32 |
| Động cơ DC | Tải chính |
| MOSFET | Điều khiển motor bằng PWM |
| Buzzer | Cảnh báo lỗi |
| LED | Báo trạng thái |
| Điện trở, tụ, diode flyback | Bảo vệ và ổn định mạch |

*Document Version: 1.1*
