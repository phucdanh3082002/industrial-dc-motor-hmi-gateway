# Motor Control Design

Comprehensive motor control strategy including state machine, modes, and protection mechanisms.

---

## Motor State Machine

The STM32 implements a finite state machine to manage motor operational states safely.

### State Definitions

```c
typedef enum {
    MOTOR_STATE_STOP = 0,      // Motor disabled, PWM=0%, safe to start
    MOTOR_STATE_RUNNING = 1,   // Normal operation, motor active
    MOTOR_STATE_WARNING = 2,   // Soft limit applied (e.g., temp high), reduced PWM
    MOTOR_STATE_FAULT = 3      // Critical fault, requires manual reset
} motor_state_t;
```

### State Diagram

```
                    START/ENABLE
                         ↓
        ┌─────────────────────────────────────┐
        │                                     │
        ▼                                     │
    ┌──────────┐                              │
    │   STOP   │◄─────────────┐               │
    └────┬─────┘              │               │
         │                    │               │
         │ Start Motor        │ Manual Reset  │
         │ (Safe Conditions)  │ (Conditions OK)
         │                    │               │
         ▼                    │               │
    ┌──────────┐              │               │
    │ RUNNING  │              │               │
    └────┬─────┘              │               │
         │                    │               │
         │ Soft Limit         │               │
         │ (Temp Warning)     │               │
         │                    │               │
         ▼                    │               │
    ┌──────────┐              │               │
    │ WARNING  │──────────────┘               │
    └────┬─────┘                              │
         │                                    │
         │ Critical Condition                 │
         │ (Over-current OR Overheat)         │
         │                                    │
         ▼                                    │
    ┌──────────┐                              │
    │  FAULT   │──────────────────────────────┘
    └──────────┘
         ▲
         │
         └──────────────────────────────────┘
            (Only via manual reset command)
```

### State Transitions

| From | To | Condition | Action |
|------|----|-----------|---------
| STOP | RUNNING | Start command + safe conditions | Set PWM to target value |
| RUNNING | WARNING | Soft limit (temp warning) | Reduce PWM to safe value |
| WARNING | RUNNING | Condition cleared | Restore PWM to target |
| WARNING | FAULT | Critical condition detected | Set PWM to 0%, sound alarm |
| RUNNING | FAULT | Critical condition detected | Set PWM to 0%, sound alarm |
| FAULT | STOP | Reset alarm command (safe) | Clear fault flags |
| STOP | STOP | Stop command | Stay at PWM=0% |

### Safe Conditions for Starting
Motor can transition from STOP to RUNNING only when:
1. Current sensor operational and reading normal
2. Temperature sensor operational and reading normal
3. No active faults or alarms
4. System has recovered from any previous error

---

## Control Modes

### Manual Mode

**Definition**: User directly specifies PWM duty cycle

**Operation**:
- ESP32 sends desired PWM to STM32 via Modbus register 40101
- STM32 applies PWM immediately (subject to safety limits)
- Motor speed controlled by HMI slider or web interface

**Flow Diagram**:
```
User Input (HMI)
        ↓
Set Speed via Web/TFT
        ↓
ESP32 Modbus Write to Register 40101
        ↓
STM32 Receives Command
        ↓
Verify Motor State is RUNNING
        ↓
Apply PWM (with protection checks)
        ↓
Read Current/Temp
        ↓
Update Motor State if needed
```

**Safety Constraints**:
- Even in MANUAL mode, over-current and over-temperature protection is active
- Motor automatically enters FAULT state if thresholds exceeded
- PWM override ignored if motor in FAULT or STOP state

**Use Cases**:
- Calibration and testing
- Direct operator control
- Load testing
- Quick response to changing load

---

### Auto Mode

**Definition**: STM32 automatically adjusts PWM based on LM35 temperature

**Temperature-to-PWM Mapping**:

