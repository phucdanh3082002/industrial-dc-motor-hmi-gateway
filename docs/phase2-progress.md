# Phase 2: Motor Control & Fault Protection - Tiến Độ

**Thời gian dự kiến**: Tuần 2-3  
**Trạng thái**: ĐANG THỰC HIỆN  
**Bắt đầu**: 2026-05-24

---

## Mục Tiêu Phase 2

Triển khai **Motor State Machine** (FSM) với bảo vệ quá nhiệt, quá dòng, fault recovery, và watchdog trên STM32F411. Chạy bare-metal (super loop, chưa dùng FreeRTOS).

---

## Các Bước Cần Thực Hiện

| Step | Nội dung | File | Trạng thái |
|------|----------|------|------------|
| 8 | Định nghĩa Motor State Machine | `motor_control.h` | 🔜 |
| 9 | Implement state transitions + guarding conditions | `motor_control.c` | 🔜 |
| 10 | Over-temperature detection (alarm 45°C, fault 65°C) | `motor_control.c` | 🔜 |
| 11 | Over-current detection (alarm 1.0A, fault 2.0A) | `motor_control.c` | 🔜 |
| 12 | Fault Manager với escalation rules | `fault_manager.h/.c` | 🔜 |
| 13 | IWDG Watchdog (2s timeout, feed mỗi 1s) | `main.c` | 🔜 |
| 14 | Test state transitions + fault recovery | `main.c` | 🔜 |

---

## Thiết Kế Motor State Machine

### Trạng Thái (4 states)

```
MOTOR_STATE_STOP    = 0    // Motor dừng, PWM = 0%
MOTOR_STATE_RUNNING = 1    // Motor chạy bình thường
MOTOR_STATE_WARNING = 2    // Soft limit (nhiệt độ cảnh báo), giảm PWM
MOTOR_STATE_FAULT   = 3    // Lỗi nghiêm trọng, motor dừng, buzzer ON
```

### Chế Độ Điều Khiển

```
MOTOR_MODE_MANUAL = 0     // User set PWM trực tiếp (0-100%)
MOTOR_MODE_AUTO   = 1     // Tự động PWM theo nhiệt độ LM35
```

### State Transition Diagram

```
                    ┌──────────┐
          enable=1  │          │  temp>65°C hoặc current>2A
        ┌──────────>│ RUNNING  │─────────────────────┐
        │           │          │                      │
   ┌────┴───┐       └────┬─────┘              ┌──────┴──────┐
   │  STOP  │             │ temp>45°C          │    FAULT    │
   │ (PWM=0)│<──────enable=0│            │    │  (PWM=0)   │
   └────────┘             │            │    │  Buzzer ON  │
        ▲                 ▼            │    │  LED RED    │
        │           ┌──────────┐       │    └──────┬──────┘
        │enable=0   │ WARNING  │       │           │
        └───────────│(PWM=50%) │───────┘    reset_fault=1
                    │ Buzzer 1 │ fault trigger
                    └──────────┘
                         │ temp<40°C
                         └──────> RUNNING
```

### Bảng Transition Chi Tiết

| Từ | Đến | Điều Khiện | Hành Động |
|----|-----|------------|-----------|
| STOP | RUNNING | enable=1 && pwm>0 | Set PWM = target |
| RUNNING | WARNING | temp > 45°C | Giảm PWM 50%, buzzer beep 1 lần |
| RUNNING | FAULT | temp > 65°C hoặc current > 2A | PWM = 0%, buzzer ON liên tục |
| RUNNING | STOP | enable = 0 | PWM = 0% |
| WARNING | RUNNING | temp < 40°C (hysteresis 5°C) | Restore PWM |
| WARNING | FAULT | temp > 65°C hoặc current > 2A | PWM = 0%, buzzer ON |
| WARNING | STOP | enable = 0 | PWM = 0% |
| FAULT | STOP | reset_fault = 1 (Modbus 40103) | Clear fault flags |

---

## Auto Mode - Temperature to PWM Mapping

| Nhiệt Độ | PWM | Trạng Thái | Mô Tả |
|-----------|-----|------------|-------|
| < 35°C | 0% | STOP | Motor tắt, làm mát |
| 35-45°C | 40% | RUNNING | Tốc độ thấp |
| 45-55°C | 70% | RUNNING | Tốc độ trung bình |
| 55-65°C | 100% | RUNNING | Tốc độ tối đa |
| > 65°C | 0% | FAULT | Quá nhiệt, motor dừng |

---

## Protection Thresholds

