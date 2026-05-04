#include <Wire.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <WebServer.h>
#define MPU_ADDR 0x68

WebServer server(80);
const char *ssid = "Babu Lakpoti";
const char *password = "9448570120";
volatile float pitch = 0.0;
volatile float roll = 0.0;
float baseline = 0.0;
volatile int score = 100;
unsigned long lastLog = 0;

SemaphoreHandle_t dataMutex;

void handleRoot()
{
  if (xSemaphoreTake(dataMutex, 10) == pdTRUE)
  {

    String html = "<!DOCTYPE html><html><head>";
    html += "<meta http-equiv='refresh' content='1'>";
    html += "<title>Posture Monitor</title>";
    html += "<style>body{font-family:sans-serif;text-align:center;background:#1e1e2e;color:white;}";
    html += ".good{color:#50fa7b;font-size:40px;}.bad{color:#ff5555;font-size:40px;}";
    html += ".score{font-size:60px;font-weight:bold;}</style></head><body>";
    html += "<h1>Posture Monitor</h1>";
    html += "<p>Pitch: " + String(pitch, 1) + "&deg;</p>";
    html += "<p>Deviation: " + String(pitch - baseline, 1) + "&deg;</p>";
    if (abs(pitch - baseline) > 20.0)
    {
      html += "<p class='bad'>BAD POSTURE</p>";
    }
    else
    {
      html += "<p class='good'>GOOD POSTURE</p>";
    }
    html += "<p class='score'>Score: " + String(score) + "</p>";
    html += "</body></html>";
    xSemaphoreGive(dataMutex);
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
  Serial.print("Baseline set: ");
  Serial.println(baseline);
}
void sensorTask(void *pvParameters)
{
  float dt = 0.01;
  float alpha = 0.96;
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

    if (xSemaphoreTake(dataMutex, 10) == pdTRUE)
    {
      pitch = alpha * (pitch + gxf * dt) + (1.0 - alpha) * accelPitch;
      roll = alpha * (roll + gyf * dt) + (1.0 - alpha) * accelRoll;
      float deviation = abs(pitch - baseline);
      if (deviation > 20.0)
      {
        score -= 2;
        if (score < 0)
          score = 0;
      }
      else
      {
        score += 1;
        if (score > 100)
          score = 100;
      }
      localPitch = pitch;
      localRoll = roll;
      localScore = score;
      xSemaphoreGive(dataMutex);
    }
    Serial.print("Pitch: ");
    Serial.print(localPitch);
    Serial.print(" Roll: ");
    Serial.print(localRoll);
    Serial.print(" Score: ");
    Serial.println(localScore);
    vTaskDelay(10 / portTICK_PERIOD_MS);
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
      Serial.println("Logged to SPIFFS");
    }
  }
}

void setup()
{
  Serial.begin(115200);
  Wire.begin(21, 22);
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0x00);
  Wire.endTransmission(true);
  Serial.println("MPU6050 ready!");
  if (!SPIFFS.begin(true))
  {
    Serial.println("SPIFFS failed! Continuing without logging...");
  }
  else
  {
    Serial.println("SPIFFS ready!");
  }
  calibrate();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connect to: http://");
  Serial.println(WiFi.localIP());
  server.on("/", handleRoot);
  server.begin();
  dataMutex = xSemaphoreCreateMutex();
  xTaskCreate(sensorTask, "Sensor", 4096, NULL, 3, NULL);
  xTaskCreate(serverTask, "Server", 4096, NULL, 2, NULL);
  xTaskCreate(loggerTask, "Logger", 2048, NULL, 1, NULL);
}

void loop()
{
}
