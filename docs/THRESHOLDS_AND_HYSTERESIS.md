# Thresholds and Hysteresis - Detecting Bad Posture

## The Detection Challenge

We need to answer: **When is posture bad enough to trigger an alert?**

```
Deviation = |Current Pitch - Baseline|

When is deviation "bad"?
  0-10°   → Definitely good
  10-18°  → Still acceptable
  18-22°  → FUZZY ZONE (where problems happen)
  22°+    → Definitely bad
```

---

## Simple Threshold (Without Hysteresis)

### The Naive Approach

```cpp
if (deviation > 20.0) {
  isBadPosture = true;  // Turn on LED/motor
} else {
  isBadPosture = false; // Turn off LED/motor
}
```

### The Problem: Flickering

Imagine your pitch hovers right around 20°:

```
Time | Deviation | Decision   | Result
-----|-----------|------------|--------
0ms  | 20.1°     | > 20° TRUE | LED ON ✓
50ms | 19.9°     | < 20° FALSE| LED OFF
100ms| 20.2°     | > 20° TRUE | LED ON ✓
150ms| 19.8°     | < 20° FALSE| LED OFF
200ms| 20.05°    | > 20° TRUE | LED ON ✓
```

**Result:** LED flickers ON-OFF-ON-OFF rapidly! ❌

### Why This Happens

**Sensor noise:**
- Real measurement might be 20.0° ± 0.3°
- This variance is normal and expected
- Crosses threshold multiple times per second

**Threshold chattering:**
- Even tiny noise causes oscillation
- Motor/LED switching creates more electrical noise
- More noise → more flickering → vicious cycle

---

## Solution: Hysteresis

### What is Hysteresis?

Use **two different thresholds**:
- **Upper threshold:** When to turn ON (higher)
- **Lower threshold:** When to turn OFF (lower)
- Between them: **Keep the current state**

### The Hysteresis Implementation

```cpp
const float THRESHOLD_ON = 22.0;   // Bad posture starts here
const float THRESHOLD_OFF = 18.0;  // Good posture starts here
bool currentState = false;          // Track current state

if (deviation > THRESHOLD_ON) {
  // Clearly bad → turn ON
  currentState = true;
} 
else if (deviation < THRESHOLD_OFF) {
  // Clearly good → turn OFF
  currentState = false;
}
// else: between 18-22, keep currentState unchanged

if (currentState) {
  digitalWrite(LED_PIN, HIGH);    // LED ON
  digitalWrite(MOTOR_PIN, HIGH);  // Motor ON
} else {
  digitalWrite(LED_PIN, LOW);     // LED OFF
  digitalWrite(MOTOR_PIN, LOW);   // Motor OFF
}
```

### Why 18-22°?

```
Lower Threshold (18°):      Upper Threshold (22°):
├─ Noise typically ±1-2°    ├─ Provides 4° hysteresis gap
├─ Catches most slouching   ├─ Prevents false positives
├─ Safe margin below ON     ├─ Clearly bad posture
└─ Recovery feels responsive└─ Requires sustained slouch
```

---

## Hysteresis Behavior

### State Diagram

```
         deviation > 22°
        ╔════════════════╗
        ║    BAD         ║
        ║   POSTURE      ║
        ║   (LED ON)     ║
        ╚════════════════╝
              ▲    ▲
         ┌────┘    └────┐
         │              │
    turn ON          turn OFF
    (22°+)          (<18°)
         │              │
         └────┐    ┌────┘
        ╔════════════════╗
        ║    GOOD        ║
        ║   POSTURE      ║
        ║   (LED OFF)    ║
        ╚════════════════╝
        deviation < 18°
```

### Example Timeline

```
Time | Dev. | State  | Action      | Physical
-----|------|--------|-------------|------------------
0s   | 5°   | GOOD   | LED OFF     | Sitting straight
5s   | 15°  | GOOD   | LED OFF     | Leaning slightly
10s  | 18°  | GOOD   | LED OFF     | Getting worse
15s  | 20°  | GOOD   | LED OFF     | In fuzzy zone
20s  | 23°  | BAD    | LED ON ✓    | Slouching clearly
25s  | 24°  | BAD    | LED ON      | Still slouching
30s  | 22°  | BAD    | LED ON      | Slight improvement
35s  | 20°  | BAD    | LED ON      | In fuzzy zone
40s  | 17°  | GOOD   | LED OFF ✓   | Sit back up
45s  | 5°   | GOOD   | LED OFF     | Good posture again
```

**Notice:** No flickering! State only changes when clearly crossing thresholds.

---

## Hysteresis Gap Size

### Current Gap: 4° (22° - 18°)

```
    THRESHOLD_ON = 22°
         ▲
         │ 4° gap
         ▼
    THRESHOLD_OFF = 18°
```

