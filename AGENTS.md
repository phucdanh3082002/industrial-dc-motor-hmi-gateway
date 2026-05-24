# Industrial DC Motor HMI Gateway - OpenCode Agent Guide

This guide helps OpenCode agents understand the project structure, architecture, and implementation details for the Industrial DC Motor Monitoring system with HMI and TinyML anomaly detection.

---

## Project Overview

**Type**: Multi-platform embedded IoT system  
**Primary Languages**: C (STM32, ESP32), Python (Raspberry Pi)  
**Build Systems**: CMake (STM32, ESP32), Python pip (Raspberry Pi)  
**Architecture**: 3-tier distributed system (Field Node → Gateway → Edge Server)

---

## System Architecture

### Three-Tier Architecture

```
┌─────────────────────────────────────────────────────────────┐
│ STM32F411 - Field Control Node                              │
│ ├── Motor PWM control via MOSFET                           │
│ ├── Sensor Reading (LM35, INA219, BMP280)                 │
│ ├── Over-current & Over-temperature protection            │
│ ├── Modbus RTU Slave (RS-485)                             │
│ ├── FreeRTOS Tasks                                        │
│ └── Watchdog for reliability                              │
└────────────────┬────────────────────────────────────────────┘
                 │ RS-485 + Modbus RTU (UART)
                 │
┌────────────────▼────────────────────────────────────────────┐
│ ESP32-WROOM-32 - HMI Gateway                               │
│ ├── Modbus RTU Master (polls STM32)                       │
│ ├── MQTT Client (publishes telemetry)                    │
│ ├── TFT ILI9488 Display (LVGL UI)                        │
│ ├── Wi-Fi connectivity                                   │
│ ├── FreeRTOS Tasks                                       │
│ └── TinyML Inference Engine (Phase 8)                    │
└────────────────┬────────────────────────────────────────────┘
                 │ Wi-Fi + MQTT
                 │
┌────────────────▼────────────────────────────────────────────┐
│ Raspberry Pi 4 - Edge Server                               │
│ ├── Mosquitto MQTT Broker                                 │
│ ├── SQLite Database (telemetry, alarms, training data)   │
│ ├── Flask/FastAPI REST API                               │
│ ├── Web Dashboard (HTML/CSS/JS)                          │
│ └── systemd Services                                     │
└─────────────────────────────────────────────────────────────┘
```

### Key Communication Protocols

- **RS-485 + Modbus RTU**: Industrial-grade communication between STM32 and ESP32
  - Baudrate: 9600 bps (initial), 115200 bps (production)
  - Slave ID: 1 (STM32)
  - Polling interval: 500ms - 1s
  - CRC16 validation

- **Wi-Fi + MQTT**: Telemetry and commands between ESP32 and Raspberry Pi
  - Topics: `factory/motor01/telemetry`, `factory/motor01/command`, `factory/motor01/alarm`
  - QoS: 1 (recommended for reliability)

- **I2C**: Sensor communication (INA219, BMP280)
- **SPI**: TFT ILI9488 display driver
- **ADC**: LM35 temperature sampling

---

## Project Directory Structure

