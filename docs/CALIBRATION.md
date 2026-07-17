# Calibration - Setting Your Baseline

## What is Calibration?

Calibration establishes your "good posture" reference point (baseline). Everything else is measured relative to this.

```
Calibration Reading: 5.2°  ← This is your BASELINE

During Use:
  Pitch = 5.2°  → Deviation = 0°   → ✓ GOOD POSTURE
  Pitch = 28.4° → Deviation = 23.2°→ ❌ BAD POSTURE
```

---

## Calibration Process

### Step 1: Initialization
```cpp
void calibrate()
{
  float sum = 0.0;
  for (int i = 0; i < 300; i++)  // 300 readings
  {
    // ... read sensor ...
    // ... calculate pitch ...
    sum += pitch;
    delay(10);  // 10ms between reads
  }
  baseline = sum / 300.0;  // Average all readings
}
```

**What happens:**
- Takes 300 sensor readings
- Each reading is 10ms apart
- Total time: 300 × 10ms = **3 seconds**
- Averages them all together

### Step 2: Perfect Position

During those 3 seconds, you should:
1. **Sit completely straight**
2. **Don't move at all**
3. **Maintain good upper back posture**

```
Correct Position:        Wrong Position:
  ╔═══════╗                 ╱╱╱╱ ← Leaning
  ║       ║                ╱  body tilted
  ║ Back  ║               │
  ║Straight              │
  ║       ║              │
  ╚═══════╝              ╲
    ✓ GOOD               ✗ BAD
```

---

## Sensor Readings During Calibration

### Accelerometer Component

```cpp
float accelPitch = atan2(axf, sqrt(ayf*ayf + azf*azf)) * 180.0 / PI;
```

**While sitting straight:**
- Gravity pulls straight down (along Z-axis)
- X-axis acceleration ≈ 0g
- Y-axis acceleration ≈ 0g
- Z-axis acceleration ≈ 1g

```
atan2(0, 1) × 180/π = 0°
```

**Example readings (sitting straight):**
```
Reading 1: accelPitch = 0.2°   (tiny noise)
Reading 2: accelPitch = -0.1°  (tiny noise)
Reading 3: accelPitch = 0.15°  (tiny noise)
Reading 4: accelPitch = 0.05°  (tiny noise)
...
Average (baseline) = 0.08°
```

### Gyroscope Component

During calibration, gyro integration also happens:
```cpp
pitch = alpha * (pitch + gxf * dt) + (1.0 - alpha) * accelPitch;
```

But since you're sitting still:
- gxf ≈ 0°/s (no rotation)
- gxf * dt ≈ 0°
- pitch ≈ accelPitch (just the acceleration)

**Result:** Calibration mostly measures accelerometer angle, which is accurate for a stationary position.

---

## Baseline Values

### Common Baseline Values

```
Sensor Mounted At:        Expected Baseline:
─────────────────────────────────────────────
Base of neck              2° to 5°
Between shoulder blades   0° to 3°
Upper back                -2° to 2°
Lower back (not recommended)  5° to 15°
```

**Why different values?**
- Sensor orientation affects measurement
- Your body shape affects angle
- Sensor placement relative to spine varies

### Example Baselines

```
Person A (straight posture):
  baseline = 2.3°
  
Person B (slight forward head):
  baseline = 8.1°
  
Person C (very upright):
  baseline = -1.5°
```

**All are valid!** Each person's baseline is their personal reference.

---

## What Goes Wrong During Calibration?

### Problem 1: Moving During Calibration

```
If you move:
  Reading 1: 0.2°   (straight)
  Reading 2: 5.1°   (leaning forward by accident)
  Reading 3: 0.15°  (back to straight)
  ...
  baseline = 2.1°   ← TOO HIGH!
```

**Result:** Your system thinks slouching is normal → won't detect bad posture.

**Fix:** Hold completely still for the 3-second calibration period.

### Problem 2: Starting Slouched

```
If you start slouched:
  Reading 1-300: ~15°  (slouching)
  baseline = 15.0°     ← WRONG!
```

**Result:** You have to slouch even more to trigger bad posture alarm.

**Fix:** Sit up straight before calibration starts.

