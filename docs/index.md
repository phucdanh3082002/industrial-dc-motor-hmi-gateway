# Documentation Index

Complete technical documentation for Industrial DC Motor HMI Gateway project.

---

## Quick Navigation

### Core Documentation

1. **[Architecture](./architecture.md)** (9 KB)
   - System overview and 3-tier architecture
   - Component roles and responsibilities
   - Communication protocols (RS-485, MQTT, I2C, SPI, ADC)
   - Power architecture and reliability features
   - Performance characteristics and security considerations

2. **[Modbus Register Map](./modbus-register-map.md)** (13 KB)
   - Complete telemetry register definitions (40001-40013)
   - Command register specifications (40100-40108)
   - Register access patterns and error handling
   - Performance notes and optimizations

3. **[MQTT Topics & Payloads](./mqtt-topics.md)** (12 KB)
   - Topic structure and naming conventions
   - Published topics (ESP32 → Raspberry Pi)
   - Subscribed topics (Raspberry Pi → ESP32)
   - Response topics and error handling
   - QoS levels and reliability strategies
   - Bandwidth estimation and scaling

4. **[Motor Control Design](./motor-control-design.md)** (15 KB)
   - Motor state machine (STOP, RUNNING, WARNING, FAULT)
   - Manual and Auto control modes
   - PWM control and motor protection mechanisms
   - Over-current and over-temperature protection
   - Alarm flags and fault codes
   - Safety interlocks and recovery procedures

5. **[Database Schema](./database-schema.md)** (14 KB)
   - SQLite table definitions (telemetry, alarm_log, command_log, training_log, ai_result)
   - Data relationships and dependencies
   - Query examples and performance optimization
   - Backup and maintenance strategies
   - Data privacy and compliance

6. **[TinyML Roadmap](./tinyml-roadmap.md)** (18 KB)
   - Machine learning strategy for anomaly detection
   - Data collection and labeling methodology
   - Model selection and training pipeline
   - ESP32 inference engine implementation
   - Model evaluation and deployment
   - Future enhancements and retraining strategy

---

## Document Purpose Guide

### For Firmware Engineers (STM32/ESP32)

**Start Here**:
1. [Architecture](./architecture.md) - Understand system overview
2. [Modbus Register Map](./modbus-register-map.md) - Learn register protocol
3. [Motor Control Design](./motor-control-design.md) - Implement state machine

**Reference**:
- [MQTT Topics & Payloads](./mqtt-topics.md) - For ESP32 MQTT integration
- [TinyML Roadmap](./tinyml-roadmap.md) - For Phase 8 TinyML task

### For Backend Engineers (Raspberry Pi/Python)

**Start Here**:
1. [Architecture](./architecture.md) - System overview
2. [MQTT Topics & Payloads](./mqtt-topics.md) - Understand data flow
3. [Database Schema](./database-schema.md) - Design persistence layer

**Reference**:
- [Modbus Register Map](./modbus-register-map.md) - For REST API implementation
- [Motor Control Design](./motor-control-design.md) - Understand motor states

### For ML Engineers (TinyML Development)

**Start Here**:
1. [TinyML Roadmap](./tinyml-roadmap.md) - Model strategy and implementation
2. [Database Schema](./database-schema.md) - Training data source
3. [Motor Control Design](./motor-control-design.md) - Understand anomalies

**Reference**:
- [Architecture](./architecture.md) - System constraints
- [MQTT Topics & Payloads](./mqtt-topics.md) - For AI result publishing

### For System Integrators/Testers

**Start Here**:
1. [Architecture](./architecture.md) - Full system overview
2. [Motor Control Design](./motor-control-design.md) - Safety procedures
3. [MQTT Topics & Payloads](./mqtt-topics.md) - Data formats

**Reference**:
- [Modbus Register Map](./modbus-register-map.md) - For commissioning
- [Database Schema](./database-schema.md) - For data validation

---

## Development Phase Reference

### Phase 1-2: STM32 Hardware & Motor Control
- Read: [Motor Control Design](./motor-control-design.md)
- Reference: [Modbus Register Map](./modbus-register-map.md)

### Phase 3: Modbus Communication
- Read: [Modbus Register Map](./modbus-register-map.md)
- Reference: [Architecture](./architecture.md) - Communication Protocols section

### Phase 4: ESP32 HMI
- Read: [Architecture](./architecture.md)
- Reference: [MQTT Topics & Payloads](./mqtt-topics.md)

### Phase 5: MQTT & Logging
- Read: [MQTT Topics & Payloads](./mqtt-topics.md), [Database Schema](./database-schema.md)
- Reference: [Architecture](./architecture.md)