```
industrial-dc-motor-hmi-gateway/
├── README.md                          # Main project overview
├── AGENTS.md                          # This file - OpenCode guidance
│
├── stm32f411-firmware/                # STM32 Field Node
│   ├── CMakeLists.txt
│   ├── src/
│   │   ├── main.c                     # Entry point
│   │   ├── drivers/                   # HAL drivers
│   │   │   ├── adc.c, adc.h          # LM35 sampling
│   │   │   ├── i2c.c, i2c.h          # INA219, BMP280
│   │   │   ├── uart.c, uart.h        # RS-485 UART
│   │   │   ├── pwm.c, pwm.h          # Motor PWM
│   │   │   └── gpio.c, gpio.h        # Buzzer, LED
│   │   ├── sensors/                   # Sensor drivers
│   │   │   ├── lm35.c, lm35.h        # Temperature
│   │   │   ├── ina219.c, ina219.h    # Current/Voltage
│   │   │   └── bmp280.c, bmp280.h    # Pressure/Temp
│   │   ├── motor/                     # Motor control
│   │   │   ├── motor_control.c       # PWM & state machine
│   │   │   ├── motor_state.h         # STOP, RUNNING, WARNING, FAULT
│   │   │   └── pid_controller.c      # Future: PID tuning
│   │   ├── modbus/                    # Modbus RTU Slave
│   │   │   ├── modbus_slave.c
│   │   │   ├── modbus_register.h     # Register map (40001-40108)
│   │   │   └── crc16.c               # Modbus CRC
│   │   ├── fault/                     # Fault detection
│   │   │   ├── alarm.c, alarm.h      # Alarm flags & codes
│   │   │   └── watchdog.c            # STM32 watchdog
│   │   ├── freertos/                  # RTOS tasks
│   │   │   ├── sensor_task.c         # Periodic sensor reading
│   │   │   ├── motor_task.c          # Motor control & state machine
│   │   │   ├── alarm_task.c          # Fault detection
│   │   │   ├── modbus_task.c         # Modbus slave handler
│   │   │   └── watchdog_task.c       # Feed watchdog
│   │   └── config/                    # Constants
│   │       └── config.h
│   └── build/                         # Build artifacts
│
├── esp32-hmi-gateway/                 # ESP32 HMI Gateway
│   ├── CMakeLists.txt
│   ├── src/
│   │   ├── main.c                     # ESP-IDF entry
│   │   ├── modbus/                    # Modbus RTU Master
│   │   │   ├── modbus_master.c       # Master protocol
│   │   │   └── modbus_client.h       # API
│   │   ├── mqtt/                      # MQTT Client
│   │   │   ├── mqtt_client.c         # Connection & publish/subscribe
│   │   │   └── mqtt_topics.h         # Topic definitions
│   │   ├── hmi/                       # HMI UI Layer
│   │   │   ├── lvgl_config.c         # LVGL setup
│   │   │   ├── screen_dashboard.c    # Main dashboard
│   │   │   ├── screen_motor.c        # Motor control screen
│   │   │   ├── screen_alarm.c        # Alarm display
│   │   │   ├── screen_trend.c        # Trend graph
│   │   │   └── screen_ai.c           # AI status (Phase 8)
│   │   ├── tft/                       # TFT Driver
│   │   │   ├── ili9488.c, ili9488.h  # ILI9488 SPI driver
│   │   │   └── tft_init.c
│   │   ├── tinyml/                    # TinyML Inference
│   │   │   ├── model.c               # Quantized model (Phase 8)
│   │   │   ├── inference.c           # Inference engine
│   │   │   └── feature_extract.c     # Feature normalization
│   │   ├── wifi/                      # Wi-Fi Management
│   │   │   ├── wifi_manager.c        # Connection & reconnect
│   │   │   └── nvs_config.c          # NVS storage
│   │   ├── freertos/                  # RTOS tasks
│   │   │   ├── modbus_master_task.c  # Poll STM32
│   │   │   ├── mqtt_task.c           # Publish/subscribe
│   │   │   ├── ui_task.c             # Update LVGL
│   │   │   ├── alarm_task.c          # Display alerts
│   │   │   └── tinyml_task.c         # AI inference (Phase 8)
│   │   └── config/                    # Constants
│   │       └── config.h
│   └── build/                         # Build artifacts
│
├── raspberry-pi-server/               # Raspberry Pi Edge Server
│   ├── requirements.txt               # Python dependencies
│   ├── setup.py                       # Package setup
│   ├── app/                           # Flask/FastAPI app
│   │   ├── __init__.py
│   │   ├── main.py                   # App entry point
│   │   ├── api/                      # REST API routes
│   │   │   ├── telemetry_api.py      # GET telemetry history
│   │   │   ├── command_api.py        # POST commands to motors
│   │   │   ├── alarm_api.py          # GET alarm history
│   │   │   ├── config_api.py         # GET/SET device config
│   │   │   └── export_api.py         # CSV/JSON export
│   │   ├── models/                   # Data models
│   │   │   ├── telemetry.py          # Telemetry schema
│   │   │   ├── alarm.py              # Alarm schema
│   │   │   └── device.py             # Device config
│   │   └── utils/                    # Helpers
│   │       ├── mqtt_handler.py       # MQTT client
│   │       └── logger.py             # Logging
│   ├── db/                            # Database
│   │   ├── schema.sql                # DDL for SQLite
│   │   ├── migrations/               # Schema updates
│   │   └── models.py                 # SQLAlchemy models
│   ├── dashboard/                     # Web UI
│   │   ├── index.html                # Main page
│   │   ├── css/
│   │   │   └── style.css             # Bootstrap + custom
│   │   ├── js/
│   │   │   ├── api-client.js         # REST calls
│   │   │   ├── chart.js              # Chart.js for trends
│   │   │   ├── realtime.js           # WebSocket for live updates
│   │   │   └── dashboard.js          # UI logic
│   │   └── assets/                   # Images, icons
│   ├── mosquitto/
│   │   └── mosquitto.conf            # MQTT broker config
│   ├── systemd/
│   │   ├── motor-api.service         # Flask API service
│   │   ├── motor-logger.service      # MQTT logger service
│   │   └── mosquitto.service         # MQTT broker service
│   └── scripts/
│       ├── install.sh                # Installation script
│       ├── start_services.sh         # Start all services
│       └── export_dataset.py         # Export for TinyML training
│
└── docs/                              # Documentation (generated from README content)
    ├── architecture.md               # Detailed architecture
    ├── modbus-register-map.md        # Modbus register specifications
    ├── mqtt-topics.md                # MQTT topic structure
    ├── motor-control-design.md       # State machine & modes
    ├── tinyml-roadmap.md             # ML integration plan
    ├── wiring-diagram.md             # Hardware connections
    ├── database-schema.md            # SQLite schema details
    └── faq.md                        # Common questions
```

