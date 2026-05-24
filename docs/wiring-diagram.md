# System Wiring Diagram

This document outlines the complete hardware wiring for the Industrial DC Motor HMI Gateway project. It covers the STM32 field node, the ESP32 HMI Gateway, and the interconnections.

## 1. STM32F411CEU6 Black Pill (Field Node)

### Sensor Connections
| Component | STM32 Pin | Power | Notes |
|-----------|-----------|-------|-------|
| **LM35** (Temp) | PA0 (ADC1_IN0) | 5V | Output is 10mV/°C. No pull-ups needed. |
| **INA219** (Power) | PB6(SCL), PB7(SDA) | 3.3V | I2C Addr: 0x40 (SDO=GND). Use 4.7kΩ pull-ups if module lacks them. |
| **BMP280** (Env) | PB6(SCL), PB7(SDA) | 3.3V | I2C Addr: 0x76 (SDO=GND). Shares I2C bus with INA219. |

### Actuators & Indicators
| Component | STM32 Pin | Power | Notes |
|-----------|-----------|-------|-------|
| **Motor PWM** | PA8 (TIM1_CH1) | - | Connects to MOSFET Gate. Use 100Ω series resistor + 10kΩ pull-down. |
| **Buzzer** | PB12 | 5V / 3.3V | Active HIGH. Use series resistor (e.g., 100Ω) or transistor driver. |
| **User LED** | PC13 | Onboard | Active LOW (onboard). |

### Communications
| Interface | TX/SCL | RX/SDA | DE/RE (Dir) | Notes |
|-----------|--------|--------|-------------|-------|
| **Debug UART1** | PA9 | PA10 | N/A | 115200 baud. Connect to USB-TTL adapter. |
| **Modbus UART2**| PA2 | PA3 | PA4 | 9600 baud. Connect to TTL-RS485 module. |

---

## 2. ESP32-WROOM-32 (HMI Gateway)

### TFT Display (ILI9488 SPI)
| ESP32 Pin | TFT Pin | Function | Notes |
|-----------|---------|----------|-------|
| GPIO 23 | MOSI | SPI MOSI | |
| GPIO 19 | MISO | SPI MISO | (Optional, usually not needed for write-only displays) |
| GPIO 18 | SCK | SPI Clock | Use series resistor (e.g. 10Ω) if encountering glitches. |
| GPIO 5 | CS | Chip Select | |
| GPIO 4 | DC/RS | Data/Command | |
| GPIO 2 | RST | Reset | |
| 3.3V | VCC & BL | Power & Backlight | Ensure sufficient 3.3V power (external regulator recommended). |

### Communications
| Interface | TX Pin | RX Pin | DE/RE (Dir) | Notes |
|-----------|--------|--------|-------------|-------|
| **Modbus UART2**| GPIO 17 | GPIO 16 | GPIO 15 | Connect to TTL-RS485 module. |

---

## 3. Sub-System Schematics

### 3.1 RS-485 Modbus Bus (STM32 <-> ESP32)
```
STM32 (Field Node)                       ESP32 (HMI Gateway)
[PA2] ----> TXD [RS485 Module 1]         [RS485 Module 2] TXD <---- [GPIO 17]
[PA3] <---- RXD [    A    B    ]---------[    A    B    ] RXD ----> [GPIO 16]
[PA4] ----> DE  [              ]         [              ] DE  <---- [GPIO 15]
            RE (tied to DE)                          RE (tied to DE)
```
*Note: Use twisted pair for A and B. Add 120Ω termination resistors across A and B at both ends for long runs.*

### 3.2 Motor Driver & Current Sensing (INA219 + MOSFET)

To measure both voltage and current, the INA219 is configured for **High-Side Sensing** (placed between the positive power supply and the motor).

```
12V Main Supply (+) -------> [VIN+] INA219 Module [VIN-] -------> Motor(+)
                                                                      |
                                                                    [Diode] (Cathode to +, Anode to -)
                                                                      |
STM32 (PA8) --[100Ω]--+--> Gate [N-CH MOSFET, e.g. IRF520/540]        |
                      |                                               |
                   [10kΩ]                                             |
                      |                                               |
                     GND                                              |
                          Source --------> GND (Common)               |
                          Drain  ---------------------------------> Motor(-)
```
**Notes on INA219:**
- The INA219 measures the voltage drop across its internal shunt resistor (between `VIN+` and `VIN-`) to calculate **current**.
- It also measures the voltage at `VIN-` relative to GND to calculate **bus voltage**.
- The Flyback Diode is strictly required across the motor terminals to protect the MOSFET and INA219 from inductive spikes.

### 3.3 Power Distribution Network
```
12V Main Supply
 ├──> Motor(+)
 └──> LM7805 / Buck Converter (5V)
       ├──> STM32 Black Pill (5V pin)
       ├──> ESP32 Dev Board (5V/VIN pin)
       ├──> LM35 Sensor (VCC)
       └──> LDO / Buck Converter (3.3V)
             ├──> INA219 (VCC)
             ├──> BMP280 (VCC)
             ├──> TFT Display (VCC)
             └──> RS-485 Modules (VCC - check module spec)
```
**CRITICAL**: All GND lines (12V, 5V, 3.3V, Motor, Sensors, Logic) MUST be connected together (Common Ground).