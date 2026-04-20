#include <Wire.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <WebServer.h>
#define MPU_ADDR 0x68

WebServer server(80);      
const char* ssid = "Wokwi-GUEST";
const char* password = "";
float pitch = 0.0;
float roll = 0.0;
float baseline = 0.0;
int score = 100;
unsigned long lastLog = 0;
void handleRoot() {
  String html = "<html><body>";
  html += "<h1>Posture: " + String(pitch) + "°</h1>";
  html += "<p>Score: " + String(score) + "</p>";
  html += (abs(pitch-baseline)>20) ? "<p style='color:red'>FIX POSTURE</p>" : "<p style='color:green'>Good</p>";
  html += "</body></html>";
 
  server.send(200, "text/html", html);  
}


void logData() {
    File f = SPIFFS.open("/posture_log.csv", FILE_APPEND);
    if (!f) {
        Serial.println("Failed to open file");
        return;
    }
    f.print(millis());      f.print(",");
    f.print(pitch);         f.print(",");
    f.print(pitch - baseline); f.print(",");
    f.println(score);
    f.close();
}
void calibrate() {
    float sum = 0.0;
    for (int i = 0; i < 300; i++) {
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
        Wire.beginTransmission(MPU_ADDR);
        Wire.write(0x43);
        Wire.endTransmission(false);
        Wire.requestFrom(MPU_ADDR, 6, true);
        int16_t gx = Wire.read() << 8 | Wire.read();
        Wire.read(); Wire.read();
        Wire.read(); Wire.read();
        float gxf = gx / 131.0;
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
    Wire.begin(21, 22);
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x6B);
    Wire.write(0x00);
    Wire.endTransmission(true);
    Serial.println("MPU6050 ready!");
    if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS failed!");
    return;
}
    Serial.println("SPIFFS ready!");
    calibrate();
     WiFi.begin(ssid,password);
  while(WiFi.status()!=WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connect to: http://");
  Serial.println(WiFi.localIP());
  server.on("/", handleRoot);  
  server.begin();              

}
   


void loop() {
    server.handleClient();
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
    float accelPitch = atan2(axf, sqrt(ayf*ayf + azf*azf)) * 180.0 / PI;
    float accelRoll  = atan2(ayf, sqrt(axf*axf + azf*azf)) * 180.0 / PI;


    pitch = alpha * (pitch + gxf * dt) + (1.0 - alpha) * accelPitch;
    roll  = alpha * (roll  + gyf * dt) + (1.0 - alpha) * accelRoll;


    Serial.print("Pitch: "); Serial.print(pitch);
    Serial.print(" Roll: "); Serial.println(roll);


    float deviation = abs(pitch - baseline);


    if (deviation > 20.0) {
        score -= 2;
        if (score < 0) score = 0;
        Serial.println("BAD POSTURE");
    } else {
        score += 1;
        if (score > 100) score = 100;
        Serial.println("GOOD POSTURE");
    }


    Serial.print("Score: ");
    Serial.println(score);
    delay(10);
    if (millis() - lastLog > 30000) {
    logData();
    lastLog = millis();
    Serial.println("Logged to SPIFFS");
}
}
