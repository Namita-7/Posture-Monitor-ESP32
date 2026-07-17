# Complementary Filter - Fusing Accelerometer and Gyroscope Data

## What is a Complementary Filter?

A complementary filter is a technique that combines data from two sensors to get the best accuracy:
- **Accelerometer:** Always correct but noisy
- **Gyroscope:** Fast and smooth but drifts

The filter blends both signals to create a smooth, accurate, stable measurement.

---

## The Problem We're Solving

### Accelerometer Alone
```
Pitch reading:
  ▲ 15°
  │     ╱╲    ╱╲    ╱╲
  │    ╱  ╲  ╱  ╲  ╱  ╲     ← Noisy!
  │   ╱    ╲╱    ╲╱    ╲
  │  10°    (oscillating with noise)
  └─────────────────────────
```

**Problem:** Too much noise makes it hard to detect real posture changes.

### Gyroscope Alone
```
Pitch reading:
  ▲ 15°
  │     ╱╱╱╱╱
  │    ╱ (drifting up)
  │   ╱
  │  5° ←────── True angle stays here
  │ ╱
  └─────────────────────────
    (sensor bias accumulates)
```

**Problem:** Over time it drifts away from the true value and never comes back.

---

## The Solution: Complementary Filter

### The Formula
```cpp
pitch = alpha * (pitch + gxf * dt) + (1.0 - alpha) * accelPitch;
```

Where:
- **alpha = 0.96** (96% weight on gyro)
- **(1 - alpha) = 0.04** (4% weight on accel)

### In Plain English
```
New Pitch = (96% of gyro prediction) + (4% of accel correction)
```

---

## Why This Works

### Gyroscope (96% weight)
**Fast Response:**
- Detects motion immediately
- Smooth between updates
- No lag

**Why not 100%?**
- Would drift indefinitely
- Error accumulates over minutes/hours
- Eventually reads completely wrong

### Accelerometer (4% weight)
**Long-term Correction:**
- Pulls the gyro back to reality
- Eliminates drift
- Happens slowly (4% per update)

**Why only 4%?**
- If too high → back to being noisy
- 4% is enough to fix drift in ~25 seconds
- Fast enough for practical use
- Slow enough to filter out noise

---

## Mathematical Breakdown

### Time Step Updates

**Every 50ms, this happens:**

```
Step 1: Predict with gyro
  predicted_pitch = previous_pitch + (angular_velocity * 0.05s)
  
Step 2: Get accel measurement
  accel_pitch = atan2(ax, √(ay²+az²)) * 180/π
  
Step 3: Blend them (0.96 gyro, 0.04 accel)
  final_pitch = 0.96 * predicted_pitch + 0.04 * accel_pitch
```

### Numerical Example

```
Scenario: Leaning forward over 500ms

Time | Accel | Gyro Rate | Gyro Pred | Filter Output
-----|-------|-----------|-----------|---------------
0ms  | 0°    | 0°/s      | 0°        | 0°
50ms | -5°   | 20°/s     | 1°        | 0.96(1) + 0.04(-5) = 0.8°
100ms| -10°  | 20°/s     | 1.8°      | 0.96(1.8) + 0.04(-10) = 1.53°
150ms| -15°  | 20°/s     | 2.6°      | 0.96(2.6) + 0.04(-15) = 2.22°
200ms| -20°  | 20°/s     | 3.4°      | 0.96(3.4) + 0.04(-20) = 2.86°
250ms| -23°  | 20°/s     | 4.2°      | 0.96(4.2) + 0.04(-23) = 3.45°
```

**Notice:**
- Output (3.45°) is between gyro (4.2°) and accel (-23°)
- Smooth curve, not noisy
- Following the real trend

---

## Visualizing the Filter

### Without Filter (just accelerometer)
```
Pitch (°)
  20 │                    ╱╲╱╲╱╲
  15 │        ╱─╲────╱╱╲  ╱  ╱  ╱  ← Jittery, hard to use
  10 │       ╱   ╲  ╱   ╲╱
   5 │      ╱     ╲╱
   0 └──────────────────────────────
     Time (seconds)
```

### Without Filter (just gyroscope)
```
Pitch (°)
  30 │                ╱────────────  ← Drifting higher!
  20 │            ╱╱╱               
  10 │        ╱╱╱ ← True angle is here (5°)
   5 │  ╱╱╱
   0 └──────────────────────────────
     Time (seconds)
```

### With Complementary Filter
```
Pitch (°)
  10 │        ╱─────────────────  ← Smooth curve
   5 │     ╱╱                  ╲   ← Stays near true value
   0 │  ╱╱                       
  -5 └──────────────────────────────
     Time (seconds)
```

---

## Why Alpha = 0.96?

### Tuning Considerations

**Alpha = 0.90** (more accel correction)
```
Pros:  Faster drift correction (15 seconds)
Cons:  More visible noise in output
Use:   Very accurate starting position needed
```

**Alpha = 0.96** (balanced)
```
Pros:  Smooth AND corrects drift (25 seconds)
Cons:  Medium sensitivity to noise
Use:   Our default choice ✓
```

**Alpha = 0.99** (more gyro trust)
```
Pros:  Very smooth, minimal noise
Cons:  Slow drift correction (100 seconds)
Use:   Only if calibration is perfect
```

### How dt Affects Alpha

Our system uses:
- **dt = 0.05** (50ms update period)
- **alpha = 0.96** (4% correction per update)

If we changed to dt = 0.1 (100ms), we'd need higher alpha (~0.98) to maintain same correction speed.

---

## Drift Correction Timeline

### Starting with 1° gyro bias

```
Time Elapsed | Gyro Error | Accel Correction | Net Pitch
-------------|------------|------------------|----------
0s           | 0°         | 0°               | 0°
5s (100 ups) | 1°         | -0.04°           | 0.96°
10s (200)    | 2°         | -0.08°           | 1.92°
25s (500)    | 5°         | -0.20°           | 4.80°  ← Still drifting
50s (1000)   | 10°        | -0.40°           | 9.60°  ← Gets worse
```

**Why?** Even though accel pulls back by 4%, the gyro bias adds 1° every 1 second, which outpaces the correction.

**Solution:** Add gyro bias calibration (see CALIBRATION.md) to reduce the source error.

---

## Filter Stability

### Stability Conditions

For the filter to remain stable:
1. **0 < alpha < 1** ✅ (We use 0.96)
2. **dt matches loop frequency** ✅ (We use 50ms)
3. **Sensor values don't saturate** ✅ (Checked during calibration)

### What Breaks the Filter

❌ **dt changes:**
- If loop suddenly runs at 100ms but dt = 0.05
- Angle changes become 2x larger than they should
- Filter can't keep up with real motion

❌ **Alpha = 1.0:**
- Ignores accelerometer completely
- Full gyro drift, no correction

❌ **Alpha = 0:**
- Ignores gyro completely
- Just noisy accelerometer

---

## Summary

| Aspect | Accelerometer | Gyroscope | Complementary Filter |
|--------|---------------|-----------|----------------------|
| **Response** | Slow | Fast | Fast |
| **Accuracy** | Perfect | Drifts | Stable |
| **Noise** | High | Low | Low |
| **Long-term** | Always correct | Diverges | Perfect |
| **Algorithm** | atan2() | integrate | 96/4 blend |

**The complementary filter gives you the best of both worlds: fast response + accurate reading + low noise!** ✅
