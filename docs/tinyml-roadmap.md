# TinyML Roadmap & Implementation Guide

Phase 8: Machine Learning-based Motor Anomaly Detection on ESP32

---

## Overview

TinyML (Tiny Machine Learning) enables on-device inference for real-time anomaly detection without relying on cloud connectivity. This guide covers planning, implementation, and deployment of TinyML on ESP32.

**Key Principle**: TinyML is implemented AFTER the core embedded system is stable (Phase 7).

---

## Why Phase 8?

### Dependencies
- ✓ Phase 1-7: Embedded system stable and operational
- ✓ Modbus communication reliable
- ✓ MQTT telemetry flowing to Raspberry Pi
- ✓ SQLite training_log table populated with labeled data
- ✓ 3+ weeks of operational data collected

### Risk Mitigation
- Core system not disrupted by AI development
- Sufficient historical data for training
- Stable platform for testing models
- Performance bottlenecks identified

---

## Problem Statement

**Objective**: Detect motor anomalies in real-time using on-device ML inference

**Inputs** (Motor Telemetry):
- Motor PWM (%)
- Motor current (mA)
- Motor voltage (V)
- LM35 temperature (°C)
- BMP280 temperature (°C)
- Pressure (hPa)
- Motor state (categorical)
- Control mode (categorical)
- Alarm flags (bitmask)

**Output** (Prediction):
```
Classification: NORMAL / WARNING / ANOMALY

(or more granular)

NORMAL
├─ Normal operation
├─ Light load
└─ Stable temperature

WARNING
├─ Heavy load (expected)
└─ Temperature rising

ANOMALY
├─ Heavy load with voltage drop (unusual)
├─ Stall (high current, low speed)
├─ Thermal runaway risk
└─ Sensor malfunction
```

**Anomaly Score**: 0.0 (normal) to 1.0 (anomaly)

---

## Data Collection Strategy

### Phase 1: Data Gathering (Weeks 1-3)

**Objective**: Collect diverse operational scenarios

**Normal Operation Data**:
- Motor running at 20%, 40%, 60%, 80%, 100% PWM
- Duration: 30 minutes each
- Label: `NORMAL`
- Expected samples: 18,000 (at 1 sample/sec)

**Steady-State Operation**:
- 8 hours continuous operation at 60% PWM
- Label: `NORMAL`
- Expected samples: 28,800

**Temperature Variation**:
- Ambient temperature from 20°C to 35°C
- Record LM35 response
- Label: `NORMAL`
- Expected samples: 5,000

**Load Testing Data**:
- Gradually increase PWM from 0% to 100%
- Observe current response
- Label: `NORMAL` (if expected response), `HEAVY_LOAD` (if unusual current spike)
- Expected samples: 10,000

**Fault Scenario Data**:

1. **Over-Current Event**:
   - Deliberately trigger by increasing load
   - Duration: 1-2 minutes
   - Label: `OVER_CURRENT`
   - Expected samples: 60-120

2. **Overheat Scenario**:
   - Apply gentle heat to LM35 using warm water (not direct heat)
   - Carefully approach 60-65°C threshold
   - Label: `OVER_TEMP` or `OVERHEAT_RISK`
   - Expected samples: 100-200
   - SAFETY: Do not damage motor

3. **Stall Condition**:
   - Mechanically jam motor briefly
   - Observe high current with limited speed change
   - Duration: 10-20 seconds
   - Label: `STALL_RISK` or `MOTOR_STALL`
   - Expected samples: 20-40

4. **Sensor Error**:
   - Disconnect INA219 or BMP280 temporarily
   - Observe error handling
   - Label: `SENSOR_ERROR`
   - Expected samples: 50-100

**Total Dataset Target**: 60,000-80,000 labeled samples

### Phase 2: Data Labeling (Week 4)

**Labeling Guidelines**:

