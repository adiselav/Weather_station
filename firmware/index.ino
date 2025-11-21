#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <SensirionI2cScd4x.h>
#include <math.h>
#include "config.h"

#define SHT21_ADDR            0x40
#define SHT21_CMD_TEMP_NOHOLD 0xF3
#define SHT21_CMD_HUM_NOHOLD  0xF5


Adafruit_BMP085 bmp;
SensirionI2cScd4x scd4x;

bool bmpOk = false;

uint16_t readSHT21Raw(uint8_t command, uint16_t delayMs) {
  Wire.beginTransmission(SHT21_ADDR);
  Wire.write(command);
  uint8_t error = Wire.endTransmission();
  if (error != 0) {
    Serial.print("SHT21: endTransmission error: ");
    Serial.println(error);
    return 0xFFFF;
  }

  delay(delayMs);

  uint8_t bytesRead = Wire.requestFrom((uint8_t)SHT21_ADDR, (uint8_t)3);
  if (bytesRead < 3) {
    Serial.print("SHT21: Not enough bytes received (received: ");
    Serial.print(bytesRead);
    Serial.println(")");
    return 0xFFFF;
  }

  uint8_t msb = Wire.read();
  uint8_t lsb = Wire.read();
  uint8_t crc = Wire.read();
  (void)crc;

  uint16_t raw = ((uint16_t)msb << 8) | lsb;
  raw &= ~0x0003;
  return raw;
}

float readSHT21TemperatureC() {
  uint16_t raw = readSHT21Raw(SHT21_CMD_TEMP_NOHOLD, 85);
  if (raw == 0xFFFF) return NAN;
  return -46.85 + 175.72 * (float)raw / 65536.0;
}

float readSHT21Humidity() {
  uint16_t raw = readSHT21Raw(SHT21_CMD_HUM_NOHOLD, 29);
  if (raw == 0xFFFF) return NAN;
  float rh = -6.0 + 125.0 * (float)raw / 65536.0;
  if (rh < 0)   rh = 0;
  if (rh > 100) rh = 100;
  return rh;
}

float avg3(float a, float b, float c) {
  float sum = 0;
  int count = 0;
  if (!isnan(a)) { sum += a; count++; }
  if (!isnan(b)) { sum += b; count++; }
  if (!isnan(c)) { sum += c; count++; }
  if (count == 0) return NAN;
  return sum / count;
}

float avg2(float a, float b) {
  float sum = 0;
  int count = 0;
  if (!isnan(a)) { sum += a; count++; }
  if (!isnan(b)) { sum += b; count++; }
  if (count == 0) return NAN;
  return sum / count;
}

void sendToServer(float temp, float hum, float press, uint16_t co2) {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  HTTPClient http;
  http.setConnectTimeout(5000);
  http.setTimeout(10000);
  
  if (!http.begin(SERVER_URL)) {
    return;
  }
  
  http.addHeader("Content-Type", "application/json");

  String payload = "{";
  
  if (isnan(temp)) {
    payload += "\"temperature\":0,";
  } else {
    payload += "\"temperature\":" + String(temp, 2) + ",";
  }
  
  if (isnan(hum)) {
    payload += "\"humidity\":0,";
  } else {
    payload += "\"humidity\":" + String(hum, 2) + ",";
  }
  
  if (isnan(press)) {
    payload += "\"pressure\":0,";
  } else {
    payload += "\"pressure\":" + String(press, 2) + ",";
  }
  
  payload += "\"co2\":" + String(co2);
  payload += "}";

  http.POST(payload);
  http.end();
}

void setup() {
  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  Wire.begin(3, 4);
  delay(100);

  bmpOk = bmp.begin();
  scd4x.begin(Wire, 0x62);
  scd4x.stopPeriodicMeasurement();
  delay(500);
  scd4x.startPeriodicMeasurement();
}

void loop() {
  static unsigned long lastMeasure = 0;
  const unsigned long interval = 3600000UL;  // 1 hour

  unsigned long now = millis();

  if (now - lastMeasure < interval && lastMeasure != 0) {
    delay(1000);
    return;
  }
  lastMeasure = now;

  float shtTemp = readSHT21TemperatureC();
  float shtRH   = readSHT21Humidity();

  float bmpTemp = NAN;
  float bmpPress_hPa = NAN;
  if (bmpOk) {
    bmpTemp = bmp.readTemperature();
    bmpPress_hPa = bmp.readPressure() / 100.0;
  }

  uint16_t co2 = 0;
  float scdTemp = NAN;
  float scdRH   = NAN;

  uint16_t error = scd4x.readMeasurement(co2, scdTemp, scdRH);
  if (error || co2 == 0xFFFF) {
    co2 = 0;
    scdTemp = NAN;
    scdRH = NAN;
  }

  float tempFinal  = avg3(shtTemp, bmpTemp, scdTemp);
  float rhFinal    = avg2(shtRH, scdRH);
  float pressFinal = bmpPress_hPa;

  sendToServer(tempFinal, rhFinal, pressFinal, co2);
}
