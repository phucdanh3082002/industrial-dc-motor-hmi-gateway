# Phase 2: Motor Control & Fault Protection + FreeRTOS - Tiến Độ

**Thời gian dự kiến**: Tuần 2-3  
**Trạng thái**: HOÀN THÀNH  
**Bắt đầu**: 2026-05-24  
**Cập nhật**: 2026-06-01

---

## Mục Tiêu Phase 2

Triển khai **Motor State Machine** (FSM) với bảo vệ quá nhiệt, quá dòng, fault recovery, watchdog, và **tích hợp FreeRTOS** cho real-time scheduling trên STM32F411.

---

## Kết Quả Cuối Cùng

| Hạng Mục | Trạng Thái | Ghi Chú |
|-----------|-----------|---------|
| Motor FSM 4 states | ✅ HOÀN THÀNH | STOP → RUNNING → WARNING → FAULT |
| Over-temperature protection | ✅ | Alarm 45°C, Fault 65°C, Hysteresis 5°C |
| Over-current protection | ✅ | Alarm 1A, Fault 2A |
| Fault Manager (escalation) | ✅ | 3 cấp: info → warning → critical |
| IWDG Watchdog | ✅ | 2s timeout, feed bởi watchdog_task |
| FreeRTOS Integration | ✅ | 4 tasks, queues, event group heartbeat |
| UART Command Parser | ✅ | S/X/0-9/A/M/R/T/Y |
| Build | ✅ | 58KB FLASH (11%), 36KB RAM (27%) |

---

## FreeRTOS Task Architecture

```
┌─────────────────────────────────────────────────────────────┐
│  Priority 5: WATCHDOG_TASK (1s)                             │
│  - Feed IWDG hardware watchdog                              │
│  - Kiểm tra heartbeat từ sensor_task + motor_task           │
│  - Nếu task frozen → force reset                           │
├─────────────────────────────────────────────────────────────┤
│  Priority 4: MOTOR_TASK (100ms)                             │
│  - Nhận sensor data từ Queue (non-blocking peek)           │
│  - Nhận command từ Queue (non-blocking receive)            │
│  - Chạy FSM: state transitions + PWM apply                │
│  - Debug print mỗi 1s                                       │
├─────────────────────────────────────────────────────────────┤
│  Priority 3: SENSOR_TASK (50ms)                             │
│  - Đọc LM35 (ADC, 16 samples average)                     │
│  - Đọc INA219 voltage + current (I2C)                      │
│  - Gửi sensor_msg_t qua Queue → motor_task                 │
│  - Heartbeat event                                          │
├─────────────────────────────────────────────────────────────┤
│  Priority 2: UART_TASK (event-driven)                       │
│  - HAL_UART_Receive blocking (100ms timeout)               │
│  - Parse command bytes: S/X/0-9/A/M/R/T/Y                 │
│  - Gửi cmd_msg_t qua Queue → motor_task                    │
└─────────────────────────────────────────────────────────────┘
```

### Inter-Task Communication

```
SENSOR_TASK ──Queue──> MOTOR_TASK
                       (sensor_msg_t: temp, voltage, current, errors)

UART_TASK ───Queue──> MOTOR_TASK
                       (cmd_msg_t: enable, pwm, mode, reset_fault)

ALL TASKS ──EventGroup──> WATCHDOG_TASK
                           (BIT_SENSOR_ALIVE, BIT_MOTOR_ALIVE, BIT_UART_ALIVE)
```

---

## Files Đã Tạo/Sửa

### Files Mới (Phase 2)

| File | Mô Tả | Dòng |
|------|-------|------|
| `src/motor/motor_control.h` | Motor FSM header | 116 |
| `src/motor/motor_control.c` | FSM + Auto mode + Protection | 397 |
| `src/fault/fault_manager.h` | Fault manager header | 100 |
| `src/fault/fault_manager.c` | Escalation + auto-recovery | 190 |

### Files Mới (FreeRTOS Integration)

| File | Mô Tả |
|------|-------|
| `Inc/FreeRTOSConfig.h` | FreeRTOS config cho STM32F411 @ 84MHz |
| `Middlewares/FreeRTOS/Kernel/` | FreeRTOS kernel (clone từ GitHub) |
| `src/freertos/freertos_tasks.c` | 4 tasks: sensor, motor, uart, watchdog |
| `src/freertos/freertos_tasks.h` | API: freertos_objects_init(), freertos_tasks_init() |

### Files Đã Sửa

| File | Thay Đổi |
|------|----------|
| `src/main.c` | Refactor: super loop → FreeRTOS startup + hooks |
| `src/config/config.h` | Thêm motor thresholds, auto PWM, Modbus ID, IWDG config |
| `CMakeLists.txt` | Thêm FreeRTOS kernel + tasks sources + include paths |
| `cmake/stm32cubemx/CMakeLists.txt` | Thêm stm32f4xx_hal_iwdg.c |
| `Inc/stm32f4xx_hal_conf.h` | Bật `HAL_IWDG_MODULE_ENABLED` |
| `src/stm32f4xx_it.c` | Xóa SVC_Handler, PendSV_Handler, SysTick_Handler (trùng FreeRTOS) |

---

## Bugs Đã Tìm & Sửa (10 bugs)