| Label | Criteria | Confidence |
|-------|----------|-----------|
| NORMAL | Normal operation, all sensors OK, current < threshold, temp < warning | 1.0 |
| HEAVY_LOAD | High current (> 1000mA) expected for load, voltage stable | 0.9 |
| OVERHEAT_RISK | Temp > 50°C but < 65°C, fans running, no other issues | 0.9 |
| STALL_RISK | High current with low voltage drop OR motor not spinning | 0.8 |
| SENSOR_ERROR | Any sensor reading invalid or absent | 0.7 |
| OVER_CURRENT | Current > 1500mA sustained | 0.95 |
| OVER_TEMP | Temp > 65°C | 0.95 |

**Labeling Process**:
1. Export telemetry windows (e.g., 60-second segments)
2. Review sensor plots and context
3. Assign label based on criteria
4. Assign confidence score (0.7-1.0)
5. Add notes if ambiguous

---

## Model Selection Strategy

### Recommendation: Start Simple, Iterate

#### Option 1: Rule-Based Baseline (Simplest)

**Logic**:
```python
if current > 1500 mA:
    anomaly_score = 0.9
elif temp > 60°C:
    anomaly_score = 0.8
elif (temp > 50°C) and (current > 1000 mA):
    anomaly_score = 0.7
else:
    anomaly_score = 0.2
```

**Pros**: No training, fast inference, interpretable
**Cons**: Limited accuracy, cannot capture complex patterns

#### Option 2: Decision Tree

**Example**:
```
if current > 1200 mA:
    if voltage < 11V:
        predict STALL_RISK
    else:
        predict HEAVY_LOAD
elif temp > 60°C:
    if pressure < 1000 hPa:
        predict OVERHEAT_RISK
    else:
        predict NORMAL
else:
    predict NORMAL
```

**Pros**: Better accuracy than rules, still interpretable
**Cons**: Prone to overfitting on training data

#### Option 3: Logistic Regression

**Input**: 8 features (normalized)
**Output**: Binary classification (NORMAL=0, ANOMALY=1) or multi-class probabilities

```python
anomaly_score = sigmoid(w0 + w1*current + w2*temp + ... + w8*pwm)
```

**Pros**: Lightweight, good for linear relationships
**Cons**: May miss non-linear patterns

#### Option 4: Tiny Neural Network (Recommended)

**Architecture**:
```
Input Layer (8 features)
    ↓
Dense Layer 1: 16 neurons, ReLU activation
    ↓
Dense Layer 2: 8 neurons, ReLU activation
    ↓
Dense Layer 3: 3 neurons, Softmax (for 3 classes)
    ↓
Output: [NORMAL, WARNING, ANOMALY] probabilities
```

**Pros**: Captures non-linear patterns, compact model size
**Cons**: Requires careful tuning, training overhead

---

## Model Training Workflow

### Tools & Environment

```bash
# On Raspberry Pi or development PC
Python 3.8+
TensorFlow 2.11+ (CPU or GPU)
TensorFlow Lite for Microcontrollers
scikit-learn (for preprocessing)
pandas, numpy, matplotlib
```

### Training Pipeline

#### Step 1: Data Preparation

```python
import pandas as pd
import numpy as np
from sklearn.preprocessing import StandardScaler

# Load training data from SQLite
df = pd.read_sql("""
    SELECT motor_pwm, motor_current, motor_voltage, lm35_temp, 
           bmp280_temp, pressure, motor_state, control_mode, label
    FROM training_log
    WHERE confidence >= 0.8
""", db_connection)

# Split into features and labels
X = df[['motor_pwm', 'motor_current', 'motor_voltage', 'lm35_temp', 
        'bmp280_temp', 'pressure']].values
y = df['label'].values

# Encode categorical features
state_mapping = {'STOP': 0, 'RUNNING': 1, 'WARNING': 2, 'FAULT': 3}
mode_mapping = {'MANUAL': 0, 'AUTO': 1}
label_mapping = {'NORMAL': 0, 'WARNING': 1, 'ANOMALY': 2}

# Normalize features
scaler = StandardScaler()
X_scaled = scaler.fit_transform(X)

# Train/test split (80/20)
from sklearn.model_selection import train_test_split
X_train, X_test, y_train, y_test = train_test_split(
    X_scaled, y, test_size=0.2, random_state=42, stratify=y
)
```