| Temperature Range | PWM Duty | Motor State | Description |
|-------------------|----------|-------------|-------------|
| < 35°C | 0% | STOP | Motor off, cooling phase |
| 35-45°C | 40% | RUNNING | Low speed, warming up |
| 45-55°C | 70% | RUNNING | Medium speed, stable operation |
| 55-65°C | 100% | RUNNING | Full speed, maximum cooling |
| > 65°C | 0% | FAULT | Critical overheat, motor stops |

**Hysteresis** (to prevent oscillation):
- Temperature increase threshold: Exact value (e.g., 55°C)
- Temperature decrease threshold: -2°C (e.g., drop to 53°C to restore PWM)

**Algorithm**:
```
1. Read LM35 temperature
2. IF temp > 65°C THEN
       Set motor state = FAULT, PWM = 0%
   ELSE IF temp > 55°C AND current_pwm < 100% THEN
       Set PWM = 100%
   ELSE IF temp > 45°C AND current_pwm < 70% THEN
       Set PWM = 70%
   ELSE IF temp > 35°C AND current_pwm < 40% THEN
       Set PWM = 40%
   ELSE IF temp < 35°C THEN
       Set PWM = 0%, motor state = STOP
   END IF
3. Wait for next cycle
```

**Flow Diagram**:
```
Auto Mode Enabled
        ↓
Read LM35 Temperature
        ↓
Apply Temperature Mapping
        ↓
Adjust PWM Accordingly
        ↓
Read Current & Verify Safe
        ↓
Update Motor State
        ↓
Update Modbus Register 40006 (PWM)
```

**Use Cases**:
- Steady-state operation with varying ambient temperature
- Reduced operator intervention
- Predictable thermal management
- Energy efficiency when load is variable

---

## Fault Mode

**Definition**: Motor enters FAULT state when critical condition detected

**Triggers**:
1. **Over-current**: INA219 reading > current threshold (register 40104)
2. **Over-temperature**: LM35 reading > fault threshold (register 40106)
3. **Sensor Error**: I2C read failure from INA219/BMP280
4. **Communication Timeout**: No valid Modbus response from ESP32

**Actions on FAULT**:
1. Set motor state = FAULT
2. Set PWM = 0% (motor stops immediately)
3. Set alarm flag bit (Alarm Flags register 40009)
4. Record fault code (Fault Code register 40010)
5. Activate buzzer (if enabled)
6. Activate LED fault indicator
7. Publish alarm to MQTT

**Recovery Requirements**:
- Execute RESET_ALARM command (Modbus register 40103 = 1)
- Verify safe conditions are met:
  - Current below threshold
  - Temperature below warning threshold
  - No active sensor errors
- Motor transitions FAULT → STOP → RUNNING (on next start command)

**Fault Code Reference**:

| Code | Type | Cause | Recovery |
|------|------|-------|----------|
| 0 | None | No fault | N/A |
| 1 | OVERCURRENT | Current exceeded limit | Check load, verify current threshold |
| 2 | OVERHEAT | Temp exceeded fault limit | Allow cooling, check LM35 sensor |
| 3 | TEMP_WARNING | Temp in warning range | Normal operation, PWM reduced |
| 4 | SENSOR_LM35 | LM35 I2C error | Check LM35 wiring, I2C bus |
| 5 | SENSOR_INA219 | INA219 I2C error | Check INA219 wiring, I2C bus |
| 6 | SENSOR_BMP280 | BMP280 I2C error | Check BMP280 wiring, I2C bus |
| 7 | WATCHDOG_RESET | Watchdog triggered | System recovered, investigate cause |
| 8 | MODBUS_CRC_ERROR | CRC validation failed | Check RS-485 wiring, try retry |
| 9 | UNKNOWN_ERROR | Reserved for future use | Contact support |

---

## Motor PWM Control

### PWM Generation

**Hardware**: STM32F411 Timer (PWM output on GPIO)

**Specifications**:
- **Frequency**: 20 kHz (typical for MOSFET efficiency)
- **Resolution**: 8-bit (0-255 counts → 0-100%)
- **Output**: Active high to MOSFET gate via 100Ω series resistor

