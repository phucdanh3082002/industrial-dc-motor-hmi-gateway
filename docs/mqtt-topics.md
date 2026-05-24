# MQTT Topics & Payloads

Comprehensive MQTT communication specification between ESP32 and Raspberry Pi.

---

## Topic Structure

**Base Format**: `factory/motor01/<message_type>`

- **factory**: Organization/namespace
- **motor01**: Device identifier (supports motor02, motor03, etc. for multi-device systems)
- **message_type**: Data category (telemetry, command, alarm, status, config, ai)

---

## Published Topics (ESP32 → Raspberry Pi)

### 1. Topic: `factory/motor01/telemetry`

**Publication Rate**: 1 Hz (configurable, typically 1 message per second)

**QoS**: 1 (At least once delivery)

**Retention**: No (messages not retained)

**Purpose**: Periodic sensor and motor telemetry data

**Payload Format** (JSON):

```json
{
  "node_id": "motor01",
  "timestamp": "2026-05-21T10:30:45Z",
  "lm35_temp": 46.5,
  "bmp280_temp": 31.2,
  "pressure": 1008,
  "motor_voltage": 12.1,
  "motor_current": 430,
  "motor_pwm": 60,
  "motor_state": "RUNNING",
  "control_mode": "MANUAL",
  "alarm_flags": 0,
  "fault_code": 0
}
```

**Field Descriptions**:

| Field | Type | Unit | Range | Description |
|-------|------|------|-------|-------------|
| `node_id` | string | - | - | Device identifier |
| `timestamp` | ISO8601 | - | - | UTC timestamp of reading |
| `lm35_temp` | float | °C | -55 to +150 | Motor/MOSFET temperature |
| `bmp280_temp` | float | °C | -40 to +85 | Ambient temperature |
| `pressure` | integer | hPa | 300-1100 | Atmospheric pressure |
| `motor_voltage` | float | V | 0-33 | Motor supply voltage |
| `motor_current` | integer | mA | 0-3200 | Motor current draw |
| `motor_pwm` | integer | % | 0-100 | PWM duty cycle |
| `motor_state` | string | - | STOP/RUNNING/WARNING/FAULT | Motor operational state |
| `control_mode` | string | - | MANUAL/AUTO | Control mode |
| `alarm_flags` | integer | hex | 0x00-0xFF | Bitmask of active alarms |
| `fault_code` | integer | - | 0-9 | Fault code (0=none) |

**Storage**: Logged to SQLite `telemetry` table for historical analysis

---

### 2. Topic: `factory/motor01/alarm`

**Publication Rate**: On-demand (when alarm state changes)

**QoS**: 1

**Retention**: Yes (last alarm is retained for new subscribers)

**Purpose**: Real-time alarm and fault notifications

**Payload Variants**:

#### Alarm Trigger:
```json
{
  "node_id": "motor01",
  "timestamp": "2026-05-21T10:35:12Z",
  "alarm_type": "OVER_CURRENT",
  "alarm_level": "CRITICAL",
  "fault_code": 1,
  "description": "Motor current (450mA) exceeded threshold (400mA)",
  "motor_current": 450,
  "threshold": 400
}
```

#### Alarm Recovery:
```json
{
  "node_id": "motor01",
  "timestamp": "2026-05-21T10:35:20Z",
  "alarm_type": "OVER_CURRENT",
  "alarm_level": "CLEARED",
  "fault_code": 0,
  "description": "Alarm cleared - motor current (200mA) below threshold",
  "motor_current": 200,
  "threshold": 400
}
```

**Alarm Types**:
| Type | Level | Action | Reset Condition |
|------|-------|--------|-----------------|
| OVER_CURRENT | CRITICAL | Motor stops | Current < threshold |
| OVER_TEMPERATURE | CRITICAL | Motor stops | Temp < fault threshold |
| TEMP_WARNING | WARNING | PWM limited | Temp < warning threshold |
| SENSOR_ERROR | ERROR | Alert | Sensor recovers/valid |
| COMMUNICATION_FAULT | ERROR | Alert | Connection restored |
| WATCHDOG_RESET | WARNING | Alert | System recovered |

**Storage**: Logged to SQLite `alarm_log` table

---

### 3. Topic: `factory/motor01/status`

**Publication Rate**: Every 30 seconds (heartbeat)

**QoS**: 1

**Retention**: Yes

**Purpose**: Connection health and system status

**Payload**:

```json
{
  "node_id": "motor01",
  "timestamp": "2026-05-21T10:30:45Z",
  "esp32_uptime_seconds": 3605,
  "mqtt_connected": true,
  "modbus_connected": true,
  "wifi_rssi": -45,
  "wifi_ssid": "FactoryNet",
  "modbus_poll_count": 3605,
  "modbus_error_count": 2,
  "mqtt_publish_count": 3605,
  "heap_free": 98765,
  "nvs_free": 512000
}
```