#### Step 2: Model Training

```python
import tensorflow as tf

# Build model
model = tf.keras.Sequential([
    tf.keras.layers.Dense(16, activation='relu', input_shape=(8,)),
    tf.keras.layers.Dropout(0.2),
    tf.keras.layers.Dense(8, activation='relu'),
    tf.keras.layers.Dropout(0.2),
    tf.keras.layers.Dense(3, activation='softmax')
])

# Compile
model.compile(
    optimizer='adam',
    loss='sparse_categorical_crossentropy',
    metrics=['accuracy']
)

# Train
history = model.fit(
    X_train, y_train,
    validation_data=(X_test, y_test),
    epochs=50,
    batch_size=32,
    verbose=1
)

# Evaluate
loss, accuracy = model.evaluate(X_test, y_test)
print(f"Accuracy: {accuracy:.2%}")
```

#### Step 3: Model Evaluation

```python
from sklearn.metrics import classification_report, confusion_matrix

# Predictions
y_pred = np.argmax(model.predict(X_test), axis=1)

# Metrics
print(classification_report(y_test, y_pred, 
    target_names=['NORMAL', 'WARNING', 'ANOMALY']))

# Confusion matrix
print(confusion_matrix(y_test, y_pred))
```

#### Step 4: Quantization & Conversion

```python
# Convert to TensorFlow Lite
converter = tf.lite.TFLiteConverter.from_keras_model(model)
converter.optimizations = [tf.lite.Optimize.DEFAULT]
converter.target_spec.supported_ops = [
    tf.lite.OpsSet.TFLITE_BUILTINS,
    tf.lite.OpsSet.SELECT_TF_OPS
]

tflite_model = converter.convert()

# Save model
with open('motor_anomaly_model.tflite', 'wb') as f:
    f.write(tflite_model)

# Check model size
import os
model_size_kb = os.path.getsize('motor_anomaly_model.tflite') / 1024
print(f"Model size: {model_size_kb:.1f} KB")
```

#### Step 5: Convert to C Array (for ESP32 embedding)

```python
# Convert TFLite model to C byte array
import array

with open('motor_anomaly_model.tflite', 'rb') as f:
    model_bytes = array.array('B', f.read())

# Generate C header file
with open('model.h', 'w') as f:
    f.write('#pragma once\n')
    f.write('#include <cstdint>\n\n')
    f.write(f'const uint32_t model_size = {len(model_bytes)};\n')
    f.write('const uint8_t model[] = {\n')
    
    for i, byte in enumerate(model_bytes):
        if i % 12 == 0:
            f.write('\n    ')
        f.write(f'0x{byte:02x},')
    
    f.write('\n};\n')
```

---

## ESP32 Inference Engine

### Implementation Structure

```c
// inference.h
#ifndef INFERENCE_H
#define INFERENCE_H

#include <stdint.h>

typedef struct {
    float motor_pwm;
    float motor_current;
    float motor_voltage;
    float lm35_temp;
    float bmp280_temp;
    float pressure;
} MotorFeatures;

typedef struct {
    const char* status;        // "NORMAL", "WARNING", "ANOMALY"
    float anomaly_score;       // 0.0 - 1.0
    const char* predicted_state;
    uint32_t inference_time_ms;
} InferenceResult;

// Initialize TinyML model
void inference_init(void);

// Run inference on motor features
InferenceResult inference_predict(const MotorFeatures* features);

// Get model version
const char* inference_get_model_version(void);

#endif
```

### Feature Extraction

