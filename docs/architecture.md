# System Architecture

## Overview

Industrial DC Motor Monitoring system with three-tier distributed architecture for embedded IoT applications.

---

## 3-Tier Architecture

```
+--------------------------------------------------------------------------------+
|                    Raspberry Pi 4 - Embedded Linux Edge Server                  |
|--------------------------------------------------------------------------------|
| Mosquitto MQTT Broker | Flask/FastAPI REST API | SQLite Database               |
| Web Dashboard | Data Logger | Training Dataset | Alarm History | systemd       |
+------------------------------------------^-------------------------------------+
                                           |
                                           | Wi-Fi + MQTT
                                           |
+------------------------------------------+-------------------------------------+
|                    ESP32-WROOM-32 HMI + TinyML Gateway                          |
|--------------------------------------------------------------------------------|
| ESP-IDF | FreeRTOS | LVGL | MQTT Client | Modbus RTU Master                  |
| TFT ILI9488 SPI | Alarm Display | Config NVS | TinyML Inference Engine          |
+------------------------------------------^-------------------------------------+
                                           |
                                           | UART + RS-485 + Modbus RTU
                                           |
+------------------------------------------+-------------------------------------+
|                    STM32F411 Field Control Node                                |
|--------------------------------------------------------------------------------|
| STM32 HAL | FreeRTOS | ADC | I2C | UART | PWM | GPIO | Watchdog               |
| LM35 | INA219 | BMP280 | MOSFET Motor Driver | Buzzer | LED                  |
+--------------------------------------------------------------------------------+
```

---

## Data Flow

```
LM35 / INA219 / BMP280
        ↓
STM32F411 processes sensor data, motor control, fault detection
        ↓
RS-485 + Modbus RTU
        ↓
ESP32 displays HMI, sends MQTT, runs TinyML inference (Phase 8)
        ↓
Wi-Fi + MQTT
        ↓
Raspberry Pi logs data, hosts dashboard, manages training data
```

---

## Component Roles

### STM32F411 - Field Control Node
- **Primary responsibility**: Embedded firmware core
- **Functions**:
  - Read LM35 via ADC
  - Read INA219 via I2C
  - Read BMP280 via I2C
  - Control DC motor via PWM (MOSFET)
  - Drive buzzer and LED
  - Handle over-current protection
  - Handle over-temperature protection
  - Detect sensor errors
  - Implement motor state machine
  - Maintain Modbus register map
  - Act as Modbus RTU Slave (RS-485)
  - Accept commands from ESP32 (start, stop, set speed, reset alarm, switch mode)
  - Use watchdog for system reliability

### ESP32-WROOM-32 - HMI Gateway & TinyML Engine
- **Primary responsibility**: Gateway between field node and server
- **Core Functions**:
  - Act as Modbus RTU Master
  - Poll STM32 data via RS-485
  - Send commands to STM32
  - Drive TFT ILI9488 via SPI
  - Run LVGL-based HMI
  - Display motor state, PWM, current, voltage, temperature, pressure, alarms
  - Publish telemetry to Raspberry Pi via MQTT
  - Subscribe to commands from Raspberry Pi
  - Manage Wi-Fi reconnection logic
  - Manage MQTT reconnection
  - Manage Modbus timeouts
  - Store configuration in NVS

- **Future Functions (Phase 8)**:
  - Run TinyML inference engine
  - Process motor telemetry for anomaly detection
  - Display AI status on TFT
  - Send anomaly scores to Raspberry Pi
  - Alert on detected anomalies

### Raspberry Pi 4 - Edge Server
- **Primary responsibility**: Data aggregation and user interface
- **Functions**:
  - Run Mosquitto MQTT broker
  - Receive and process telemetry from ESP32
  - Persist data to SQLite database
  - Maintain alarm history
  - Maintain command history
  - Maintain training logs for TinyML
  - Run REST API (Flask or FastAPI)
  - Host web dashboard
  - Export datasets for model training
  - Manage systemd services

---

## Communication Protocols

### RS-485 + Modbus RTU (STM32 ↔ ESP32)

**Purpose**: Industrial-grade communication between field node and gateway

**Specifications**:
- **Initial Baudrate**: 9600 bps
- **Production Baudrate**: 115200 bps
- **Data Bits**: 8
- **Parity**: None
- **Stop Bits**: 1
- **Mode**: Half-duplex
- **CRC**: Modbus CRC16
- **Slave ID**: 1 (STM32)
- **Polling Interval**: 500ms - 1s
- **Timeout**: 2s (with retry logic)

**Advantages**:
- Long-distance communication capability
- Noise immunity with twisted pair
- Industrial standard protocol
- CRC error detection
- Multiple slave support (future expansion)

### Wi-Fi + MQTT (ESP32 ↔ Raspberry Pi)

**Purpose**: Telemetry publishing and command subscription

