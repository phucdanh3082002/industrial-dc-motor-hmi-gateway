# Database Schema

SQLite database schema for Raspberry Pi server logging and data persistence.

---

## Overview

- **Database**: SQLite3
- **Location**: `/path/to/data/motor_system.db`
- **Tables**: 5 main tables (telemetry, alarm_log, command_log, training_log, ai_result)
- **Primary Key**: Auto-increment integer ID
- **Timestamps**: ISO8601 UTC format

---

## Table Definitions

### 1. Table: `telemetry`

Stores periodic sensor and motor telemetry data.

```sql
CREATE TABLE telemetry (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp TEXT NOT NULL,                -- ISO8601 UTC: "2026-05-21T10:30:45Z"
    node_id TEXT NOT NULL,                  -- Device identifier: "motor01"
    lm35_temp REAL,                         -- Motor/MOSFET temperature (°C)
    bmp280_temp REAL,                       -- Ambient temperature (°C)
    pressure REAL,                          -- Atmospheric pressure (hPa)
    motor_voltage REAL,                     -- Supply voltage (V)
    motor_current REAL,                     -- Motor current (mA)
    motor_pwm INTEGER,                      -- PWM duty cycle (0-100%)
    motor_state TEXT,                       -- State: STOP/RUNNING/WARNING/FAULT
    control_mode TEXT,                      -- Mode: MANUAL/AUTO
    alarm_flags INTEGER,                    -- Bitmask of active alarms
    fault_code INTEGER,                     -- Fault code (0=none)
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_telemetry_timestamp ON telemetry(timestamp);
CREATE INDEX idx_telemetry_node_id ON telemetry(node_id);
CREATE INDEX idx_telemetry_motor_state ON telemetry(motor_state);
```

**Purpose**: Historical telemetry for trend analysis, dashboards, and anomaly detection training

**Typical Retention**: 30-90 days (configurable)

**Data Volume**: ~1 row/second × 86400 sec/day = ~86,400 rows/day

---

### 2. Table: `alarm_log`

Records all alarm and fault events with detailed context.

```sql
CREATE TABLE alarm_log (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp TEXT NOT NULL,                -- When alarm triggered
    node_id TEXT NOT NULL,                  -- Device identifier
    alarm_type TEXT NOT NULL,               -- Type: OVER_CURRENT, OVER_TEMP, etc.
    alarm_level TEXT,                       -- Level: CRITICAL/WARNING/INFO
    fault_code INTEGER,                     -- Associated fault code
    description TEXT,                       -- Human-readable description
    motor_state TEXT,                       -- Motor state at time of alarm
    motor_current REAL,                     -- Current when alarm triggered (mA)
    lm35_temp REAL,                         -- Temperature when alarm triggered (°C)
    threshold_value REAL,                   -- Threshold that was exceeded
    recovery_time INTEGER,                  -- Time to recover (seconds), NULL if ongoing
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_alarm_timestamp ON alarm_log(timestamp);
CREATE INDEX idx_alarm_node_id ON alarm_log(node_id);
CREATE INDEX idx_alarm_type ON alarm_log(alarm_type);
```

**Purpose**: Audit trail for fault analysis, maintenance scheduling, and system reliability

**Typical Retention**: 1 year or longer (compliance requirement)

**Data Volume**: ~10-50 rows/day (depends on system stability)

---

### 3. Table: `command_log`

Tracks all commands issued to motors and their execution status.

```sql
CREATE TABLE command_log (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp TEXT NOT NULL,                -- When command issued
    node_id TEXT NOT NULL,                  -- Target device
    command_id TEXT,                        -- Unique command identifier
    command TEXT NOT NULL,                  -- Command type: SET_MOTOR_SPEED, etc.
    value TEXT,                             -- Command value (JSON serialized)
    source TEXT,                            -- Source: HMI/API/WEB/AUTO
    status TEXT NOT NULL,                   -- Status: SENT/ACKNOWLEDGED/EXECUTED/FAILED
    execution_time_ms INTEGER,              -- Time to execute (milliseconds)
    error_message TEXT,                     -- Error description if failed
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_command_timestamp ON command_log(timestamp);
CREATE INDEX idx_command_node_id ON command_log(node_id);
CREATE INDEX idx_command_status ON command_log(status);
```

**Purpose**: Command audit, debugging communication issues, system behavior analysis