```c
// feature_extract.c
#include "inference.h"
#include "modbus_client.h"  // To read STM32 registers

static float scaler_mean[] = {50.0, 400.0, 12.0, 45.0, 30.0, 1010.0};
static float scaler_std[] = {30.0, 300.0, 1.0, 15.0, 5.0, 50.0};

void extract_features(MotorFeatures* features) {
    // Read from Modbus registers
    uint16_t pwm = modbus_read_register(40006);
    uint16_t current = modbus_read_register(40005);
    uint16_t voltage = modbus_read_register(40004);
    int16_t lm35 = modbus_read_register(40001);
    int16_t bmp280_t = modbus_read_register(40002);
    uint16_t pressure = modbus_read_register(40003);
    
    // Convert to physical units
    features->motor_pwm = (float)pwm;
    features->motor_current = (float)current;
    features->motor_voltage = (float)voltage / 100.0f;
    features->lm35_temp = (float)lm35 / 10.0f;
    features->bmp280_temp = (float)bmp280_t / 10.0f;
    features->pressure = (float)pressure;
    
    // Normalize (standardization)
    features->motor_pwm = (features->motor_pwm - scaler_mean[0]) / scaler_std[0];
    features->motor_current = (features->motor_current - scaler_mean[1]) / scaler_std[1];
    features->motor_voltage = (features->motor_voltage - scaler_mean[2]) / scaler_std[2];
    features->lm35_temp = (features->lm35_temp - scaler_mean[3]) / scaler_std[3];
    features->bmp280_temp = (features->bmp280_temp - scaler_mean[4]) / scaler_std[4];
    features->pressure = (features->pressure - scaler_mean[5]) / scaler_std[5];
}
```

### Model Inference

```c
// inference.c
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "model.h"  // Generated C array

static const tflite::Model* model = nullptr;
static tflite::MicroInterpreter* interpreter = nullptr;
static TfLiteTensor* input_tensor = nullptr;
static TfLiteTensor* output_tensor = nullptr;

constexpr int kArenaSize = 8 * 1024;  // 8 KB
uint8_t tensor_arena[kArenaSize];

void inference_init(void) {
    // Load model
    model = tflite::GetModel(model);
    
    // Set up resolver
    static tflite::AllOpsResolver resolver;
    
    // Create interpreter
    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, kArenaSize
    );
    interpreter = &static_interpreter;
    
    // Allocate tensors
    interpreter->AllocateTensors();
    
    // Get pointers
    input_tensor = interpreter->input(0);
    output_tensor = interpreter->output(0);
}

InferenceResult inference_predict(const MotorFeatures* features) {
    InferenceResult result = {0};
    
    uint32_t start_time = esp_timer_get_time() / 1000;  // milliseconds
    
    // Populate input tensor
    float* input = input_tensor->data.f;
    input[0] = features->motor_pwm;
    input[1] = features->motor_current;
    input[2] = features->motor_voltage;
    input[3] = features->lm35_temp;
    input[4] = features->bmp280_temp;
    input[5] = features->pressure;
    
    // Run inference
    interpreter->Invoke();
    
    // Extract output
    float* output = output_tensor->data.f;
    float normal_prob = output[0];
    float warning_prob = output[1];
    float anomaly_prob = output[2];
    
    // Determine status
    float max_prob = (normal_prob > warning_prob) ? normal_prob : warning_prob;
    max_prob = (max_prob > anomaly_prob) ? max_prob : anomaly_prob;
    
    if (anomaly_prob == max_prob && anomaly_prob > 0.7f) {
        result.status = "ANOMALY";
        result.predicted_state = "ANOMALY_DETECTED";
        result.anomaly_score = anomaly_prob;
    } else if (warning_prob == max_prob && warning_prob > 0.5f) {
        result.status = "WARNING";
        result.predicted_state = "MONITOR_REQUIRED";
        result.anomaly_score = warning_prob;
    } else {
        result.status = "NORMAL";
        result.predicted_state = "NORMAL_OPERATION";
        result.anomaly_score = 1.0f - normal_prob;
    }
    
    result.inference_time_ms = (esp_timer_get_time() / 1000) - start_time;
    
    return result;
}

const char* inference_get_model_version(void) {
    return "v1.0";
}
```