| # | Bug | Nguyên nhân | Sửa |
|---|-----|-------------|-----|
| 1 | INA219 chưa init | Phase 1 test bị xóa | Thêm `ina219_test_init()` |
| 2 | Sensor error chặn motor start | Quá nghiêm ngặt | Chỉ block khi cả 2 sensor fail |
| 3 | UART flood 100ms | Debug print quá nhanh | Giảm xuống 1s/lần |
| 4 | INA219 đọc liên tục | Không có delay | Thêm delay 5ms |
| 5 | fault_manager_update không gọi | Auto-recovery không chạy | Thêm vào main loop |
| 6 | **test_get_ms() luôn = 0** | SysTick_Handler không gọi HAL_SYSTICK_Callback | Dùng HAL_GetTick() |
| 7 | WARNING state PWM = 0 | u8_current_pwm=0 × 50% = 0 | Dùng command setpoint × 50% |
| 8 | PWM compare không cập nhật | motor_apply_pwm dùng giá trị cũ | Section 5 tính từ command |
| 9 | **SVC/PendSV/SysTick trùng** | FreeRTOS port.c vs stm32f4xx_it.c | Xóa handler khỏi stm32f4xx_it.c |
| 10 | **IWDG HAL không có** | File stm32f4xx_hal_iwdg.c chưa tải | Tải từ GitHub STM32CubeF4 |

---

## Cấu Trúc Lớp (Layered Architecture)

```
┌─────────────────────────────────────────────────────┐
│  APPLICATION LAYER                                   │
│  main.c → freertos_tasks_init() → vTaskStartScheduler│
├─────────────────────────────────────────────────────┤
│  RTOS TASK LAYER                                     │
│  sensor_task → motor_task → uart_task → watchdog_task│
├─────────────────────────────────────────────────────┤
│  MOTOR CONTROL LAYER                                 │
│  motor_control.c (FSM) + fault_manager.c            │
├─────────────────────────────────────────────────────┤
│  SENSOR/DRIVER LAYER                                 │
│  lm35_test.c, ina219_test.c, adc_driver, i2c_driver │
├─────────────────────────────────────────────────────┤
│  HAL / BOARD SUPPORT                                 │
│  STM32 HAL (GPIO, ADC, I2C, TIM1, USART, IWDG)     │
└─────────────────────────────────────────────────────┘
```

---

## FreeRTOS Configuration

| Parameter | Giá trị |
|-----------|---------|
| Config file | `Inc/FreeRTOSConfig.h` |
| CPU Clock | 84 MHz (HSE + PLL) |
| Tick Rate | 1 kHz (1ms) |
| Heap | 32 KB (heap_4.c) |
| Preemptive | Yes |
| Stack Overflow Check | Level 2 |
| Malloc Failed Hook | Yes |
| Max Priorities | 7 |

### Task Stack Sizes

| Task | Stack | Priority |
|------|-------|----------|
| SENSOR_TASK | 512 words (2KB) | 3 |
| MOTOR_TASK | 512 words (2KB) | 4 |
| UART_TASK | 256 words (1KB) | 2 |
| WATCHDOG_TASK | 256 words (1KB) | 5 |

---

## Protection Thresholds

| Thông Số | Giá Trị | Đơn Vị |
|----------|---------|--------|
| Temp Warning | 45.0°C | Alarm (PWM giảm 50%) |
| Temp Fault | 65.0°C | Fault (motor dừng, buzzer ON) |
| Hysteresis | 5.0°C | Clear alarm khi temp < 40°C |
| Current Alarm | 1000mA | Warning |
| Current Fault | 2000mA | Fault |

---

## Auto Mode - Temperature to PWM

| Nhiệt Độ | PWM | Mô Tả |
|-----------|-----|-------|
| < 35°C | 0% | Motor tắt |
| 35-45°C | 40% | Tốc độ thấp |
| 45-55°C | 70% | Tốc độ trung bình |
| 55-65°C | 100% | Tốc độ tối đa |
| > 65°C | 0% | FAULT |

---

## UART Command Interface

| Lệnh | Ý Nghĩa |
|-------|---------|
| `S` | Start motor (50% default) |
| `X` | Stop motor |
| `0`-`9` | Set PWM (100%, 10%-90%) |
| `A` | Auto mode |
| `M` | Manual mode |
| `R` | Reset fault |
| `T` | Test PWM trực tiếp (bypass FSM) |
| `Y` | Stop test PWM |

---

## Kế Hoạch Test

1. ✅ Build thành công (58KB FLASH, 36KB RAM)
2. 🔜 Flash xuống STM32 + verify 4 tasks chạy
3. 🔜 Test motor control: S → chạy, X → dừng
4. 🔜 Test fault recovery: quá nhiệt → WARNING → FAULT → reset
5. 🔜 Test watchdog: freeze motor_task → auto reset

---

## Tiếp Theo (Phase 3: Modbus RTU Slave)

- Khởi tạo UART2 cho RS-485 (9600 baud)
- Implement Modbus RTU slave protocol (CRC16, function codes 0x03/0x06)
- Register map 40001-40013 (telemetry) + 40100-40108 (commands)
- Tạo `modbus_task` trong FreeRTOS

---

**Cập nhật lần cuối**: 2026-06-01  
**Version**: 2.0 (FreeRTOS Integration)
