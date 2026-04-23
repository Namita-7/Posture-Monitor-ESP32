# Posture Monitor - ESP32
Wearable posture monitor using ESP32, MPU6050, complementary filter, FreeRTOS — no external sensor libraries
## What it does
Detects bad posture in real time using raw I2C sensor fusion.
Alerts via vibration motor. Logs sessions to ESP32 flash memory.
## How it works
The MPU6050 communicates with ESP32 over I2C protocol (no sensor library).
Raw accelerometer and gyroscope register values are read directly.
A complementary filter (α=0.96) fuses both sensors for stable angle estimation.
On startup the system calibrates a baseline pitch over 300 readings.
Any deviation beyond 20° from baseline triggers a bad posture alert.
## Hardware
- ESP32 DevKit V1
- MPU6050 IMU sensor
- Coin vibration motor

## Tech stack
- Raw I2C without sensor libraries
- Complementary filter for angle estimation
- FreeRTOS task scheduling (soon)
- WebServer dashboard (soon)
## Current status
- [x] Raw I2C communication
- [x] Complementary filter
- [x] Baseline calibration
- [x] Posture detection
- [x] FreeRTOS multitasking
- [ ] WebServer dashboard
- [x] SPIFFS logging
- [ ] Real hardware build