| Thông Số | Giá Trị Mặc Định | Min | Max | Đơn Vị |
|----------|-------------------|-----|-----|--------|
| Current Alarm | 1000 | 500 | 3200 | mA |
| Current Fault | 2000 | 1000 | 3200 | mA |
| Temp Warning | 45 | 35 | 60 | °C |
| Temp Fault | 65 | 55 | 85 | °C |
| Hysteresis | 5 | 2 | 10 | °C |

---

## Fault Manager - Escalation Rules

| Lần Xuất Hiện | Hành Động |
|----------------|-----------|
| Lần 1 | Log info, tiếp tục chạy |
| Lần 2 | Log warning, giảm PWM 50% |
| Lần 3+ | Log critical, chuyển FAULT state |
| Recovery | Auto clear counter sau 30s ổn định |

---

## Fault Codes

| Code | Loại | Mô Tả |
|------|------|-------|
| 0 | NONE | Không có lỗi |
| 1 | OVERCURRENT | Quá dòng |
| 2 | OVERHEAT | Quá nhiệt |
| 3 | TEMP_WARNING | Nhiệt độ cảnh báo |
| 4 | SENSOR_LM35 | LM35 I2C/ADC error |
| 5 | SENSOR_INA219 | INA219 I2C error |
| 6 | SENSOR_BMP280 | BMP280 I2C error |
| 7 | WATCHDOG_RESET | Watchdog triggered |
| 8 | MODBUS_CRC | CRC validation failed |

---

## Alarm Flags (Bitmask - Register 40009)

```
Bit 0: Over-current
Bit 1: Temperature warning
Bit 2: Over-temperature (fault)
Bit 3: INA219 sensor error
Bit 4: BMP280 sensor error
Bit 5: RS-485 timeout
Bit 6: MQTT disconnected
Bit 7: Watchdog reset
```

---

## Files Cần Tạo/Sửa

### Files Mới

| File | Mô Tả |
|------|-------|
| `src/motor/motor_control.h` | Motor FSM header (states, commands, API) |
| `src/motor/motor_control.c` | Motor FSM implementation |
| `src/fault/fault_manager.h` | Fault manager header |
| `src/fault/fault_manager.c` | Fault manager implementation |

### Files Cần Sửa

| File | Thay Đổi |
|------|----------|
| `src/config/config.h` | Thêm motor thresholds, fault codes |
| `src/main.c` | Tích hợp motor control vào super loop |
| `stm32f411-firmware/CMakeLists.txt` | Thêm motor_control.c, fault_manager.c |

---

## Integration với Existing Code

### Cấu Trúc Lớp (Layered Architecture)

```
┌─────────────────────────────────────┐
│   APPLICATION LAYER                 │
│   main.c (super loop)              │
├─────────────────────────────────────┤
│   MOTOR CONTROL LAYER               │
│   motor_control.c (FSM)            │
│   fault_manager.c (escalation)     │
├─────────────────────────────────────┤
│   SENSOR/DRIVER LAYER               │
│   lm35_test.c, ina219_test.c       │
│   adc_driver.c, i2c_driver.c       │
├─────────────────────────────────────┤
│   HAL / BOARD SUPPORT               │
│   STM32 HAL (GPIO, ADC, I2C, PWM)  │
└─────────────────────────────────────┘
```

### Gọi Từ Layer Thấp Hơn

```
main.c → motor_control.c → lm35_test.c → adc_driver.c → HAL_ADC
                       → ina219_test.c → i2c_driver.c → HAL_I2C
                       → gpio_driver.c → HAL_GPIO
fault_manager.c → motor_control.c (set state)
               → gpio_driver.c (LED, buzzer)
```

---

## Watchdog Strategy

- **IWDG Timeout**: 2s
- **Feed Interval**: Mỗi 1s từ main loop
- **Reset Detection**: Kiểm tra RCC_FLAG_IWDGRST trên startup
- **Health Check**: Nếu motor task không feed watchdog → force reset

---

## Kế Hoạch Test

1. **Unit Test**: Motor state transitions (STOP→RUNNING→WARNING→FAULT→STOP)
2. **Integration Test**: Đọc LM35/INA219 → Motor control → PWM output
3. **Fault Test**: Giả lập quá nhiệt (temp > 65°C) → verify FAULT state
4. **Recovery Test**: Reset fault → verify motor trở lại STOP
5. **Watchdog Test**: Verify watchdog reset khi system bị đơ

---

## Ghi Chú

- Phase 2 chạy **bare-metal** (super loop), chưa dùng FreeRTOS
- FreeRTOS sẽ được tích hợp ở Phase 3 (Modbus RTU)
- BMP280暫時 bỏ qua (chưa kết nối)
- Motor PWM dùng TIM1_CH1 (PA8), 20kHz

---

**Cập nhật lần cuối**: 2026-05-24  
**Version**: 1.0
