# Angle Calculations - How the MPU6050 Works

## Overview
The Posture Monitor uses the MPU6050 sensor to calculate body tilt angles. This document explains the complete calculation process.

## Step 1: Read Raw Sensor Data

The MPU6050 provides two types of measurements:

### Accelerometer (3 axes)
```cpp
int16_t ax = Wire.read() << 8 | Wire.read();  // X-axis
int16_t ay = Wire.read() << 8 | Wire.read();  // Y-axis
int16_t az = Wire.read() << 8 | Wire.read();  // Z-axis
```

- **Measures:** Gravitational acceleration (including tilt)
- **Range:** ±16g (16-bit signed integer)
- **Use:** Accurate but noisy

### Gyroscope (rotation rate)
```cpp
int16_t gx = Wire.read() << 8 | Wire.read();  // Rotation around X
int16_t gy = Wire.read() << 8 | Wire.read();  // Rotation around Y
int16_t gz = Wire.read() << 8 | Wire.read();  // Rotation around Z
```

- **Measures:** Angular velocity (rotation speed)
- **Range:** ±250°/s (16-bit signed integer)
- **Use:** Fast response but drifts over time

---

## Step 2: Convert to Real Units

### Accelerometer Conversion
```cpp
float axf = ax / 16384.0;  // ÷16384 = convert to g (gravity units)
float ayf = ay / 16384.0;
float azf = az / 16384.0;
```

**Why 16384?**
- MPU6050 spec: ±2g default range with 16-bit resolution
- 2g range = 4g total ÷ 2^15 = 16384 LSB per g

### Gyroscope Conversion
```cpp
float gxf = gx / 131.0;    // ÷131 = convert to °/s
float gyf = gy / 131.0;
float gzf = gz / 131.0;
```

**Why 131?**
- MPU6050 spec: ±250°/s range with 16-bit resolution
- 250°/s range = 500°/s total ÷ 2^15 = 131 LSB per °/s

---

## Step 3: Calculate Pitch from Accelerometer

### The Formula
```cpp
float accelPitch = atan2(axf, sqrt(ayf*ayf + azf*azf)) * 180.0 / PI;
```

### What This Does

**atan2(X, √(Y²+Z²))** calculates the angle of tilt:
- Takes the X-axis acceleration
- Compares it to the combined Y and Z acceleration
- Returns the angle of tilt from vertical
- Multiply by 180/π converts from radians to degrees

### Visual Examples

```
Sitting Straight:
  ax ≈ 0g, ay ≈ 0g, az ≈ 1g
  accelPitch = atan2(0, 1) * 180/π ≈ 0°
  
Leaning Forward 30°:
  ax ≈ 0.5g, ay ≈ 0g, az ≈ 0.87g
  accelPitch = atan2(0.5, 0.87) * 180/π ≈ 30°
  
Leaning Back 20°:
  ax ≈ -0.34g, ay ≈ 0g, az ≈ 0.94g
  accelPitch = atan2(-0.34, 0.94) * 180/π ≈ -20°
```

**Pros:**
- ✅ Always accurate (gravity direction is fixed)
- ✅ No drift over time

**Cons:**
- ❌ Noisy (affected by vibration, acceleration)
- ❌ Slow to respond (needs filtering)

---

## Step 4: Gyroscope Integration

### The Formula
```cpp
float dt = 0.05;  // 50 milliseconds
pitch_from_gyro = pitch + gxf * dt;
```

### What This Does

- **gxf** = current rotation rate (°/s)
- **dt** = time since last update (0.05 seconds)
- **gxf * dt** = angle rotated in this time period
- Add it to previous pitch to get new pitch

### Example Calculation
```
Time 0ms:  pitch = 0°
Rotating at 20°/s for 50ms:
  angle_rotated = 20 * 0.05 = 1°
  new_pitch = 0 + 1 = 1°

Continues rotating for another 50ms:
  angle_rotated = 20 * 0.05 = 1°
  new_pitch = 1 + 1 = 2°
```

**Pros:**
- ✅ Very fast response
- ✅ Smooth (not noisy)

**Cons:**
- ❌ Drifts over time (sensor bias)
- ❌ Eventually reads wrong angle

---

## Step 5: Complementary Filter (Fusion)

### The Problem
- Accelerometer: Accurate but noisy and slow
- Gyroscope: Fast and smooth but drifts

### The Solution: Blend Them!

```cpp
float alpha = 0.96;  // trust gyro 96%, accel 4%

pitch = alpha * (pitch + gxf * dt) + (1.0 - alpha) * accelPitch;
```

### Breaking It Down

**Gyro part (96%):**
```
alppha * (pitch + gxf * dt)
= 0.96 * (previous_pitch + rotation)
```
Fast response from gyroscope

**Accel part (4%):**
```
(1.0 - alpha) * accelPitch
= 0.04 * accurate_pitch_from_gravity
```
Slow drift correction from accelerometer

### Why 96/4 Split?
- Gyroscope updates every 50ms → fast enough to use mostly
- Accelerometer slowly corrects drift → only needs 4%
- This ratio is tuned for our 50ms loop period

### Example Over Time

```
Time | Accel | Gyro Integrated | Filtered (96/4) | Status
-----|-------|-----------------|-----------------|--------
0ms  | 0°    | 0°              | 0°              | ✓ Start
50ms | -2°   | 0.5°            | 0.48°           | Noise filtered
100ms| -1.5° | 1.0°            | 0.98°           | Smooth curve
150ms| -0.5° | 1.5°            | 1.48°           | Following true angle
```

---

## Step 6: Compare to Baseline

### Calculate Deviation
```cpp
float deviation = abs(pitch - baseline);
```

### Example
```
During calibration (sitting straight):
  baseline = 5.2°

While working:
  pitch = 5.2°  → deviation = 0°   → ✓ GOOD POSTURE
  pitch = 15.3° → deviation = 10°  → ✓ GOOD POSTURE
  pitch = 28.5° → deviation = 23.3°→ ❌ BAD POSTURE (>22°)
```

---

## Complete Data Flow

```
┌─────────────────────────────────┐
│   MPU6050 Raw Sensor Data       │
│  (16-bit accelerometer/gyro)    │
└────────────┬────────────────────┘
             │
             ▼
┌─────────────────────────────────┐
│  Convert to Real Units          │
│  (÷16384 for accel, ÷131 gyro)  │
└────────────┬────────────────────┘
             │
      ┌──────┴──────┐
      ▼             ▼
  [ACCEL]      [GYRO]
   atan2()    integration
     │             │
     └──────┬──────┘
            ▼
   ┌─────────────────────┐
   │ Complementary Filter│
   │   (96% gyro + 4%)   │
   └────────┬────────────┘
            ▼
      ┌──────────────┐
      │ Final Pitch  │
      │   Angle      │
      └────────┬─────┘
               ▼
      ┌──────────────────┐
      │ Compare to       │
      │ Baseline         │
      │ (hysteresis)     │
      └────────┬─────────┘
               ▼
      ┌──────────────────┐
      │ LED / Vibrator   │
      │ ON/OFF Decision  │
      └──────────────────┘
```

---

## Summary

| Component | Formula | Purpose | Speed | Accuracy |
|-----------|---------|---------|-------|----------|
| **Accel** | atan2() | Gravity reference | Slow | High |
| **Gyro** | integrate | Motion detection | Fast | Drifts |
| **Filter** | 96/4 blend | Best of both | Fast | Stable |
| **Baseline** | abs(delta) | Posture check | Real-time | Corrected |

This combination gives you **smooth, accurate, responsive** posture detection! ✅