### Problem 3: Sensor Not Settled

```
If sensor is moving:
  Reading 1: 0.2°
  Reading 2: 2.1°   (moved slightly)
  Reading 3: 0.8°   (moved back)
  Reading 4: 1.5°   (still moving)
  ...
  baseline = 1.2°   ← Noisy
```

**Result:** False positives/negatives due to calibration noise.

**Fix:** Mount sensor securely before powering on.

---

## Multi-Person Calibration

### Each Person Needs Their Own Baseline

```
Person A calibrates: baseline_A = 3.2°
Person B uses system: baseline_A = 3.2° (WRONG!)
  Result: Different thresholds for different people
  
Correct:
Person B calibrates: baseline_B = 5.1°
Person B uses system: baseline_B = 5.1° ✓
```

### How to Recalibrate

**Option 1: Power Cycle**
- Unplug ESP32
- Wait 2 seconds
- Power on
- Sit straight during 3-second calibration

**Option 2: Add Reset Button** (Future feature)
```cpp
if (resetButtonPressed()) {
  calibrate();  // Run calibration again
}
```

---

## Calibration Quality Checks

### Good Calibration
- Baseline is small (0° to 10°)
- After calibration, sitting straight → deviation ≈ 0°
- Slouching forward → clear increase in deviation
- LED/vibrator responds smoothly

```
Serial output after good calibration:
  ✓ Baseline set: 3.2
  Pitch: 3.2° | Deviation: 0.0° | Score: 100 | Posture: ✓ GOOD
```

### Bad Calibration
- Baseline is very high (>15°)
- Sitting straight → deviation still high
- Hard to trigger bad posture detection

```
Serial output after bad calibration:
  ✓ Baseline set: 18.5  ← TOO HIGH!
  Pitch: 18.7° | Deviation: 0.2° | Score: 100 | Posture: ✓ GOOD
  (You're already slouching but it says good!)
```

---

## Gyro Bias During Calibration

### What is Gyro Bias?

The gyroscope has a small constant offset when stationary:
```cpp
Actual rotation: 0°/s
Sensor reads: 0.5°/s (bias error)
```

During calibration, this bias gets "baked in" because:
- The complementary filter trusts the gyro
- Small bias × 300 readings = accumulated error

### Impact on Long-Term Accuracy

```
After 30 seconds of use:
  Accumulated gyro bias error ≈ 0.1° (acceptable)
  
After 5 minutes:
  Accumulated error ≈ 1° (noticeable)
  
After 30 minutes:
  Accumulated error ≈ 5° (significant)
```

### Solution: Gyro Bias Calibration

**Future improvement:**
```cpp
float gyroBiasX = 0.0;

void calibrate_with_bias_correction() {
  float gyroSum = 0.0;
  for (int i = 0; i < 300; i++) {
    // ... read sensor ...
    gyroSum += gxf;  // accumulate gyro readings
  }
  gyroBiasX = gyroSum / 300.0;  // average bias
  
  // Later, subtract this from all gyro readings
  float gxf_corrected = gxf - gyroBiasX;
}
```

---

## Calibration Checklist

Before pressing power:
- [ ] Sensor mounted securely
- [ ] Upper back/neck position selected
- [ ] Wires not pulling on sensor
- [ ] Planning to sit still for 3 seconds

When powering on:
- [ ] Hold posture completely straight
- [ ] Don't move until LED changes state
- [ ] Wait for "Baseline set" message

After calibration:
- [ ] Serial shows reasonable baseline (0-10°)
- [ ] Sitting straight → deviation ≈ 0°
- [ ] Slouching → deviation increases
- [ ] LED/vibrator responds correctly

---

## Summary

| Aspect | Good Calibration | Bad Calibration |
|--------|-----------------|------------------|
| **Duration** | Full 3 seconds | Interrupted |
| **Position** | Perfectly straight | Slouched |
| **Baseline** | 0-10° | >15° |
| **After cal** | Deviation ≈ 0° | Deviation still high |
| **Detection** | Works correctly | Fails to detect bad posture |

**Calibration is the foundation of your posture monitor. Get it right, and everything else works!** ✅