**Field Descriptions**:

| Field | Type | Description |
|-------|------|-------------|
| `esp32_uptime_seconds` | integer | Time since ESP32 boot |
| `mqtt_connected` | boolean | MQTT broker connection status |
| `modbus_connected` | boolean | STM32 Modbus communication status |
| `wifi_rssi` | integer | WiFi signal strength (dBm), -50 to -100 typical |
| `wifi_ssid` | string | Connected WiFi network name |
| `modbus_poll_count` | integer | Total successful Modbus polls |
| `modbus_error_count` | integer | Total failed Modbus communications |
| `mqtt_publish_count` | integer | Total MQTT messages published |
| `heap_free` | integer | Available ESP32 heap memory (bytes) |
| `nvs_free` | integer | Available NVS storage (bytes) |

**Diagnostic Use**: Track ESP32 health, memory leaks, communication reliability

---

### 4. Topic: `factory/motor01/ai` (Phase 8)

**Publication Rate**: 1 Hz (when TinyML enabled)

**QoS**: 1

**Retention**: No

**Purpose**: AI anomaly detection results

**Payload - Normal State**:

```json
{
  "node_id": "motor01",
  "timestamp": "2026-05-21T10:30:45Z",
  "ai_status": "NORMAL",
  "anomaly_score": 0.12,
  "predicted_state": "NORMAL",
  "model_version": "v1.2",
  "inference_time_ms": 25
}
```

**Payload - Anomaly Detected**:

```json
{
  "node_id": "motor01",
  "timestamp": "2026-05-21T10:35:12Z",
  "ai_status": "ANOMALY",
  "anomaly_score": 0.87,
  "predicted_state": "HEAVY_LOAD_OR_STALL",
  "model_version": "v1.2",
  "inference_time_ms": 28,
  "alert": true,
  "explanation": "Motor current spike detected with normal voltage - possible load increase"
}
```

**AI Status Values**:
| Status | Anomaly Score Range | Meaning |
|--------|-------------------|---------|
| NORMAL | 0.0 - 0.3 | Normal motor operation |
| WARNING | 0.3 - 0.7 | Potential issue detected |
| ANOMALY | 0.7 - 1.0 | Significant anomaly detected |

**Predicted States**:
- NORMAL
- HEAVY_LOAD
- OVERHEAT_RISK
- STALL_RISK
- SENSOR_MALFUNCTION

**Storage**: Logged to SQLite `ai_result` table

---

## Subscribed Topics (Raspberry Pi → ESP32)

### 1. Topic: `factory/motor01/command`

**Publication Rate**: On-demand (when commands issued)

**QoS**: 1

**Retention**: No

**Purpose**: Control commands from Raspberry Pi to ESP32

**Payload Variants**:

#### Start Motor (Manual Mode):
```json
{
  "command_id": "cmd_001",
  "command": "SET_MOTOR_SPEED",
  "value": 60,
  "timestamp": "2026-05-21T10:31:00Z"
}
```
- Sets motor PWM to 60% (only in MANUAL mode)
- Ignores if in AUTO mode

#### Stop Motor:
```json
{
  "command_id": "cmd_002",
  "command": "MOTOR_STOP",
  "timestamp": "2026-05-21T10:31:05Z"
}
```
- Immediately stops motor (PWM → 0%)
- Transitions to STOP state

#### Switch Control Mode:
```json
{
  "command_id": "cmd_003",
  "command": "SET_MODE",
  "value": "AUTO",
  "timestamp": "2026-05-21T10:31:10Z"
}
```
- Value: "MANUAL" or "AUTO"
- AUTO mode uses LM35 temperature for PWM control

#### Reset Alarm:
```json
{
  "command_id": "cmd_004",
  "command": "RESET_ALARM",
  "timestamp": "2026-05-21T10:31:15Z"
}
```
- Clears fault state and alarms
- Only succeeds if system is safe

#### Update Thresholds:
```json
{
  "command_id": "cmd_005",
  "command": "UPDATE_THRESHOLDS",
  "current_threshold_ma": 1500,
  "temp_warning_celsius": 55,
  "temp_fault_celsius": 65,
  "timestamp": "2026-05-21T10:31:20Z"
}
```
- Modify protection thresholds dynamically

#### Configure Buzzer/LED:
```json
{
  "command_id": "cmd_006",
  "command": "SET_ALERTS",
  "buzzer_enabled": true,
  "led_mode": 0,
  "timestamp": "2026-05-21T10:31:25Z"
}
```
- Control audio/visual alarm signals