**Typical Retention**: 30 days (rolling)

**Data Volume**: ~100-500 rows/day

---

### 4. Table: `training_log`

Collects labeled data for TinyML model training and evaluation.

**CRITICAL**: This table should log EVERY sensor reading with a human-assigned label for supervised learning.

```sql
CREATE TABLE training_log (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp TEXT NOT NULL,                -- Exact time of reading
    node_id TEXT NOT NULL,                  -- Device identifier
    motor_pwm INTEGER,                      -- PWM at time of reading (0-100%)
    motor_voltage REAL,                     -- Voltage (V)
    motor_current REAL,                     -- Current (mA)
    lm35_temp REAL,                         -- Motor temperature (°C)
    bmp280_temp REAL,                       -- Ambient temperature (°C)
    pressure REAL,                          -- Pressure (hPa)
    motor_state TEXT,                       -- Motor state
    control_mode TEXT,                      -- Control mode
    alarm_flags INTEGER,                    -- Active alarms (bitmask)
    fault_code INTEGER,                     -- Fault code
    label TEXT NOT NULL,                    -- Ground truth label for training
    confidence REAL,                        -- Confidence of label (0.0-1.0)
    annotator TEXT,                         -- Who labeled this sample
    notes TEXT,                             -- Additional notes
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_training_timestamp ON training_log(timestamp);
CREATE INDEX idx_training_label ON training_log(label);
CREATE INDEX idx_training_node_id ON training_log(node_id);
```

**Purpose**: Dataset for training TinyML anomaly detection models

**Label Values** (ground truth):
- `NORMAL` - Normal operation, no issues
- `HEAVY_LOAD` - Motor under heavy load (expected high current)
- `OVERHEAT_RISK` - Temperature elevated, risk of overheat
- `STALL_RISK` - Motor possibly stalled (high current, low speed)
- `SENSOR_ERROR` - Invalid sensor reading
- `OVER_CURRENT` - Over-current event
- `OVER_TEMP` - Over-temperature event

**Data Collection Strategy**:
- Log every N seconds (e.g., every 5-10 seconds initially)
- Intensify logging during known fault scenarios
- Manual labeling required during testing phases
- Target: 10,000-50,000 labeled samples for model training

**Typical Retention**: Keep indefinitely (training data archive)

**Data Volume**: ~8,640 rows/day (1 sample per 10 seconds)

---

### 5. Table: `ai_result`

Stores TinyML model inference results (Phase 8).

```sql
CREATE TABLE ai_result (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp TEXT NOT NULL,                -- When inference executed
    node_id TEXT NOT NULL,                  -- Device identifier
    ai_status TEXT NOT NULL,                -- Status: NORMAL/WARNING/ANOMALY
    anomaly_score REAL NOT NULL,            -- Score 0.0-1.0 (higher = more anomalous)
    predicted_state TEXT,                   -- Predicted condition
    model_version TEXT,                     -- Model version used for inference
    model_accuracy REAL,                    -- Model accuracy on validation set
    inference_time_ms INTEGER,              -- Inference execution time (ms)
    feature_values TEXT,                    -- Feature vector (JSON)
    explanation TEXT,                       -- Human-readable explanation
    ground_truth_label TEXT,                -- Actual label (for post-analysis)
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_ai_result_timestamp ON ai_result(timestamp);
CREATE INDEX idx_ai_result_status ON ai_result(ai_status);
CREATE INDEX idx_ai_result_node_id ON ai_result(node_id);
```

**Purpose**: Log AI inference results, enable model evaluation and retraining

**AI Status Values**:
| Status | Anomaly Score | Meaning |
|--------|---------------|---------|
| NORMAL | 0.0 - 0.3 | No anomaly detected |
| WARNING | 0.3 - 0.7 | Potential issue, monitor closely |
| ANOMALY | 0.7 - 1.0 | Significant anomaly, take action |

**Typical Retention**: 90 days (rolling for recent inference history)

**Data Volume**: ~86,400 rows/day (1 inference/second)

---

## Data Relationships

