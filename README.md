# Industrial DC Motor HMI Gateway with TinyML Anomaly Detection

> A comprehensive embedded IoT system for monitoring and controlling industrial DC motors with real-time HMI, MQTT telemetry, and machine learning-based anomaly detection.

**Vietnamese name:** Hệ thống giám sát, điều khiển động cơ DC và phát hiện bất thường bằng TinyML

## Quick Overview

A three-tier embedded IoT architecture combining embedded firmware (STM32F411), IoT gateway (ESP32), and edge server (Raspberry Pi) for intelligent DC motor monitoring.

### System Architecture

```
STM32F411 (Field Node)
    ↓ [RS-485 + Modbus RTU]
ESP32-WROOM-32 (HMI Gateway)
    ↓ [Wi-Fi + MQTT]
Raspberry Pi 4 (Edge Server)
```

### Key Features

- **Motor Control**: PWM-based DC motor speed control with FreeRTOS state machine
- **Sensor Integration**: Real-time data from INA219 (current/voltage), LM35 (temperature), BMP280 (pressure)
- **Fault Protection**: Over-current and over-temperature detection with automatic shutdown
- **HMI Display**: 3.5" TFT ILI9488 with LVGL UI (dashboard, alarms, trends)
- **Industrial Communication**: RS-485 Modbus RTU + Wi-Fi MQTT connectivity
- **Data Logging**: SQLite database for telemetry, alarms, and training datasets
- **AI Ready**: TinyML inference engine on ESP32 for motor anomaly detection

## Table of Contents

