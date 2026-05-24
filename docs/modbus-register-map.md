# Modbus RTU Register Map

STM32F411 implements Modbus RTU Slave with comprehensive register mapping for telemetry and command data.

---

## Overview

- **Protocol**: Modbus RTU
- **Slave ID**: 1
- **CRC**: CRC16/Modbus
- **Byte Order**: Big-endian (Modbus standard)
- **Register Type**: Holding Registers (Function Code 03/16)

---

## Telemetry Registers (Read-Only from Master)

All telemetry registers are read-only. ESP32 polls these registers periodically (500ms - 1s interval).

### Register 40001: LM35 Temperature

**Description**: Temperature measured near motor or MOSFET

| Property | Value |
|----------|-------|
| Address | 40001 |
| Data Type | int16 |
| Unit | °C × 10 |
| Range | -550 to +1500 (-55°C to +150°C) |
| Access | Read Only |
| Update Rate | ~100ms |

**Example**:
- ADC raw: 512 (0x200)
- Voltage: 0.4125V (512 / 4095 × 3.3V)
- Temperature: 41.25°C
- Register Value: 412 (sent as 41.2°C × 10)

---

### Register 40002: BMP280 Temperature

**Description**: Ambient temperature from environmental sensor

| Property | Value |
|----------|-------|
| Address | 40002 |
| Data Type | int16 |
| Unit | °C × 10 |
| Range | -400 to +850 (-40°C to +85°C) |
| Access | Read Only |
| Update Rate | ~1s (I2C poll) |

---

### Register 40003: BMP280 Pressure

**Description**: Atmospheric pressure reading

| Property | Value |
|----------|-------|
| Address | 40003 |
| Data Type | uint16 |
| Unit | hPa (hectopascals) |
| Range | 300 to 1100 hPa |
| Access | Read Only |
| Update Rate | ~1s (I2C poll) |

**Note**: Typical sea level pressure is ~1013 hPa. This can be used for altitude calculation or environmental monitoring.

---

### Register 40004: Motor Voltage

**Description**: Supply voltage to the DC motor

| Property | Value |
|----------|-------|
| Address | 40004 |
| Data Type | uint16 |
| Unit | V × 100 |
| Range | 0 to 3300 (0V to 33V) |
| Access | Read Only |
| Update Rate | ~100ms |
| Sensor | INA219 |

**Example**:
- INA219 reading: 12.1V
- Register Value: 1210 (represents 12.1V × 100)

---

### Register 40005: Motor Current

**Description**: Current drawn by the DC motor

| Property | Value |
|----------|-------|
| Address | 40005 |
| Data Type | uint16 |
| Unit | mA (milliamps) |
| Range | 0 to 3200 (0 to 3.2A) |
| Access | Read Only |
| Update Rate | ~100ms |
| Sensor | INA219 |

**Example**:
- Motor load: 430mA
- Register Value: 430

---

### Register 40006: Motor PWM Duty Cycle

**Description**: Current PWM output to motor

| Property | Value |
|----------|-------|
| Address | 40006 |
| Data Type | uint16 |
| Unit | % (0-100%) |
| Range | 0 to 100 |
| Access | Read Only |
| Update Rate | ~10ms |

**Interpretation**:
- 0 = Motor off (0% duty)
- 50 = Motor at 50% speed
- 100 = Motor at full speed

---

### Register 40007: Motor State

**Description**: Current operational state of the motor

| Property | Value |
|----------|-------|
| Address | 40007 |
| Data Type | uint16 |
| Unit | State enum |
| Range | 0-3 |
| Access | Read Only |
| Update Rate | ~10ms |

**State Values**:
| Value | State | Meaning |
|-------|-------|---------|
| 0 | STOP | Motor disabled, PWM=0% |
| 1 | RUNNING | Motor operating normally |
| 2 | WARNING | Soft limit applied (e.g., temp warning), reduced PWM |
| 3 | FAULT | Critical fault detected, PWM=0%, requires reset |

---

### Register 40008: Control Mode

**Description**: Active motor control mode

| Property | Value |
|----------|-------|
| Address | 40008 |
| Data Type | uint16 |
| Unit | Mode enum |
| Range | 0-1 |
| Access | Read Only |

**Mode Values**:
| Value | Mode | Meaning |
|-------|------|---------|
| 0 | MANUAL | User controls PWM via ESP32 commands |
| 1 | AUTO | STM32 automatically adjusts PWM based on temperature |

---

### Register 40009: Alarm Flags

**Description**: Bit flags indicating active alarms

| Property | Value |
|----------|-------|
| Address | 40009 |
| Data Type | uint16 |
| Unit | Bitmask |
| Access | Read Only |