### Phase 6: Web Dashboard & API
- Read: [Database Schema](./database-schema.md), [MQTT Topics & Payloads](./mqtt-topics.md)
- Reference: [Motor Control Design](./motor-control-design.md) for motor states

### Phase 7: System Stabilization
- Read: [Architecture](./architecture.md) - Reliability Features
- Reference: All documents for validation checklist

### Phase 8: TinyML
- Read: [TinyML Roadmap](./tinyml-roadmap.md)
- Reference: [Database Schema](./database-schema.md) for training_log table

---

## Key Design Decisions

### 1. Communication Protocol Choice
- **RS-485 Modbus RTU**: Industrial-grade, noise-immune, proven reliability
- **Wi-Fi MQTT**: Flexible, easy integration, IoT-friendly
- **See**: [Architecture](./architecture.md) - Communication Protocols

### 2. Motor State Machine
- **4-state design** (STOP, RUNNING, WARNING, FAULT) ensures safety
- **Hysteresis prevents oscillation** between states
- **See**: [Motor Control Design](./motor-control-design.md)

### 3. Protection Strategy
- **Hardware-first**: STM32 handles real-time protection
- **Redundant checks**: Both threshold and current monitoring
- **See**: [Motor Control Design](./motor-control-design.md) - Protection Mechanisms

### 4. Data Architecture
- **Separate tables** for different data types (telemetry, alarms, commands, training)
- **Training log with labels** designed for supervised learning
- **See**: [Database Schema](./database-schema.md)

### 5. TinyML Deployment
- **Phase 8 (last)**: Ensures stable foundation before ML complexity
- **On-device inference**: Real-time, no cloud dependency
- **See**: [TinyML Roadmap](./tinyml-roadmap.md)

---

## Common Questions

### Q: How do I start implementing the motor control?
A: Start with [Motor Control Design](./motor-control-design.md). It includes pseudocode and state diagrams.

### Q: What registers should I read from STM32?
A: See [Modbus Register Map](./modbus-register-map.md) - Telemetry Registers section.

### Q: How do I know if a motor fault is over-current or over-temp?
A: Check the `fault_code` field in Modbus register 40010. See [Motor Control Design](./motor-control-design.md) - Fault Codes table.

### Q: What MQTT topics should I subscribe to?
A: See [MQTT Topics & Payloads](./mqtt-topics.md) - Subscribed Topics section.

### Q: How do I set up the database?
A: Run SQL scripts from [Database Schema](./database-schema.md). Create all 5 tables in order.

### Q: When should I start implementing TinyML?
A: Only after Phase 7 is complete and you have weeks of operational data. See [TinyML Roadmap](./tinyml-roadmap.md).

### Q: What's the maximum motor current before fault?
A: Configured via Modbus register 40104. Default: 1500 mA. See [Modbus Register Map](./modbus-register-map.md).

---

## Document Maintenance

**Last Updated**: May 2026  
**Version**: 1.0

**Update Schedule**:
- Review quarterly for accuracy
- Update with implementation changes
- Add troubleshooting section after first deployment
- Incorporate lessons learned from Phase 7 system testing

**Contributing**:
- Submit issues via GitHub Issues
- Propose changes via Pull Requests
- Maintain consistent formatting and structure
- Keep pseudocode up-to-date with actual implementation

---

## File Sizes & Statistics

| Document | Size | Lines | Topics |
|----------|------|-------|--------|
| architecture.md | 9.1 KB | 285 | 3-tier architecture, protocols, power, reliability |
| modbus-register-map.md | 13 KB | 420 | 13 telemetry + 9 command registers |
| mqtt-topics.md | 12 KB | 385 | 6 published topics + responses |
| motor-control-design.md | 15 KB | 485 | State machine, modes, protection |
| database-schema.md | 14 KB | 430 | 5 tables + queries + maintenance |
| tinyml-roadmap.md | 18 KB | 580 | Data collection, training, deployment |
| **TOTAL** | **81 KB** | **2,585** | **Complete system specification** |

---

## Related Files

- **README.md**: Quick project overview and getting started
- **AGENTS.md**: OpenCode agent guidance for project understanding
- **docs/index.md** (this file): Navigation and reference guide

---

## Support & Questions

For questions about specific topics:
- **Architecture**: See [Architecture](./architecture.md)
- **Implementation**: See relevant phase documentation
- **Troubleshooting**: See [Motor Control Design](./motor-control-design.md) - Protection Mechanisms
- **Machine Learning**: See [TinyML Roadmap](./tinyml-roadmap.md)

For urgent issues, contact project lead.

---

**Last Built**: May 21, 2026  
**Docs Version**: 1.0  
**Project Status**: Design Complete - Ready for Phase 1 Implementation
