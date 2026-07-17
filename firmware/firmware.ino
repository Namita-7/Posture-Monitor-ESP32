#include <Wire.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <WebServer.h>

#define MPU_ADDR 0x68
#define MOTOR_PIN 13
#define LED_PIN 14

WebServer server(80);
const char *ssid = "Babu Lakpoti";
const char *password = "9448570120";
volatile float pitch = 0.0;
volatile float roll = 0.0;
float baseline = 0.0;
volatile int score = 100;
volatile bool badPosture = false;
SemaphoreHandle_t dataMutex;

void handleRoot()
{
  if (xSemaphoreTake(dataMutex, 10) == pdTRUE)
  {
    float localPitch = pitch;
    float localBaseline = baseline;
    int localScore = score;
    xSemaphoreGive(dataMutex);

    String html = "<!DOCTYPE html><html><head>";
    html += "<meta http-equiv='refresh' content='1'>";
    html += "<title>Posture Monitor</title>";
    html += "<style>body{font-family:sans-serif;text-align:center;background:#1e1e2e;color:white;}";
    html += ".good{color:#50fa7b;font-size:40px;}.bad{color:#ff5555;font-size:40px;}";
    html += ".score{font-size:60px;font-weight:bold;}</style></head><body>";
    html += "<h1>Posture Monitor</h1>";
    html += "<p>Pitch: " + String(localPitch, 1) + "&deg;</p>";
    html += "<p>Deviation: " + String(localPitch - localBaseline, 1) + "&deg;</p>";
    
    if (abs(localPitch - localBaseline) > 20.0)
    {
      html += "<p class='bad'>BAD POSTURE</p>";
    }
    else
    {
      html += "<p class='good'>GOOD POSTURE</p>";
    }
    
    html += "<p class='score'>Score: " + String(localScore) + "</p>";
    html += "</body></html>";
    
    server.send(200, "text/html", html);
  }
  else
  {
    server.send(503, "text/plain", "Busy");
  }
}

void calibrate()
{
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

    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x43);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_ADDR, 6, true);
    int16_t gx = Wire.read() << 8 | Wire.read();
    Wire.read();
    Wire.read();
    Wire.read();
    Wire.read();
    float gxf = gx / 131.0;
    float accelPitch = atan2(axf, sqrt(ayf * ayf + azf * azf)) * 180.0 / PI;
    float dt = 0.01;
    float alpha = 0.96;
    pitch = alpha * (pitch + gxf * dt) + (1.0 - alpha) * accelPitch;
    sum += pitch;
    delay(10);
  }
  baseline = sum / 300.0;
  Serial.print("✓ Baseline set: ");
  Serial.println(baseline);
}

void sensorTask(void *pvParameters)
{
  float dt = 0.05;  // 50ms - matches vTaskDelay
  float alpha = 0.96;
  bool lastBadPosture = false;  // for hysteresis
  
  for (;;)
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

    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x43);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_ADDR, 6, true);
    int16_t gx = Wire.read() << 8 | Wire.read();
    int16_t gy = Wire.read() << 8 | Wire.read();
    int16_t gz = Wire.read() << 8 | Wire.read();
    float gxf = gx / 131.0;
    float gyf = gy / 131.0;

    float accelPitch = atan2(axf, sqrt(ayf * ayf + azf * azf)) * 180.0 / PI;
    float accelRoll = atan2(ayf, sqrt(axf * axf + azf * azf)) * 180.0 / PI;

    float localPitch = 0.0;
    float localRoll = 0.0;
    int localScore = 0;
    bool isBadPosture = false;

    if (xSemaphoreTake(dataMutex, 10) == pdTRUE)
    {
      pitch = alpha * (pitch + gxf * dt) + (1.0 - alpha) * accelPitch;
      roll = alpha * (roll + gyf * dt) + (1.0 - alpha) * accelRoll;
      float deviation = abs(pitch - baseline);
      
      // Hysteresis to prevent flickering
      if (deviation > 22.0)  // needs CLEARLY bad to trigger
      {
        isBadPosture = true;
      }
      else if (deviation < 18.0)  // needs CLEARLY good to stop
      {
        isBadPosture = false;
      }
      else  // between 18-22, keep previous state
      {
        isBadPosture = lastBadPosture;
      }
      
      if (isBadPosture)
      {
        score -= 2;
        if (score < 0) score = 0;
      }
      else
      {
        score += 1;
        if (score > 100) score = 100;
      }
      
      localPitch = pitch;
      localRoll = roll;
      localScore = score;
      badPosture = isBadPosture;
      lastBadPosture = isBadPosture;
      xSemaphoreGive(dataMutex);
    }

    Serial.print("Pitch: ");
    Serial.print(localPitch, 1);
    Serial.print("° | Deviation: ");
    Serial.print(abs(localPitch - baseline), 1);
    Serial.print("° | Score: ");
    Serial.print(localScore);
    Serial.print(" | Posture: ");
    Serial.println(isBadPosture ? "❌ BAD" : "✓ GOOD");

    vTaskDelay(50 / portTICK_PERIOD_MS);  // 50ms - matches dt
  }
}