**Bit Assignments**:
| Bit | Mask | Alarm Type |
|-----|------|-----------|
| 0 | 0x01 | Over-current detected |
| 1 | 0x02 | Temperature warning (LM35) |
| 2 | 0x04 | Over-temperature (LM35) |
| 3 | 0x08 | INA219 sensor error |
| 4 | 0x10 | BMP280 sensor error |
| 5 | 0x20 | RS-485 communication timeout |
| 6 | 0x40 | MQTT disconnected (logged by ESP32) |
| 7 | 0x80 | Watchdog reset occurred |

**Example**:
- Register Value: 0x05 (binary: 00000101)
- Meaning: Over-current (bit 0) + Temperature warning (bit 1) active

---

### Register 40010: Fault Code

**Description**: Detailed fault code for diagnostics

| Property | Value |
|----------|-------|
| Address | 40010 |
| Data Type | uint16 |
| Access | Read Only |

**Fault Codes**:
| Code | Fault Type | Description |
|------|-----------|-------------|
| 0 | NONE | No fault |
| 1 | OVERCURRENT | Motor current exceeded threshold |
| 2 | OVERHEAT | Motor temperature exceeded fault threshold |
| 3 | TEMP_WARNING | Motor temperature in warning range |
| 4 | SENSOR_LM35 | LM35 reading invalid or out of range |
| 5 | SENSOR_INA219 | INA219 I2C communication error |
| 6 | SENSOR_BMP280 | BMP280 I2C communication error |
| 7 | WATCHDOG_RESET | System recovered from watchdog timeout |
| 8 | MODBUS_CRC_ERROR | Modbus packet CRC validation failed |
| 9 | UNKNOWN_ERROR | Unknown/reserved error |

---

### Register 40011: System Status

**Description**: Overall system health status

| Property | Value |
|----------|-------|
| Address | 40011 |
| Data Type | uint16 |
| Access | Read Only |

**Status Values**:
| Value | Status |
|-------|--------|
| 0 | INITIALIZING |
| 1 | READY (idle, no motor activity) |
| 2 | RUNNING (motor active) |
| 3 | ERROR (needs investigation) |
| 4 | RECOVERY (attempting recovery from error) |

---

### Register 40012: Sensor Error Count

**Description**: Cumulative count of sensor reading errors

| Property | Value |
|----------|-------|
| Address | 40012 |
| Data Type | uint16 |
| Range | 0 to 65535 |
| Access | Read Only |

**Tracks**:
- I2C communication failures (INA219, BMP280)
- ADC reading errors (LM35)
- Invalid sensor readings

---

### Register 40013: Watchdog Reset Count

**Description**: Number of times system recovered from watchdog timeout

| Property | Value |
|----------|-------|
| Address | 40013 |
| Data Type | uint16 |
| Range | 0 to 65535 |
| Access | Read Only |

**Significance**: High reset count indicates system stability issues (e.g., task deadlock, stack overflow).

---

## Command Registers (Write from Master)

Command registers allow ESP32 to control STM32 motor and configuration.

### Register 40100: Motor Enable

**Description**: Enable/disable motor operation

| Property | Value |
|----------|-------|
| Address | 40100 |
| Data Type | uint16 |
| Access | Write Only |
| Effect | Immediate (next control cycle) |

**Values**:
| Value | Action |
|-------|--------|
| 0 | Stop motor (set state to STOP) |
| 1 | Enable motor (transition to RUNNING if safe) |

---

### Register 40101: Motor Speed Setpoint

**Description**: Desired motor PWM duty cycle (Manual mode only)

| Property | Value |
|----------|-------|
| Address | 40101 |
| Data Type | uint16 |
| Unit | % (0-100%) |
| Range | 0 to 100 |
| Access | Write Only |
| Validity | Only used when Control Mode = MANUAL |

**Behavior**:
- If mode is MANUAL, apply this PWM immediately
- If mode is AUTO, this value is ignored
- If motor in FAULT state, PWM not applied until fault reset

---

### Register 40102: Control Mode

**Description**: Switch between manual and automatic motor control

| Property | Value |
|----------|-------|
| Address | 40102 |
| Data Type | uint16 |
| Access | Write Only |
| Effect | Immediate (next control cycle) |

**Values**:
| Value | Mode | Behavior |
|-------|------|----------|
| 0 | MANUAL | Use register 40101 (Speed Setpoint) for PWM |
| 1 | AUTO | STM32 adjusts PWM based on LM35 temperature |

**Auto Mode Temperature Mapping**:
| LM35 Temperature | Motor PWM | State |
|------------------|-----------|-------|
| < 35°C | 0% | STOP |
| 35-45°C | 40% | RUNNING |
| 45-55°C | 70% | RUNNING |
| 55-65°C | 100% | RUNNING |
| > 65°C | 0% | FAULT |