---

## Core Concepts

### Motor State Machine

The motor operates in four states (defined in STM32):

```
MOTOR_STATE_STOP    = 0    // PWM = 0%, motor stopped
MOTOR_STATE_RUNNING = 1    // Motor running normally
MOTOR_STATE_WARNING = 2    // Soft limit applied (e.g., temp warning)
MOTOR_STATE_FAULT   = 3    // Critical fault: PWM=0%, motor stopped, buzzer ON
```

**State Transitions**:
```
STOP ──start──> RUNNING
         ↓
      (conditions met)
         ↓
     WARNING ──(critical)──> FAULT
         ↓
      (reset)
         ↓
      STOP
```

### Control Modes

- **MANUAL (0)**: User sets PWM directly (0-100%) via HMI or MQTT
- **AUTO (1)**: STM32 automatically adjusts PWM based on LM35 temperature:
  - < 35°C: PWM = 0%
  - 35-45°C: PWM = 40%
  - 45-55°C: PWM = 70%
  - 55-65°C: PWM = 100%
  - > 65°C: FAULT state

### Modbus Register Map

**Telemetry Registers** (Read-only from master):
- `40001`: LM35 Temperature (°C × 10)
- `40002`: BMP280 Temperature (°C × 10)
- `40003`: BMP280 Pressure (hPa)
- `40004`: Motor Voltage (V × 100)
- `40005`: Motor Current (mA)
- `40006`: Motor PWM Duty (0-100%)
- `40007`: Motor State (0=STOP, 1=RUNNING, 2=WARNING, 3=FAULT)
- `40008`: Control Mode (0=MANUAL, 1=AUTO)
- `40009`: Alarm Flags (bit flags)
- `40010`: Fault Code
- `40011`: System Status
- `40012`: Sensor Error Count
- `40013`: Watchdog Reset Count

**Command Registers** (Write from master):
- `40100`: Motor Enable (0=STOP, 1=RUN)
- `40101`: Motor Speed Setpoint (0-100%)
- `40102`: Control Mode (0=MANUAL, 1=AUTO)
- `40103`: Reset Alarm (write 1 to reset)
- `40104`: Current Threshold (mA)
- `40105`: LM35 Warning Threshold (°C × 10)
- `40106`: LM35 Fault Threshold (°C × 10)
- `40107`: Buzzer Enable (0=OFF, 1=ON)
- `40108`: LED Mode (0=normal, 1=warning, 2=fault)

