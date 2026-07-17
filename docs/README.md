# Posture Monitor - Technical Documentation

## Overview
This folder contains comprehensive documentation about how the Posture Monitor ESP32 system works, including sensor calculations, hardware setup, and software architecture.

## Contents

- **ANGLE_CALCULATIONS.md** - Detailed explanation of how pitch angles are calculated from MPU6050 sensor data
- **HARDWARE_SETUP.md** - Component connections, pinouts, and circuit diagram information
- **COMPLEMENTARY_FILTER.md** - How the gyroscope and accelerometer data is fused for accurate angle measurement
- **CALIBRATION.md** - Baseline calibration process and how it works
- **THRESHOLDS_AND_HYSTERESIS.md** - Posture detection logic and flickering prevention

## Quick Start

1. **Understanding the Sensor:** Start with [ANGLE_CALCULATIONS.md](ANGLE_CALCULATIONS.md)
2. **Building the Hardware:** See [HARDWARE_SETUP.md](HARDWARE_SETUP.md)
3. **How Detection Works:** Read [THRESHOLDS_AND_HYSTERESIS.md](THRESHOLDS_AND_HYSTERESIS.md)

## System Architecture

```
ESP32 + MPU6050 → Angle Calculation → Baseline Comparison → Decision Logic → Output (LED/Motor)
```

For detailed information, refer to the individual documentation files.
