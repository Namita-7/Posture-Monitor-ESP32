#include <Wire.h>
#define MPU_ADDR 0x68
float pitch = 0.0;
float roll = 0.0;
float baseline = 0.0;

void calibrate() {
    float sum = 0.0;
    for (int i = 0; i < 300; i++)
     {
       Wire.beginTransmission(MPU_ADDR);
        Wire.write(0x3B);
        Wire.endTransmission(false);
        Wire.requestFrom(MPU_ADDR, 6, true);
        int16_t ax = Wire.read() << 8 | Wire.read();
        int16_t ay = Wire.read() << 8 | Wire.read();
        int16_t az = Wire.read() << 8 | Wire.read();
        float axf = ax / 16384.0;
        float ayf = ay / 16384.0;
        float azf = az / 16384.0;

        // read gyroscope
        Wire.beginTransmission(MPU_ADDR);
        Wire.write(0x43);
        Wire.endTransmission(false);
        Wire.requestFrom(MPU_ADDR, 6, true);
        int16_t gx = Wire.read() << 8 | Wire.read();
        Wire.read(); Wire.read();
        Wire.read(); Wire.read();
        float gxf = gx / 131.0;

        // update filter
        float accelPitch = atan2(axf, sqrt(ayf*ayf + azf*azf)) * 180.0 / PI;
        float dt = 0.01;
        float alpha = 0.96;
        pitch = alpha * (pitch + gxf * dt) + (1.0 - alpha) * accelPitch;
        sum += pitch;
        delay(10);
    }
    baseline = sum / 300.0;
    Serial.print("Baseline set: ");
    Serial.println(baseline);
}
void setup() {
  Serial.begin(115200);
  Wire.begin(21,22);
  Wire.beginTransmission(0x68);
  Wire.write(0x6B);
  Wire.write(0x00);
  Wire.endTransmission(true);
  Serial.println("MPU6050 ready!");
  calibrate() ;
}

void loop() {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x3B);
    Wire.endTransmission(false);    
    Wire.requestFrom(MPU_ADDR, 6, true);
    int16_t ax = Wire.read() << 8 | Wire.read();
    int16_t ay = Wire.read() << 8 | Wire.read();
    int16_t az = Wire.read() << 8 | Wire.read(); 
    float axf=ax/16384.0;
    float ayf=ay/16384.0;
    float azf=az/16384.0;
    Serial.print("X: "); 
    Serial.print(axf);
    Serial.print(" Y: "); 
    Serial.print(ayf);
    Serial.print(" Z: ");
    Serial.println(azf);
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x43);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_ADDR, 6, true);

    int16_t gx = Wire.read() << 8 | Wire.read();
    int16_t gy = Wire.read() << 8 | Wire.read();
    int16_t gz = Wire.read() << 8 | Wire.read();

    float gxf = gx / 131.0; 
    float gyf = gy / 131.0; 
    float gzf = gz / 131.0; 
    float dt = 0.01;
    float alpha = 0.96;

    float accelPitch = atan2 (axf, sqrt(ayf*ayf + azf*azf)) * 180.0 / PI;
    float accelRoll  = atan2(ayf, sqrt(axf*axf + azf*azf)) * 180.0 / PI;

    pitch = alpha * (pitch + gxf * dt) + (1.0 - alpha) * accelPitch;
    roll  = alpha * (roll  + gyf * dt) + (1.0 - alpha) * accelRoll;
    Serial.print("Pitch: ");
    Serial.print(pitch);
    Serial.print(" Roll: ");
    Serial.println(roll);
    float deviation = pitch - baseline;
    if (deviation > 20.0) {
        Serial.println("BAD POSTURE");
}   else {
        Serial.println("GOOD POSTURE");
}
    delay(10);
}