### Why 4°?

**Too small (1-2°):**
- Still susceptible to noise
- Gap barely larger than sensor noise
- Flickering still possible

```
Noise: ±1.5°
On at: 20°
Off at: 18°
Gap: 2°  ← Noise can still cross both!
```

**Just right (3-5°):** ✓
- Larger than typical sensor noise
- Clear recovery zone
- No flickering with normal movements

```
Noise: ±1.5°
On at: 22°
Off at: 18°
Gap: 4°  ← Noise won't cross both!
```

**Too large (10°+):**
- Takes too long to recover
- User has to straighten WAY up to turn off alarm
- Feels delayed and frustrating

```
On at: 25°
Off at: 15°
Gap: 10° ← Need to improve a lot before alert stops!
```

---

## Threshold Selection

### How We Chose 22°

Typical posture positions:
```
Perfect Upright:      0°  ✓
Normal Working:       5°  ✓
Slightly Slouched:   10°  ✓
Moderately Slouched: 15°  ✓
Clearly Slouching:   20°  ← Getting close
Badly Slouching:     25°  ✓ Definitely alert
```

**22° hits sweet spot:**
- Captures obvious slouching (25°+)
- Doesn't false-alarm on normal variation (15-20°)
- Gives user feedback before extreme posture

### Customizing Thresholds

**For Strict Mode:**
```cpp
const float THRESHOLD_ON = 18.0;   // More sensitive
const float THRESHOLD_OFF = 14.0;  // Gap: 4°
```

**For Lenient Mode:**
```cpp
const float THRESHOLD_ON = 26.0;   // Less sensitive
const float THRESHOLD_OFF = 22.0;  // Gap: 4°
```

**For Very Lenient:**
```cpp
const float THRESHOLD_ON = 30.0;   // Very forgiving
const float THRESHOLD_OFF = 26.0;  // Gap: 4°
```

---

## Hysteresis in Code

### Implementation Details

```cpp
// Global state tracker
bool lastBadPosture = false;  // Remember previous state

void sensorTask(void *pvParameters) {
  for (;;) {
    // ... calculate deviation ...
    
    // Hysteresis logic
    if (deviation > 22.0) {
      isBadPosture = true;      // Needs clear bad posture
    } 
    else if (deviation < 18.0) {
      isBadPosture = false;     // Needs clear good posture
    } 
    else {
      // Between 18-22: keep previous state
      isBadPosture = lastBadPosture;
    }
    
    // Update state for next iteration
    lastBadPosture = isBadPosture;
    
    // Use isBadPosture to control LED/motor...
  }
}
```

### Three-Zone Behavior

```cpp
Zone 1: deviation < 18°
  → Always turn OFF
  → isBadPosture = false
  
Zone 2: 18° ≤ deviation ≤ 22°
  → Keep current state
  → isBadPosture = lastBadPosture
  → No action needed (state doesn't change)
  
Zone 3: deviation > 22°
  → Always turn ON
  → isBadPosture = true
```

---

## Testing Hysteresis

### Manually Test

1. **Start with good posture** (straight)
   - Deviation ≈ 0°
   - LED OFF ✓

2. **Slowly lean forward**
   - Deviation increases: 5°, 10°, 15°, 18°
   - LED OFF (in hysteresis zone)
   - LED OFF (still in zone)

3. **Lean further**
   - Deviation = 22.5°
   - **LED turns ON** ← First transition ✓

4. **Lean back slowly**
   - Deviation = 20°, 19°
   - **LED STAYS ON** (still in zone)

5. **Sit up fully**
   - Deviation = 17.5°
   - **LED turns OFF** ← Second transition ✓

**No flickering at any point!** ✓

---

## Combined with Complementary Filter

### Why Both Matter

**Complementary Filter:**
- Provides smooth, accurate deviation value
- Reduces noise that would cause flickering
- 96% gyro + 4% accel blending

**Hysteresis:**
- Prevents switching on residual noise
- Creates recovery zone
- Makes decisions robust to remaining noise

**Together:**
```
Raw sensor noise → Filtered by complementary filter → Smoothed
                                                        ↓
                                            Hysteresis prevents flickering
                                                        ↓
                                          Stable LED/motor control ✓
```

---

## Summary

| Feature | Without Hysteresis | With Hysteresis |
|---------|-------------------|------------------|
| **Flickering** | Severe | None |
| **Response** | Twitchy | Stable |
| **False positives** | Common | Rare |
| **User experience** | Frustrating | Reliable |
| **Implementation** | Simple | Slightly complex |
| **Recommended** | ❌ No | ✅ Yes |

**Hysteresis is essential for a good user experience!** The 4° gap (18-22°) provides the best balance between responsiveness and stability. ✅