---

### Register 40103: Reset Alarm

**Description**: Clear active alarm/fault conditions

| Property | Value |
|----------|-------|
| Address | 40103 |
| Data Type | uint16 |
| Access | Write Only |

**Values**:
| Value | Action |
|-------|--------|
| 0 | No action |
| 1 | Reset all alarms, clear fault state (if safe) |

**Safety Check**: Reset only succeeds if:
- Motor is not in immediate danger
- Temperature is below warning threshold
- Current is below threshold
- Sensor readings are valid

---

### Register 40104: Current Threshold

**Description**: Maximum motor current before over-current fault

| Property | Value |
|----------|-------|
| Address | 40104 |
| Data Type | uint16 |
| Unit | mA (milliamps) |
| Range | 0 to 3200 |
| Access | Write Only |
| Default | 1500 mA (typical) |

**Example**:
- Set to 1500 mA for maximum safe current
- Motor exceeding 1500mA triggers over-current fault

---

### Register 40105: LM35 Warning Threshold

**Description**: Temperature threshold for soft warning (PWM limitation)

| Property | Value |
|----------|-------|
| Address | 40105 |
| Data Type | int16 |
| Unit | °C × 10 |
| Range | -550 to +1500 |
| Access | Write Only |
| Default | 550 (55°C) |

**Behavior**:
- When LM35 exceeds this threshold, motor state = WARNING
- PWM is limited to reduce heat generation
- Does not stop motor completely

---

### Register 40106: LM35 Fault Threshold

**Description**: Critical temperature threshold triggering motor shutdown

| Property | Value |
|----------|-------|
| Address | 40106 |
| Data Type | int16 |
| Unit | °C × 10 |
| Range | -550 to +1500 |
| Access | Write Only |
| Default | 650 (65°C) |

**Behavior**:
- When LM35 exceeds this threshold, motor state = FAULT
- PWM set to 0%
- Motor stops immediately
- Requires manual reset via register 40103

---

### Register 40107: Buzzer Enable

**Description**: Control audio alarm buzzer

| Property | Value |
|----------|-------|
| Address | 40107 |
| Data Type | uint16 |
| Access | Write Only |

**Values**:
| Value | State |
|-------|-------|
| 0 | Buzzer disabled (muted) |
| 1 | Buzzer enabled (sound on alarm) |

---

### Register 40108: LED Mode

**Description**: Control LED indicator behavior

| Property | Value |
|----------|-------|
| Address | 40108 |
| Data Type | uint16 |
| Access | Write Only |

**LED Modes**:
| Value | Mode | Behavior |
|-------|------|----------|
| 0 | NORMAL | Solid or slow blink (normal operation) |
| 1 | WARNING | Fast blink (temperature warning) |
| 2 | FAULT | Rapid blink or solid (critical fault) |

---

## Register Access Patterns

### Typical ESP32 Polling Cycle (1s interval)

```
1. Read telemetry registers (40001-40013)
   └─ Extract motor state, temperature, current, voltage
2. Check alarm flags (register 40009)
   └─ Publish alarms to MQTT if changed
3. Send command if needed (registers 40100-40108)
   └─ Update speed, mode, thresholds
4. Update TFT display with latest data
5. Publish telemetry to MQTT broker
6. Wait for next cycle
```

### Emergency Stop Sequence

```
1. Write 0 to register 40100 (Motor Enable) → Motor stops immediately
2. Read register 40009 (Alarm Flags) to verify stop
3. If fault detected, write 1 to register 40103 (Reset Alarm) after fixing issue
```

### Mode Switching (Manual → Auto)

```
1. Write 1 to register 40102 (Control Mode = AUTO)
2. STM32 immediately begins using LM35 temperature for PWM control
3. Manual speed setpoint (40101) is ignored until mode switched back
```

---

## Error Handling

### Modbus CRC Error
- Frame discarded
- No register update
- ESP32 retries after timeout
- Error logged in Raspberry Pi

### Invalid Register Address
- Modbus exception response (code 02)
- No state change
- Request rejected

### Invalid Data (Out of Range)
- Write rejected for out-of-range values
- Existing register value unchanged
- No exception thrown

### Timeout (No Response)
- ESP32 retries after 200ms
- After 3 retries, mark communication as FAULT
- Set alarm flag ALARM_RS485_TIMEOUT

---

## Performance Notes

- **Modbus RTU Speed**: 9600 bps (initial) → 115200 bps (optimized)
- **Maximum Packet Size**: 13 registers in single read
- **Typical Read Time**: ~50ms at 115200 bps
- **Write Time**: ~30ms per register
- **Polling Recommendation**: 500ms - 1s for stability

---

**Last Updated**: May 2026  
**Version**: 1.0
