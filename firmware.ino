#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <SensirionI2cScd4x.h>
#include <math.h>

#define SHT21_ADDR            0x40
#define SHT21_CMD_TEMP_NOHOLD 0xF3
#define SHT21_CMD_HUM_NOHOLD  0xF5

const char* WIFI_SSID     = "NUMELE_TAU_DE_WIFI";
const char* WIFI_PASSWORD = "PAROLA_TA";

const char* SERVER_URL    = "http://192.168.1.100:3000/api/readings";

Adafruit_BMP085 bmp;
SensirionI2cScd4x scd4x;

bool bmpOk = false;

uint16_t readSHT21Raw(uint8_t command, uint16_t delayMs) {
  Wire.beginTransmission(SHT21_ADDR);
  Wire.write(command);
  Wire.endTransmission();

  delay(delayMs);

  Wire.requestFrom(SHT21_ADDR, (uint8_t)3);
  if (Wire.available() < 3) {
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
    Serial.println("WiFi nu este conectat, sar trimiterea.");
    return;
  }

  HTTPClient http;
  http.begin(SERVER_URL);
  http.addHeader("Content-Type", "application/json");

  String payload = "{";
  payload += "\"temperature\":" + String(temp, 2) + ",";
  payload += "\"humidity\":"    + String(hum, 2) + ",";
  payload += "\"pressure\":"    + String(press, 2) + ",";
  payload += "\"co2\":"         + String(co2);
  payload += "}";

  Serial.print("Trimit: ");
  Serial.println(payload);

  int httpCode = http.POST(payload);
  Serial.print("HTTP status: ");
  Serial.println(httpCode);

  if (httpCode > 0) {
    String response = http.getString();
    Serial.print("Raspuns: ");
    Serial.println(response);
  }

  http.end();
}

void setup() {
  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Conectare la WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectat.");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  Wire.begin(20, 21);

  bmpOk = bmp.begin();

  scd4x.begin(Wire, 0x62);
  scd4x.stopPeriodicMeasurement();
  delay(500);
  scd4x.startPeriodicMeasurement();
}

void loop() {
  static unsigned long lastMeasure = 0;
  const unsigned long interval = 3600000UL;

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

  Serial.print("Temperatura (C): "); Serial.println(tempFinal);
  Serial.print("Umiditate (%): ");   Serial.println(rhFinal);
  Serial.print("Presiune (hPa): ");  Serial.println(pressFinal);
  Serial.print("CO2 (ppm): ");       Serial.println(co2);
  Serial.println();

  sendToServer(tempFinal, rhFinal, pressFinal, co2);
}