void motorTask(void *pvParameters)
{
  for (;;)
  {
    if (xSemaphoreTake(dataMutex, 10) == pdTRUE)
    {
      bool isBadPosture = badPosture;
      xSemaphoreGive(dataMutex);
      
      if (isBadPosture)
      {
        // Bad posture: Turn ON vibrator and RED LED
        digitalWrite(MOTOR_PIN, HIGH);
        digitalWrite(LED_PIN, HIGH);
      }
      else
      {
        // Good posture: Turn OFF vibrator and RED LED
        digitalWrite(MOTOR_PIN, LOW);
        digitalWrite(LED_PIN, LOW);
      }
    }
    
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

void serverTask(void *pvParameters)
{
  for (;;)
  {
    server.handleClient();
    vTaskDelay(5 / portTICK_PERIOD_MS);
  }
}

void loggerTask(void *pvParameters)
{
  for (;;)
  {
    vTaskDelay(30000 / portTICK_PERIOD_MS);
    float localPitch = 0.0;
    int localScore = 0;
    if (xSemaphoreTake(dataMutex, 100) == pdTRUE)
    {
      localPitch = pitch;
      localScore = score;
      xSemaphoreGive(dataMutex);
    }
    File f = SPIFFS.open("/posture_log.csv", FILE_APPEND);
    if (f)
    {
      f.print(millis());
      f.print(",");
      f.print(localPitch);
      f.print(",");
      f.print(localPitch - baseline);
      f.print(",");
      f.println(localScore);
      f.close();
      Serial.println("📝 Logged to SPIFFS");
    }
  }
}

void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n=== POSTURE MONITOR STARTING ===\n");
  
  // Motor pin setup
  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW);  // Start with motor OFF
  Serial.println("✓ Motor pin initialized on GPIO 13");
  
  // LED pin setup
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);  // Start with LED OFF
  Serial.println("✓ RED LED pin initialized on GPIO 14");
  
  Wire.begin(21, 22);
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0x00);
  Wire.endTransmission(true);
  Serial.println("✓ MPU6050 ready!");
  
  if (!SPIFFS.begin(true))
  {
    Serial.println("✗ SPIFFS failed! Continuing without logging...");
  }
  else
  {
    Serial.println("✓ SPIFFS ready!");
  }
  
  Serial.println("\n--- Calibrating (hold STRAIGHT for 3 seconds) ---");
  calibrate();
  
  Serial.println("\n--- Connecting to WiFi ---");
  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20)
  {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println("");
  
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("\n✓ WiFi Connected!");
    Serial.print("📡 Server IP: http://");
    Serial.println(WiFi.localIP());
    Serial.println("   (Open this in your browser)\n");
  }
  else
  {
    Serial.println("\n✗ WiFi Connection Failed\n");
  }
  
  server.on("/", handleRoot);
  server.begin();
  
  Serial.println("=== SYSTEM READY ===\n");
  
  dataMutex = xSemaphoreCreateMutex();
  xTaskCreate(sensorTask, "Sensor", 4096, NULL, 3, NULL);
  xTaskCreate(motorTask, "Motor", 2048, NULL, 2, NULL);
  xTaskCreate(serverTask, "Server", 4096, NULL, 2, NULL);
  xTaskCreate(loggerTask, "Logger", 2048, NULL, 1, NULL);
}

void loop()
{
}