**Specifications**:
- **Protocol**: MQTT 3.1.1
- **QoS**: 1 (at least once delivery)
- **Keep Alive**: 60 seconds
- **Auto-reconnect**: Enabled with exponential backoff
- **Topics**: See mqtt-topics.md

**Key Topics**:
- `factory/motor01/telemetry` - Periodic sensor data
- `factory/motor01/command` - Commands from server
- `factory/motor01/alarm` - Alarm notifications
- `factory/motor01/status` - Connection/heartbeat
- `factory/motor01/ai` - AI inference results (Phase 8)

### I2C (STM32 ↔ Sensors)

**Purpose**: Sensor communication

**Specifications**:
- **Clock Speed**: 100 kHz (standard mode) or 400 kHz (fast mode)
- **Slave Addresses**:
  - INA219: 0x40 (default)
  - BMP280: 0x76 or 0x77 (configurable)

### SPI (ESP32 ↔ TFT ILI9488)

**Purpose**: Display driver communication

**Specifications**:
- **Clock Speed**: 40 MHz (production), 5 MHz (debug/low-power)
- **Mode**: SPI Mode 0
- **Data Width**: 8-bit

### ADC (STM32 ↔ LM35)

**Purpose**: Analog temperature measurement

**Specifications**:
- **ADC Resolution**: 12-bit
- **Sample Rate**: Configurable (typical: 10-100 samples/s)
- **Voltage Reference**: 3.3V (VREF+)
- **Averaging**: 16+ samples recommended for noise reduction

---

## Power Architecture

### Voltage Domains

| Domain | Voltage | Purpose | Note |
|--------|---------|---------|------|
| Motor Supply | 12V DC | DC motor power | Separate PSU recommended |
| STM32 VDD | 3.3V | Microcontroller | Via dedicated LDO |
| ESP32 VDD | 3.3V | WiFi SoC | Via dedicated LDO (higher current) |
| INA219 VDD | 3.3V | Current sensor | Via main LDO |
| BMP280 VDD | 3.3V | Pressure sensor | Via main LDO |
| LM35 VDD | 5V | Temperature sensor | Can use USB power or separate supply |
| TFT VDD | 3.3V | Display | Via dedicated LDO (recommended) |

### Power Supply Recommendations

- **Motor PSU**: 12V, 5-10A (depending on motor specifications)
- **Logic PSU**: 5V USB or linear PSU, 2-3A total
  - STM32: ~50mA
  - ESP32: 80-500mA (Wi-Fi activity dependent)
  - TFT: 200-400mA at full brightness
  - Sensors: ~20mA total

---

## Expansion Capabilities

### Modbus Multi-Slave Support
- Current implementation: 1 STM32 slave
- Future: Support multiple motor nodes by assigning different Slave IDs

### MQTT Topic Scalability
- Current: `factory/motor01/*`
- Future: Extend to `factory/motor02/*`, `factory/motor03/*`, etc.

### Database Multi-Node Support
- Add `node_id` field to all tables (already implemented in schema)
- Allow logging from multiple motor systems simultaneously

### TinyML Model Versioning
- Support multiple model versions in `ai_result` table
- Enable A/B testing of different anomaly detection models

---

## Reliability Features

### Watchdog Mechanism
- STM32 internal watchdog for system recovery
- Watchdog reset counter tracking
- Critical section monitoring

### Fault Recovery
- Over-current: Automatic motor stop, fault flag set
- Over-temperature: PWM limitation followed by stop
- Sensor error: Flag set, telemetry continued with error indication
- Communication timeout: Automatic retry with exponential backoff

### Data Integrity
- Modbus CRC16 validation on every packet
- SQLite transaction support for atomic operations
- MQTT QoS 1 ensures message delivery

### System Resilience
- Automatic Wi-Fi reconnection
- MQTT broker reconnection with backoff
- Modbus slave timeout/retry handling
- Graceful degradation (local operation if server unavailable)

---

## Performance Characteristics

| Metric | Target | Notes |
|--------|--------|-------|
| Modbus Poll Rate | 1 poll/s | Can be optimized up to 500ms |
| MQTT Publish Rate | 1 msg/s | Configurable per application |
| TFT Update Rate | 10 Hz | Limited by LVGL rendering |
| ADC Sample Rate | 100 Hz | Per sensor |
| Modbus Response Time | <500ms | Including network latency |
| Motor State Change | <100ms | From ESP32 command |
| System Boot Time | <10s | STM32 + ESP32 initialization |

---

## Security Considerations (Future)

- **Modbus**: No built-in authentication (use RS-485 isolation)
- **MQTT**: Optional username/password authentication
- **TLS**: Can be added to MQTT for encrypted communication
- **Firmware Signing**: Recommended for production deployments
- **Network Segmentation**: Keep industrial network separate from public network

---

**Last Updated**: May 2026  
**Version**: 1.0
