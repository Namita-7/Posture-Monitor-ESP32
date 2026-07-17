# Hardware Setup - Connections and Component Guide

## Component List

| Component | Model | Quantity | Purpose |
|-----------|-------|----------|----------|
| Microcontroller | ESP32 DevKit | 1 | Main controller |
| IMU Sensor | MPU6050 | 1 | Accelerometer + Gyroscope |
| Vibration Motor | 3-5V Vibrator | 1 | Posture alert feedback |
| LED | Red (5mm) | 1 | Visual posture indicator |
| NPN Transistor | 2N2222 or similar | 2 | Motor/LED driver |
| Flyback Diode | 1N4007 | 1 | Protects against motor spikes |
| Resistor | 10kΩ | 2 | Base resistance for transistors |
| Resistor | 220Ω | 1 | LED current limiting |
| Jumper Wires | Female-Female | 15+ | Connections |
| Breadboard | Standard | 1 | Prototyping |

---

## ESP32 Pinout

```
╔════════════════════════════════════╗
║           ESP32 DevKit             ║
╠════════════════════════════════════╣
║  GND  ┌─────────────────────────┐  ║
║  ┃    │                         │  ║
║ 3V3  ─┤ I2C Power              ├─ ┌── D1(GPIO5)
║ EN   ┌┤                         ├─ ├── D2(GPIO4)
║ SVP  │├─ I2C/SPI Zone         ├─ ├── D3(GPIO0)
║ SVN  ││                         ││ ├── D4(GPIO2)
║ GND  ││ 21(GPIO21) - SDA      ││ ├── Tx(GPIO1)
║  3V3 ││ 22(GPIO22) - SCL      ││ ├── Rx(GPIO3)
║      └┤                         ├─ ├── D5(GPIO5)
║ GND  ┌─┤ Motor/LED Zone        ├─ ├── D6(GPIO18)
║      │├─ 13(GPIO13) - Motor   ├─ ├── D7(GPIO19)
║      │├─ 14(GPIO14) - LED     ├─ ├── D8(GPIO21)
║      │└─ GND (multiple)        ├─ ├── D9(GPIO22)
║      └─────────────────────────┘  └── GND
╚════════════════════════════════════╝
```

---

## MPU6050 Connections

### MPU6050 Pinout

```
┌─────────────┐
│   MPU6050   │
│  (Top View) │
├─────────────┤
│ VCC → 3.3V  │  Power supply
│ GND → GND   │  Ground
│ SDA → GPIO21│  I2C Data
│ SCL → GPIO22│  I2C Clock
└─────────────┘
```

### Wiring Table

| MPU6050 Pin | ESP32 Pin | Color | Signal |
|-------------|-----------|-------|--------|
| VCC | 3V3 | Red | Power |
| GND | GND | Black | Ground |
| SDA | GPIO 21 | Yellow | I2C Data |
| SCL | GPIO 22 | Blue | I2C Clock |

### Breadboard Layout

```
     ┌─────────────────────┐
     │   Breadboard        │
     ├─────────────────────┤
     │ + ─ ─ ─ ─ ─ ─ ─ ─  │
     │ a b c d e f g h i j │
     │                     │
  1  │ RED(+) ─ ─ ─ ─ ─ ─ │ ← VCC from MPU
  2  │  │  BLK(−) ─ ─ ─ ─ │ ← GND
  3  │  │   │  YEL(SDA) ─ │ ← SDA from GPIO21
  4  │  │   │   │  BLU(SCL)│ ← SCL from GPIO22
     │                     │
     └─────────────────────┘
```

### I2C Address

```cpp
#define MPU_ADDR 0x68  // Default I2C address

Wire.begin(21, 22);  // Initialize I2C on GPIO 21 & 22
```

**Note:** Some MPU6050 boards have an AD0 pin that can change address to 0x69. Our code uses 0x68.

---

## Motor Circuit (Vibration Feedback)

### Circuit Diagram

```
         ┌──── 5V (or 3.3V)
         │
         │  [Vibration Motor]
         │        │
         └────────┼─────────────┐
                  │             │
              1N4007 Diode   100Ω Resistor
              (flyback)       (current limit)
                  │             │
                  └────────┬─────┘
                           │
                        ┌──┴──┐
                        │  E  │ NPN Transistor
                  GPIO13├──B  │  (2N2222)
        (from ESP32)    │  C  │
                        └──┬──┘
                           │
                          GND
```

### Motor Wiring

| Component | Pin/Connection | Notes |
|-----------|-----------------|-------|
| Motor (+) | 5V power rail | Positive terminal |
| Motor (-) | Through diode → Transistor C | Negative goes through diode first |
| Diode (+) | Motor (-) | Silver band (cathode) at motor |
| Diode (-) | Transistor C | Non-silver band (anode) at collector |
| Transistor B | GPIO 13 (via 10kΩ) | Base current control |
| Transistor E | GND | Emitter to ground |

### Flyback Diode Purpose

**Without diode:**
```
When motor turns OFF:
  Motor has stored energy (inductance)
  Releases as voltage spike (up to 100V!)
  Damages transistor and corrupts ESP32
```

**With diode:**
```
Diode provides alternate path for energy:
  Energy dissipates harmlessly in diode
  Transistor protected ✓
  No voltage spikes ✓
```

### Check Diode Orientation

```
Correct (Silver band at motor):        Wrong (Silver at transistor):
┌─────────┐                           ┌─────────┐
│ Motor   │                           │ Motor   │
│  (+)    │                           │  (+)    │
└────●────┘ ← Silver band toward     └─────●───┘
     │      motor (at anode)             │
     │                                   │
```