### MQTT Topics & Payloads

**Topic Structure**: `factory/motor01/<type>`

Types:
- `telemetry`: Periodic data (temperature, current, voltage, etc.)
- `command`: Commands from server to ESP32 (SET_MOTOR_SPEED, SET_MODE, etc.)
- `alarm`: Alarm notifications
- `status`: Connection/heartbeat status
- `ai`: AI anomaly detection results (Phase 8)

**Example Telemetry Payload**:
```json
{
  "node_id": "motor01",
  "lm35_temp": 46.5,
  "bmp280_temp": 31.2,
  "pressure": 1008,
  "motor_voltage": 12.1,
  "motor_current": 430,
  "motor_pwm": 60,
  "motor_state": "RUNNING",
  "control_mode": "MANUAL",
  "alarm_flags": 0,
  "fault_code": 0,
  "timestamp": "2026-05-21T10:30:45Z"
}
```

### Sensor Specifications

| Sensor | Interface | Range | Key Use |
|--------|-----------|-------|---------|
| **LM35** | ADC | -55 to +150°C | Motor/MOSFET temperature |
| **INA219** | I2C | 0-32V, ±3.2A max | Motor voltage & current |
| **BMP280** | I2C | -40 to +85°C, 300-1100 hPa | Environment monitoring |
| **TFT ILI9488** | SPI | 320×480 pixels | HMI display |

**Note**: BMP280 does NOT have humidity measurement - project tracks only temperature and pressure.

---

## Development Phases

### Phase 1: STM32 Hardware Bring-Up
- Initialize GPIO (LED, buzzer)
- UART debug logging
- ADC for LM35 reading
- I2C for INA219 and BMP280
- PWM for motor control
- **Duration**: Week 1-2
- **Success Criteria**: Basic sensors read correctly, motor PWM works

### Phase 2: Motor Control & Protection
- Implement motor state machine
- Manual and auto modes
- Over-current detection (INA219)
- Over-temperature detection (LM35)
- Fault flags and alarm codes
- Watchdog implementation
- **Duration**: Week 2-3
- **Success Criteria**: Safe motor control with fault recovery

### Phase 3: Modbus RTU Communication
- RS-485 module wiring
- STM32 as Modbus RTU Slave
- ESP32 as Modbus RTU Master
- Register map implementation
- Timeout and retry logic
- **Duration**: Week 3-4
- **Success Criteria**: Reliable data exchange between boards

### Phase 4: ESP32 HMI
- TFT ILI9488 driver (SPI)
- LVGL integration
- Dashboard, motor control, alarm, trend screens
- Real-time data display
- **Duration**: Week 4-5
- **Success Criteria**: Functional HMI showing live motor data

### Phase 5: MQTT & Logging
- Mosquitto MQTT broker on Raspberry Pi
- ESP32 MQTT publish/subscribe
- SQLite database schema
- Telemetry, alarm, command, training log tables
- **Duration**: Week 5-6
- **Success Criteria**: Data persisted and queryable

### Phase 6: Web Dashboard & API
- Flask/FastAPI REST API
- HTML/CSS/JavaScript dashboard
- Charts and trends
- Remote motor control
- CSV export for analytics
- **Duration**: Week 6-7
- **Success Criteria**: Full remote monitoring and control

### Phase 7: System Stabilization
- FreeRTOS optimization
- Reconnection logic (Wi-Fi, MQTT, Modbus)
- Static analysis (Cppcheck)
- Unit tests
- Documentation
- **Duration**: Week 7-8
- **Success Criteria**: Production-ready, stable operation

### Phase 8: TinyML Anomaly Detection (Future)
- Data collection and labeling
- Model training (Python on Raspberry Pi)
- Model quantization and conversion
- ESP32 inference engine
- AI status display on HMI
- **Duration**: Post-Phase 7
- **Success Criteria**: Reliable anomaly detection

---

## Key Technologies & Tools

### STM32F411 Development
- **IDE**: STM32CubeIDE
- **Toolchain**: ARM GCC (arm-none-eabi)
- **RTOS**: FreeRTOS
- **HAL**: STM32 HAL library
- **Build**: CMake