- [Architecture](#system-architecture)
- [Hardware Components](#hardware-components)
- [Project Structure](#project-structure)
- [Development Phases](#development-phases)
- [Documentation](#documentation)
- [Contributing](#contributing)

---

## Hardware Components

### Main Boards

| Component | Role |
|-----------|------|
| **STM32F411** | Field control node (motor PWM, sensor reading, fault handling) |
| **ESP32-WROOM-32** | HMI gateway (Modbus master, MQTT client, TFT display) |
| **Raspberry Pi 4** | Edge server (MQTT broker, SQLite logging, REST API, web dashboard) |

### Sensors & Actuators

| Component | Purpose |
|-----------|---------|
| INA219 | Motor current & voltage monitoring |
| LM35 | Temperature sensor (near motor/MOSFET) |
| BMP280 | Environment temperature & pressure |
| TFT ILI9488 (3.5") | HMI display |
| DC Motor | Main actuator |
| MOSFET | Motor PWM driver |
| Buzzer & LED | Fault indicators |

### Communication

| Interface | Purpose |
|-----------|---------|
| RS-485 + Modbus RTU | STM32 ↔ ESP32 industrial link |
| Wi-Fi + MQTT | ESP32 ↔ Raspberry Pi telemetry |
| I2C | Sensor reading (INA219, BMP280) |
| SPI | TFT display driver |
| ADC | LM35 temperature sampling |

---

## Project Structure

```
.
├── stm32f411-firmware/          # STM32 field control firmware
│   ├── src/
│   │   ├── drivers/             # ADC, I2C, UART, PWM
│   │   ├── sensors/             # LM35, INA219, BMP280
│   │   ├── motor/               # PWM control, state machine
│   │   ├── modbus/              # Modbus RTU slave
│   │   ├── fault/               # Alarm detection
│   │   └── freertos/            # Task definitions
│   └── CMakeLists.txt
│
├── esp32-hmi-gateway/           # ESP32 HMI & MQTT gateway
│   ├── src/
│   │   ├── modbus/              # Modbus RTU master
│   │   ├── mqtt/                # MQTT client
│   │   ├── hmi/                 # LVGL UI screens
│   │   ├── tinyml/              # AI inference engine
│   │   └── freertos/            # Task definitions
│   └── CMakeLists.txt
│
├── raspberry-pi-server/         # Edge server & logging
│   ├── mosquitto/               # MQTT broker config
│   ├── app/                     # Flask/FastAPI REST API
│   ├── db/                      # SQLite schema & migrations
│   ├── dashboard/               # Web UI (HTML/CSS/JS)
│   └── systemd/                 # Service definitions

---

## Development Phases

### Phase 1-2: STM32 Firmware (Motor Control & Protection)
- Hardware bring-up (LEDs, UART, sensors)
- Motor state machine with manual/auto modes
- Over-current and over-temperature protection
- Watchdog and fault recovery

### Phase 3: Modbus Communication
- RS-485 + Modbus RTU between STM32 and ESP32
- Register map for telemetry and command

### Phase 4: ESP32 HMI Gateway
- TFT ILI9488 display with LVGL
- Real-time motor dashboard
- Alarm and trend screens

### Phase 5: MQTT + Data Logging
- Mosquitto MQTT broker on Raspberry Pi
- SQLite logging (telemetry, alarms, training data)
- Command history tracking

### Phase 6: Web Dashboard & API
- REST API on Raspberry Pi
- Web dashboard for remote monitoring
- CSV export for dataset analysis

### Phase 7: System Stabilization
- FreeRTOS optimization
- Watchdog and reconnect logic
- Static analysis (Cppcheck)
- Unit testing

### Phase 8: TinyML Anomaly Detection (Future)
- Train motor anomaly detection model
- Deploy TensorFlow Lite model on ESP32
- Real-time AI status on HMI
- Anomaly score publishing to MQTT

---

## Documentation

For detailed technical documentation, see:

- **[AGENTS.md](./AGENTS.md)** - Project guidelines for OpenCode
- **[docs/architecture.md](./docs/architecture.md)** - Detailed system architecture
- **[docs/modbus-register-map.md](./docs/modbus-register-map.md)** - Modbus RTU specification
- **[docs/mqtt-topics.md](./docs/mqtt-topics.md)** - MQTT topic structure
- **[docs/motor-control-design.md](./docs/motor-control-design.md)** - State machine & control modes
- **[docs/tinyml-roadmap.md](./docs/tinyml-roadmap.md)** - Machine learning integration plan
- **[docs/wiring-diagram.md](./docs/wiring-diagram.md)** - Hardware connections
- **[docs/database-schema.md](./docs/database-schema.md)** - SQLite schema

---

## Getting Started

### Prerequisites

- **STM32CubeIDE** + **STM32 HAL** for firmware development
- **ESP-IDF** 5.0+ for ESP32 development
- **Python 3.8+** for Raspberry Pi server
- **Mosquitto** MQTT broker

### Quick Build

```bash
# STM32 firmware
cd stm32f411-firmware
cmake -B build
cmake --build build

# ESP32 firmware
cd esp32-hmi-gateway
idf.py build

# Raspberry Pi server
cd raspberry-pi-server
pip install -r requirements.txt
```

---

## Detailed Documentation

For technical depth, extensive documentation is available in the `docs/` directory covering system architecture, register maps, communication protocols, and implementation details.

---

## Contributing

Contributions are welcome! Please:

1. Follow the existing code style and conventions
2. Include test coverage for new features
3. Update documentation accordingly
4. Reference the AGENTS.md for project-specific guidelines

---

## License

This project is licensed under the MIT License - see LICENSE file for details.

---

## Support & Feedback

- **OpenCode Documentation**: https://opencode.ai/docs
- **Report Issues**: https://github.com/anomalyco/opencode
- **Project Questions**: Refer to detailed docs in `docs/` directory

---

## Technical Stack

- **STM32F411**: C, STM32 HAL, FreeRTOS
- **ESP32-WROOM-32**: ESP-IDF, FreeRTOS, LVGL, TensorFlow Lite Micro
- **Raspberry Pi 4**: Python, Flask/FastAPI, SQLite, Mosquitto MQTT
- **Communication**: Modbus RTU, MQTT, SPI, I2C, UART

---

**Last Updated**: May 2026  
**Roadmap**: 8 phases from hardware bring-up to TinyML deployment