```
┌──────────────────┐
│   telemetry      │ (Historical sensor readings)
│ (86,400 rows/day)│
└────────┬─────────┘
         │
         └─ Used by ──────────────┐
                                  │
┌──────────────────┐             ▼
│  alarm_log       │ (Events triggered from telemetry anomalies)
│ (10-50 rows/day) │
└──────────────────┘

┌──────────────────┐
│ command_log      │ (Operator/system commands)
│(100-500 rows/day)│
└──────────────────┘

┌──────────────────┐
│ training_log     │ (Labeled data for ML)
│(8,640 rows/day)  │ ◄─── Subset of telemetry with manual labels
└────────┬─────────┘
         │
         └─ Used to train ──────┐
                                 │
                                ▼
                         ┌──────────────────┐
                         │   Model          │
                         │ (TinyML v1.2)    │
                         └────────┬─────────┘
                                  │
                         Used for inference
                                  │
                                ▼
                         ┌──────────────────┐
                         │  ai_result       │ (Inference results on ESP32)
                         │(86,400 rows/day) │
                         └──────────────────┘
```

---

## Query Examples

### Get Latest Motor Status
```sql
SELECT node_id, motor_state, motor_pwm, lm35_temp, motor_current, timestamp
FROM telemetry
WHERE node_id = 'motor01'
ORDER BY timestamp DESC
LIMIT 1;
```

### Trend Analysis (Last 24 Hours)
```sql
SELECT 
    strftime('%H:00', timestamp) as hour,
    AVG(motor_current) as avg_current,
    MAX(motor_current) as max_current,
    AVG(lm35_temp) as avg_temp,
    MAX(lm35_temp) as max_temp
FROM telemetry
WHERE node_id = 'motor01'
  AND timestamp > datetime('now', '-24 hours')
GROUP BY hour
ORDER BY hour DESC;
```

### Recent Alarms
```sql
SELECT timestamp, alarm_type, alarm_level, description, recovery_time
FROM alarm_log
WHERE node_id = 'motor01'
  AND timestamp > datetime('now', '-7 days')
ORDER BY timestamp DESC
LIMIT 50;
```

### Export Training Data for ML
```sql
SELECT motor_pwm, motor_current, motor_voltage, lm35_temp, bmp280_temp, pressure, 
       motor_state, control_mode, alarm_flags, fault_code, label
FROM training_log
WHERE node_id = 'motor01'
  AND label IN ('NORMAL', 'HEAVY_LOAD', 'OVERHEAT_RISK', 'STALL_RISK')
  AND confidence >= 0.9
ORDER BY timestamp;
```

### Model Performance Evaluation
```sql
SELECT 
    ai_status,
    COUNT(*) as inference_count,
    AVG(anomaly_score) as avg_anomaly_score,
    COUNT(CASE WHEN ground_truth_label = 'NORMAL' AND ai_status = 'NORMAL' THEN 1 END) as true_negatives
FROM ai_result
WHERE timestamp > datetime('now', '-7 days')
GROUP BY ai_status;
```

---

## Database Maintenance

### Backup Strategy
- **Frequency**: Daily at 02:00 UTC
- **Retention**: 30 days of backups
- **Location**: `/backups/motor_system_YYYY-MM-DD.db.bak`

### Archival Strategy
- **Monthly**: Archive old telemetry to compressed file
- **Yearly**: Archive entire database to long-term storage
- **Purge**: Delete telemetry older than 90 days (configurable)

### Vacuum Operation
```sql
-- Reclaim disk space after large deletes
VACUUM;

-- Should run monthly
ANALYZE;  -- Update query optimization statistics
```

### Integrity Check
```sql
-- Verify database integrity
PRAGMA integrity_check;

-- Check for corruption
PRAGMA foreign_key_check;
```

---

## Performance Optimization

### Write Performance
- **Batch inserts**: Group multiple rows in single transaction
- **Index strategy**: Create indexes on frequently queried columns
- **WAL mode**: Enable Write-Ahead Logging for concurrent access

```sql
PRAGMA journal_mode = WAL;
PRAGMA synchronous = NORMAL;
PRAGMA cache_size = 10000;
```

### Query Performance
- Use indexes on timestamp, node_id, motor_state, alarm_type
- Partition data by time (recent vs. historical)
- Consider materialized views for common aggregations

---

## Data Privacy & Compliance

- **Data Retention**: Comply with industrial data retention requirements
- **Access Control**: Restrict database access to authorized services
- **Encryption**: Consider encrypting sensitive operational data
- **Audit Trail**: Maintain all changes to critical settings

---

**Last Updated**: May 2026  
**Version**: 1.0