**PWM to Speed Relationship** (non-linear, depends on load):
```
PWM Duty | Approximate Speed | Current Typical |
0%       | Motor off         | 0-10 mA idle    |
20%      | ~10-15 rpm        | 50-100 mA       |
40%      | ~30-40 rpm        | 150-250 mA      |
60%      | ~50-60 rpm        | 300-400 mA      |
80%      | ~70-80 rpm        | 400-500 mA      |
100%     | ~100 rpm max      | 500-800 mA peak |
```

**Update Rate**: 10 ms (100 Hz frequency)

---

## Protection Mechanisms

### Over-Current Protection

**Sensor**: INA219 (I2C interface)

**Threshold**: Configurable via Modbus register 40104 (default: 1500 mA)

**Response**:
1. Detect current > threshold
2. Set alarm flag: ALARM_OVER_CURRENT
3. Record fault code: 1 (OVERCURRENT)
4. Set motor state = FAULT
5. Set PWM = 0%
6. Publish alarm to MQTT

**Detection Latency**: ~100 ms (single ADC cycle + I2C read)

**Typical Causes**:
- Sudden load increase
- Motor mechanical jam
- Short circuit in motor winding
- Bearing failure

**Resolution**:
- Remove load or clear obstruction
- Verify motor connections
- Check for mechanical damage
- Execute RESET_ALARM command

---

### Over-Temperature Protection

**Sensor**: LM35 (ADC input)

**Thresholds**:
- **Warning Threshold**: Configurable (default: 55°C, register 40105)
  - Response: Limit PWM, transition to WARNING state
  - Does NOT stop motor
  
- **Fault Threshold**: Configurable (default: 65°C, register 40106)
  - Response: Stop motor, transition to FAULT state
  - Requires manual reset

**Temperature Reading**:
- **Sampling Rate**: ~100 Hz (ADC)
- **Filtering**: 16-sample moving average for noise reduction
- **Accuracy**: ±1°C (typical LM35 specification)

**Response Timeline**:
```
55°C (Warning)  → State = WARNING, PWM limited
         ↓ (increase)
60°C           → PWM further limited
         ↓ (increase)
65°C (Fault)    → State = FAULT, PWM = 0%, motor stops
         ↓ (decrease)
63°C (hysteresis) → Condition improving, ready for reset
```

**Typical Causes of Overheat**:
- Prolonged high-load operation
- Inadequate cooling/ventilation
- Motor bearing failure
- Continuous 100% PWM operation

**Prevention**:
- Implement duty-cycle limits (e.g., max 80% continuous)
- Ensure adequate airflow around motor
- Regular preventive maintenance
- Consider PWM ramp-up instead of sudden acceleration

---

### Sensor Error Handling

**LM35 Error Conditions**:
- ADC reading out of range (-55 to +150°C)
- Multiple consecutive invalid readings
- I2C timeout (if using I2C variant)

**INA219 Error Conditions**:
- I2C communication failure
- Invalid current reading (< 0 or > max)
- Chip ID validation failure

**BMP280 Error Conditions**:
- I2C communication failure
- Invalid temperature or pressure reading
- Chip ID validation failure
- Calibration data corrupted

**Recovery Strategy**:
1. Retry sensor read (3 attempts)
2. If all retries fail:
   - Set sensor error flag
   - Continue with last valid value (if available)
   - Increment error counter (register 40012)
3. If errors persist (>10 consecutive):
   - Trigger alert to ESP32
   - Publish alarm to MQTT
   - Motor transitions to fault state (safety first)

---

## Alarm Flags

**Register**: 40009 (Alarm Flags)

**Format**: 16-bit bitmask, bit 0 = LSB

```
Bit 7 | Bit 6 | Bit 5 | Bit 4 | Bit 3 | Bit 2 | Bit 1 | Bit 0
  -   |MQTT   | RS485 |BMP280 |INA219 |OverT  |TempW  |OverI
      |Disc   |TO     |Err    |Err    |       |       |
```