**Command Response**: ESP32 acknowledges command execution via `command_status` topic

---

### 2. Topic: `factory/motor01/config`

**Publication Rate**: On-demand (configuration changes)

**QoS**: 1

**Retention**: Yes (configuration retained across reconnections)

**Purpose**: Device configuration and parameters

**Payload**:

```json
{
  "node_id": "motor01",
  "modbus_baudrate": 115200,
  "mqtt_publish_interval_ms": 1000,
  "modbus_poll_interval_ms": 500,
  "brightness": 100,
  "language": "en",
  "timezone": "UTC+7",
  "ntp_server": "pool.ntp.org",
  "timestamp": "2026-05-21T10:31:00Z"
}
```

**Configuration Parameters**:

| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| `modbus_baudrate` | integer | 9600/115200 | 115200 | RS-485 communication speed |
| `mqtt_publish_interval_ms` | integer | 100-10000 | 1000 | Telemetry publish frequency |
| `modbus_poll_interval_ms` | integer | 100-2000 | 500 | Modbus read frequency |
| `brightness` | integer | 0-100 | 100 | TFT display brightness |
| `language` | string | en/vi/zh | en | UI language |
| `timezone` | string | UTC±HH:MM | UTC | Local timezone |
| `ntp_server` | string | FQDN | pool.ntp.org | Time synchronization server |

**Storage**: Configuration persisted in ESP32 NVS (Non-Volatile Storage)

---

## Response Topics (ESP32 → Raspberry Pi)

### Topic: `factory/motor01/command_status`

**Purpose**: Acknowledge command execution results

**Payload**:

```json
{
  "command_id": "cmd_001",
  "status": "SUCCESS",
  "result": {
    "motor_pwm": 60,
    "motor_state": "RUNNING"
  },
  "timestamp": "2026-05-21T10:31:01Z"
}
```

Status values: `SUCCESS`, `FAILED`, `INVALID_COMMAND`, `DEVICE_ERROR`

---

## MQTT QoS & Reliability

| Topic | QoS | Reason |
|-------|-----|--------|
| telemetry | 1 | At-least-once delivery, can tolerate duplicates |
| alarm | 1 | Critical - ensure delivery |
| command | 1 | Critical - must execute exactly once |
| status | 1 | Health monitoring - ensure delivery |
| config | 1 | Configuration changes must be reliable |
| ai | 1 | AI results should be captured |

---

## Error Handling & Retry Logic

### MQTT Broker Unavailable
- ESP32 queues messages in local buffer (up to 50 messages)
- Automatic reconnection with exponential backoff
- Backoff sequence: 1s, 2s, 4s, 8s, 30s (max)

### Invalid Command
```json
{
  "command_id": "cmd_007",
  "status": "FAILED",
  "error": "INVALID_COMMAND",
  "message": "Unknown command type: 'INVALID_OP'",
  "timestamp": "2026-05-21T10:31:30Z"
}
```

### Device-Side Error
```json
{
  "command_id": "cmd_008",
  "status": "DEVICE_ERROR",
  "error": "MOTOR_FAULT",
  "message": "Cannot execute command - motor in FAULT state",
  "timestamp": "2026-05-21T10:31:35Z"
}
```

---

## Bandwidth Estimation

### Typical Telemetry (1 Hz)
- Message Size: ~300 bytes
- Per Hour: 300 bytes × 3600 = 1.08 MB
- Per Day: 25.9 MB

### Alarms (Variable, ~10/day assumed)
- Per Day: 10 × 400 bytes = 4 KB

### Status (Every 30s)
- Per Hour: 120 × 500 bytes = 60 KB
- Per Day: 1.44 MB

### Total Daily Usage (Estimated)
- **Low Activity**: ~27 MB
- **High Activity (100 alarms/day)**: ~27.5 MB
- **Network Suitable**: LTE, 4G, fixed broadband

---

## Scaling to Multiple Devices

For systems with multiple motor nodes, extend topic structure:

```
factory/motor01/telemetry
factory/motor02/telemetry
factory/motor03/telemetry
...
factory/motorNN/telemetry
```

**Subscription Pattern**:
- Subscribe to `factory/+/telemetry` to receive all motor telemetry
- Each device uses unique `node_id` for identification
- Database design already supports multiple nodes (`node_id` field)

---

## Security Best Practices

1. **MQTT Broker Authentication**: Enable username/password
2. **TLS/SSL Encryption**: Encrypt MQTT traffic (port 8883)
3. **Topic Authorization**: Use ACL to restrict device-specific topics
4. **Payload Encryption**: Additional layer for sensitive data (optional)
5. **Firewall Rules**: Restrict broker access to trusted networks

---

**Last Updated**: May 2026  
**Version**: 1.0