### ESP32-WROOM-32 Development
- **Framework**: ESP-IDF 5.0+
- **RTOS**: FreeRTOS (built-in)
- **UI**: LVGL 8.3+ for display
- **Build**: CMake (ESP-IDF uses CMake)
- **TinyML**: TensorFlow Lite for Microcontrollers

### Raspberry Pi 4 Development
- **OS**: Raspberry Pi OS (Debian-based)
- **Language**: Python 3.8+
- **Web Framework**: Flask or FastAPI
- **Database**: SQLite3
- **MQTT**: Mosquitto
- **Build**: pip, systemd

---

## OpenCode Skills & Resources

The project includes specialized skills for OpenCode agents to accelerate development:

### Embedded Firmware Patterns Skill
**Location**: `.opencode/skills/embedded-firmware-patterns/SKILL.md`

This skill provides comprehensive design patterns and best practices for implementing industrial embedded firmware on STM32F4 and ESP32 with FreeRTOS. It covers:

- **Layered Architecture**: HAL → Drivers → Sensors → Application layers
- **Finite State Machines**: Motor control FSM with proper transitions and guards
- **Fault Management**: Escalation rules, watchdog health checks, recovery strategies
- **RTOS Patterns**: Task coordination, queues, semaphores, ISR deferred processing
- **Communication Protocols**: Modbus RTU, MQTT, I2C/SPI drivers with error handling
- **Code Quality**: Naming conventions, module templates, review checklists
- **Implementation Roadmap**: 51-step plan from Phase 1 (sensors) to Phase 8 (TinyML)

**When to Use**: Call this skill when designing a firmware module, debugging communication failures, implementing RTOS tasks, handling faults, or optimizing code for real-time constraints.

**Example Usage**:
```
Implement a temperature sensor driver with averaging, 
timeout, and retry logic following embedded patterns
```

---

## Important Conventions & Gotchas

### Naming Conventions
- **Modbus registers**: `40XXX` (Holding Registers, 0-indexed in protocol, 1-indexed in documentation)
- **MQTT topics**: kebab-case for clarity: `factory/motor01/telemetry`
- **Function names**: snake_case in C: `motor_control_task()`, `read_lm35()`
- **Variables**: Prefix with type hint: `u8_count`, `f32_temperature`

### Hardware Considerations
- **RS-485 wiring**: Use twisted pair, termination resistors (120Ω) at both ends for long runs
- **GND reference**: Ensure common ground between STM32, ESP32, and power supply
- **ADC reference**: Use clean 3.3V supply; add 100nF cap near ADC pin
- **Motor current**: Peak current may exceed rated; use flyback diode on motor
- **TFT power**: Separate 3.3V regulator recommended to avoid brownout

### Firmware Development Tips
- **Modbus CRC**: Always verify CRC16 calculation; use libmodbus for reference
- **FreeRTOS stack**: Monitor stack usage; default 4KB may be insufficient for Modbus task
- **UART baud rate**: Start at 9600, test at 115200; check timing critical paths
- **I2C clock stretching**: Some sensors may hold SCL; allow sufficient timeout
- **ADC averaging**: Use 16+ samples for LM35 to reduce noise

### Testing Recommendations
- **Unit tests**: Test motor state transitions, alarm logic, Modbus CRC
- **Integration tests**: Test STM32↔ESP32 communication before adding MQTT
- **Load tests**: Run system at max current (with load) for 1+ hour to verify thermal behavior
- **Watchdog tests**: Verify watchdog triggers on fault, STM32 recovers properly

---

## Common Troubleshooting

### Problem: ESP32 Cannot Read STM32 via Modbus
- **Causes**: Wrong RS-485 wiring, baud rate mismatch, CRC error
- **Debug**: Use scope to check RS-485 signal levels (0-5V); verify UART TX/RX cables
- **Fix**: Add Modbus analyzer tool; test with simple ping register first

### Problem: Over-Current Fault Triggered Too Easily
- **Causes**: Threshold too low, ADC noise on current measurement
- **Debug**: Add 10x oversampling on INA219; verify threshold value in register 40104
- **Fix**: Increase threshold gradually while monitoring motor load