---

## ESP32 Integration

### TinyML Task

```c
// freertos tasks
void tinyml_task(void* pvParameters) {
    MotorFeatures features;
    InferenceResult result;
    
    // Initialize TinyML
    inference_init();
    
    while (1) {
        // Extract features from Modbus registers
        extract_features(&features);
        
        // Run inference
        result = inference_predict(&features);
        
        // Log result
        update_ai_registers(result);
        
        // Publish to MQTT
        publish_ai_result(result);
        
        // Update HMI screen
        display_ai_status(result);
        
        // Wait for next cycle (1 second)
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
```

### Memory & Performance

**Memory Usage**:
- Model size: ~50-100 KB (quantized)
- Tensor arena: 8-16 KB
- Total: ~60-120 KB (well within ESP32 limits)

**Performance**:
- Inference time: 20-50 ms typical
- Throughput: 20-50 inferences/second possible
- Can run at 1 inference/second comfortably

---

## Evaluation & Validation

### Offline Evaluation (Raspberry Pi)

```python
# Test model on held-out test set
from sklearn.metrics import accuracy_score, f1_score, roc_auc_score

y_pred = model.predict(X_test)
y_pred_classes = np.argmax(y_pred, axis=1)

print(f"Accuracy: {accuracy_score(y_test, y_pred_classes):.2%}")
print(f"F1 Score (weighted): {f1_score(y_test, y_pred_classes, average='weighted'):.2f}")
```

### Online Evaluation (System Integration)

1. **Deploy model to ESP32**
2. **Run system for 1 week** with AI enabled
3. **Collect inference results** to `ai_result` table
4. **Compare with ground truth** (manual observations)
5. **Calculate real-world accuracy**
6. **Iterate if needed** (retrain with new data)

---

## Model Versioning & Retraining

### Version Management

```
motor_anomaly_v1.0.tflite  - Initial model
motor_anomaly_v1.1.tflite  - After 2 weeks tuning
motor_anomaly_v2.0.tflite  - Major retraining
```

### Retraining Cadence

- **Monthly**: Evaluate model performance, collect new failure modes
- **Quarterly**: Retrain if F1 score drops below 0.85
- **Yearly**: Major retraining with new hardware variations

### CI/CD Integration

```bash
# Automated workflow
1. Export training_log to CSV
2. Run preprocessing
3. Train model
4. Evaluate on test set
5. If accuracy > 90%:
   - Quantize model
   - Convert to C array
   - Deploy to GitHub releases
6. Notify team
```

---

## Deployment Checklist

- [ ] Model accuracy >= 90% on test set
- [ ] F1 score >= 0.85 (weighted average)
- [ ] Inference time <= 100 ms
- [ ] Model size <= 150 KB
- [ ] Memory footprint acceptable (< 256 KB)
- [ ] Tested on real ESP32 hardware
- [ ] No performance impact on motor control
- [ ] MQTT publishing working
- [ ] AI results logged to database
- [ ] HMI displays AI status correctly
- [ ] Team trained on model interpretation
- [ ] Documented for future maintenance

---

## Future Enhancements

- **Transfer Learning**: Use pre-trained models from other industrial domains
- **Federated Learning**: Train collaboratively across multiple motor systems
- **Continuous Learning**: Adapt model online as system operates
- **Explainability**: Add LIME or SHAP for model interpretability
- **Ensemble Methods**: Combine multiple models for robustness
- **Edge TPU**: Integrate Google Coral TPU for faster inference

---

**Last Updated**: May 2026  
**Version**: 1.0  
**Status**: Design Phase - Ready for Phase 8 Implementation