**Bit Definitions**:
- **Bit 0**: Over-current detected
- **Bit 1**: Temperature warning (soft limit)
- **Bit 2**: Over-temperature (fault)
- **Bit 3**: INA219 sensor error
- **Bit 4**: BMP280 sensor error
- **Bit 5**: RS-485 timeout
- **Bit 6**: MQTT disconnected (set by ESP32)
- **Bit 7**: Watchdog reset occurred

**Example**: Value 0x05 (binary: 00000101)
- Bit 0 = 1: Over-current active
- Bit 1 = 1: Temperature warning active
- Bits 2-7 = 0: All other conditions normal

---

## Motor Control Logic (Pseudocode)

```pseudocode
FUNCTION motor_control_loop():
    LOOP forever:
        // 1. Read sensors
        lm35_temp = read_lm35_with_averaging(16)
        ina219_current = read_ina219()
        ina219_voltage = read_ina219_voltage()
        
        // 2. Check for sensor errors
        IF (lm35_temp == INVALID OR ina219_current == INVALID) THEN
            SET alarm_flag |= ALARM_SENSOR_ERROR
            CONTINUE
        END IF
        
        // 3. Check protection thresholds
        IF (ina219_current > current_threshold) THEN
            SET alarm_flag |= ALARM_OVER_CURRENT
            motor_state = FAULT
            motor_pwm = 0%
            ACTIVATE_BUZZER()
            CONTINUE
        END IF
        
        IF (lm35_temp > temp_fault_threshold) THEN
            SET alarm_flag |= ALARM_OVER_TEMP
            motor_state = FAULT
            motor_pwm = 0%
            ACTIVATE_BUZZER()
            CONTINUE
        END IF
        
        // 4. Handle soft limit (warning)
        IF (lm35_temp > temp_warning_threshold AND motor_state != WARNING) THEN
            SET alarm_flag |= ALARM_TEMP_WARNING
            motor_state = WARNING
            // Reduce PWM by 20%
            motor_pwm = MAX(0, motor_pwm - 20)
        END IF
        
        // 5. Apply control mode
        IF (control_mode == MANUAL) THEN
            motor_pwm = requested_pwm_from_esp32
        ELSE IF (control_mode == AUTO) THEN
            // Apply temperature-based PWM mapping
            IF (lm35_temp < 35) THEN
                motor_pwm = 0%
                motor_state = STOP
            ELSE IF (lm35_temp < 45) THEN
                motor_pwm = 40%
                motor_state = RUNNING
            ELSE IF (lm35_temp < 55) THEN
                motor_pwm = 70%
                motor_state = RUNNING
            ELSE IF (lm35_temp < 65) THEN
                motor_pwm = 100%
                motor_state = RUNNING
            ELSE
                motor_pwm = 0%
                motor_state = FAULT
            END IF
        END IF
        
        // 6. Apply PWM to motor
        SET_PWM_DUTY(motor_pwm)
        
        // 7. Update Modbus registers
        modbus_register[40006] = motor_pwm
        modbus_register[40007] = motor_state
        modbus_register[40009] = alarm_flags
        
        // 8. Feed watchdog
        FEED_WATCHDOG()
        
        // 9. Wait for next cycle (10ms)
        SLEEP(10ms)
    END LOOP
END FUNCTION
```

---

## Recommended Thresholds

| Parameter | Recommended | Min | Max | Unit |
|-----------|-------------|-----|-----|------|
| Current Threshold | 1500 | 500 | 3200 | mA |
| Temp Warning | 55 | 45 | 60 | °C |
| Temp Fault | 65 | 60 | 80 | °C |
| PWM Warning Reduction | 20 | 10 | 50 | % |
| Motor Idle Timeout | 300 | 60 | 3600 | sec |

---

**Last Updated**: May 2026  
**Version**: 1.0