### Problem: HMI Display Corrupted or Flickering
- **Causes**: SPI clock too fast, insufficient power supply, ground bounce
- **Debug**: Reduce SPI clock to 5MHz (from 40MHz) and test; check 3.3V supply
- **Fix**: Add series resistor (10Ω) on SPI clock line; check PCB layout

### Problem: MQTT Connection Drops
- **Causes**: Weak Wi-Fi signal, MQTT broker overloaded, network timeouts
- **Debug**: Monitor RSSI (signal strength) on ESP32; check Mosquitto logs
- **Fix**: Add reconnect logic with exponential backoff; increase keep-alive timeout

---

## Code Style & Standards

### C Code (STM32 & ESP32)
- **Indentation**: 4 spaces (no tabs)
- **Line length**: Max 100 characters
- **Comments**: Doxygen-style for public APIs
- **Error handling**: Always check return codes; use -1 or NULL for errors
- **Memory**: Statically allocate buffers; avoid malloc in real-time tasks

**Example**:
```c
/**
 * @brief Read motor current from INA219
 * @return Current in mA, or -1 on error
 */
int16_t ina219_read_current(void) {
    uint8_t data[2];
    if (i2c_read(INA219_ADDR, INA219_CURRENT_REG, data, 2) != 0) {
        return -1; // I2C error
    }
    int16_t current = (data[0] << 8) | data[1];
    return current; // mA
}
```

### Python Code (Raspberry Pi)
- **Style**: PEP 8 compliance
- **Type hints**: Use for function parameters and returns
- **Docstrings**: Google-style for modules and functions
- **Logging**: Use `logging` module, not `print()`

**Example**:
```python
def publish_telemetry(client: mqtt.Client, topic: str, payload: dict) -> bool:
    """
    Publish telemetry data to MQTT broker.
    
    Args:
        client: MQTT client instance
        topic: MQTT topic string
        payload: Dictionary with telemetry data
    
    Returns:
        True if published successfully, False otherwise
    """
    try:
        result = client.publish(topic, json.dumps(payload), qos=1)
        return result.rc == mqtt.MQTT_ERR_SUCCESS
    except Exception as e:
        logging.error(f"MQTT publish error: {e}")
        return False
```

---

## Build Commands

### STM32 Firmware
```bash
cd stm32f411-firmware
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -- -j4
cmake --build build -- flash  # Using OpenOCD or STLink
```

### ESP32 Firmware
```bash
cd esp32-hmi-gateway
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor  # Linux/Mac
idf.py -p COM3 flash monitor          # Windows
```

### Raspberry Pi Server
```bash
cd raspberry-pi-server
pip install -r requirements.txt
python -m pytest                      # Run tests
python -m flask run --host 0.0.0.0    # Start API server
```

---

## CI/CD & Testing

### Static Analysis
```bash
# STM32/ESP32 C code
cppcheck --enable=all --suppress=missingIncludeSystem src/

# Python code
flake8 raspberry-pi-server/
pylint raspberry-pi-server/
```

### Unit Tests
- **C**: Unity framework for STM32/ESP32 tests
- **Python**: pytest for Raspberry Pi server

### Integration Tests
- Test Modbus communication with mock slave/master
- Test MQTT publish/subscribe with test broker
- Test motor state transitions under various conditions

---

## References & External Resources

- **Modbus Specification**: http://modbus.org/docs/Modbus_Application_Protocol_V1_1b3.pdf
- **STM32F411 Datasheet**: https://www.st.com/en/microcontrollers-microprocessors/stm32f411.html
- **ESP32 Documentation**: https://docs.espressif.com/projects/esp-idf/
- **LVGL Documentation**: https://docs.lvgl.io/
- **TensorFlow Lite Micro**: https://www.tensorflow.org/lite/microcontrollers

---

## Maintenance & Support

- **Issues**: Report bugs or feature requests via GitHub Issues
- **Documentation**: Keep docs/ updated with implementation changes
- **Changelog**: Update CHANGELOG.md with version releases
- **Roadmap**: Track Phase 8 (TinyML) planning in docs/tinyml-roadmap.md

---

**Last Updated**: May 2026  
**Version**: 1.0  
**Authors**: Team