---

## LED Circuit (Visual Indicator)

### Circuit Diagram

```
         ┌────── 3.3V
         │
      220Ω Resistor
         │
       RED LED (+)
         │
       LED (−)
         │
      GPIO14
    (from ESP32)
         │
        GND
```

### LED Wiring

| Component | Pin/Connection | Notes |
|-----------|-----------------|-------|
| LED (+) | Through 220Ω → GPIO 14 | Long leg (positive) |
| LED (−) | GND | Short leg (negative) |
| Resistor | 220Ω between GPIO 14 and LED | Limits current to ~10mA |

### LED Brightness

**Resistor value affects brightness:**

```
220Ω  → Normal brightness ✓
100Ω  → Very bright (might draw too much)
470Ω  → Dim
1kΩ   → Very dim
```

**Use 220Ω for good visibility without overloading GPIO.**

### GPIO Pin Configuration

```cpp
#define LED_PIN 14
#define MOTOR_PIN 13

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(MOTOR_PIN, OUTPUT);
  
  // Start with both OFF (safe state)
  digitalWrite(LED_PIN, LOW);    // LED off
  digitalWrite(MOTOR_PIN, LOW);  // Motor off
}
```

---

## Complete System Diagram

```
┌──────────────────────────────────────────────────┐
│                 ESP32                            │
│  ┌────────────────────────────────────────────┐  │
│  │ Task Execution                             │  │
│  │ - sensorTask (GPIO 21/22 I2C)             │  │
│  │ - motorTask (GPIO 13 control)             │  │
│  │ - WebServer (WiFi)                        │  │
│  │ - loggerTask (SPIFFS)                     │  │
│  └────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────┘
       │ I2C (21/22)        │ Output (13)    │ Output (14)
       │                    │                │
       ▼                    ▼                ▼
  ┌─────────────┐      ┌──────────┐     ┌────────┐
  │  MPU6050    │      │ 2N2222   │     │  LED   │
  │             │      │Transistor│     │  220Ω  │
  │ Accel+Gyro  │      │  + Diode │     │ (Red)  │
  │             │      │          │     │        │
  │ • SDA ──────┼──┐   │          │     │        │
  │ • SCL ──────┼──┤   │          │     │        │
  │ • VCC ──┐   │  │   │          │     │        │
  │ • GND ──┼───┼──┼───┼──────────┼─────┼────────┼── GND
  └─────────┼───┘  │   │          │     │        │
            │      │   └──────┬───┘     └────────┘
            │      │      (Motor control)
            │      │          │
           3.3V    │     ┌──────────┐
                   │     │  Motor   │
                   │     │(5V, 3.3V)│
                   │     └──────────┘
                   └── Base resistor (10kΩ)
```

---

## Power Considerations

### Voltage Levels

```
MPU6050:       3.3V (I2C logic levels)
LED:           3.3V GPIO output
Motor:         3.3V or 5V (if transistor sourced from 5V)
Transistor:    NPN type (base-emitter on GPIO)
```

### Current Draw

```
MPU6050:    ~3-4mA
LED:        ~10mA
Motor:      ~100-200mA (peak)
ESP32:      ~50-100mA
─────────────────────
Total:      ~200-400mA (when all active)
```

**Recommendation:** Use 5V/1A USB power supply for stability.

### Ground Connections

```
Critical: All components must share common GND!

┌─────────┐
│  Power  │
│  +5V    │
└────┬────┘
     │ ┌───────────────────┐
     ├─┤ Motor             │
     │ └───────────────────┘
     │ ┌───────────────────┐
     ├─┤ ESP32 VCC (3.3V)  │
     │ └───────────────────┘
     │ ┌───────────────────┐
     ├─┤ MPU6050 VCC       │
     │ └───────────────────┘
     │
  ┌──┴──────────────────────────┐
  │ GND (MUST be common for all)│
  └─────────────────────────────┘
```

---

## Troubleshooting

### MPU6050 Not Communicating

**Check:**
- ✓ SDA/SCL on correct GPIO (21/22)
- ✓ I2C address in code (0x68)
- ✓ VCC at 3.3V
- ✓ GND connected
- ✓ Pull-up resistors (some boards have them built-in)

### Motor Not Vibrating

**Check:**
- ✓ Diode orientation (silver band toward motor)
- ✓ Transistor base to GPIO 13 (not floating)
- ✓ Motor powered (not ground)
- ✓ Motor connections not reversed

### LED Not Lighting

**Check:**
- ✓ LED polarity (long leg +, short leg −)
- ✓ 220Ω resistor in series
- ✓ GPIO 14 connected
- ✓ GND connected

### Flickering/Glitching

**Check:**
- ✓ Flyback diode across motor (prevents spikes)
- ✓ All GND connections solid
- ✓ 50ms loop delay in code (synchronizes with complementary filter)
- ✓ Hysteresis enabled (18-22° zone)

---

## Summary

| Connection | ESP32 Pin | Component | Purpose |
|------------|-----------|-----------|----------|
| I2C Data | GPIO 21 | MPU6050 SDA | Sensor communication |
| I2C Clock | GPIO 22 | MPU6050 SCL | Sensor communication |
| Motor Control | GPIO 13 | 2N2222 Base | Vibration on/off |
| LED Control | GPIO 14 | Red LED | Visual indicator |
| Power | 3V3 | All (via regulator) | Logic supply |
| Ground | GND | All components | Common reference |

**All connections are now properly configured for accurate posture detection!** ✅
