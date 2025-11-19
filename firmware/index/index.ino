#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <SensirionI2cScd4x.h>
#include <math.h>

#define SHT21_ADDR            0x40
#define SHT21_CMD_TEMP_NOHOLD 0xF3
#define SHT21_CMD_HUM_NOHOLD  0xF5

const char* WIFI_SSID     = "Andrei’s Home";
const char* WIFI_PASSWORD = "dragos03$";

const char* SERVER_URL = "http://192.168.68.52:3000/api/readings";


Adafruit_BMP085 bmp;
SensirionI2cScd4x scd4x;

bool bmpOk = false;

uint16_t readSHT21Raw(uint8_t command, uint16_t delayMs) {
  Wire.beginTransmission(SHT21_ADDR);
  Wire.write(command);
  uint8_t error = Wire.endTransmission();
  if (error != 0) {
    Serial.print("SHT21: Eroare endTransmission: ");
    Serial.println(error);
    return 0xFFFF;
  }

  delay(delayMs);

  uint8_t bytesRead = Wire.requestFrom((uint8_t)SHT21_ADDR, (uint8_t)3);
  if (bytesRead < 3) {
    Serial.print("SHT21: Nu s-au primit destule bytes (primite: ");
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
    Serial.println("WiFi nu este conectat, sar trimiterea.");
    return;
  }

  HTTPClient http;
  http.begin(SERVER_URL);
  http.addHeader("Content-Type", "application/json");

  String payload = "{";
  
  // Temperatură - dacă este nan, trimite 0
  if (isnan(temp)) {
    payload += "\"temperature\":0,";
  } else {
    payload += "\"temperature\":" + String(temp, 2) + ",";
  }
  
  // Umiditate - dacă este nan, trimite 0
  if (isnan(hum)) {
    payload += "\"humidity\":0,";
  } else {
    payload += "\"humidity\":" + String(hum, 2) + ",";
  }
  
  // Presiune - dacă este nan, trimite 0
  if (isnan(press)) {
    payload += "\"pressure\":0,";
  } else {
    payload += "\"pressure\":" + String(press, 2) + ",";
  }
  
  // CO2 - întotdeauna valid (număr)
  payload += "\"co2\":" + String(co2);
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
  delay(100);

  Serial.println("\n=== Initializare senzori ===");
  
  // Test I2C scan
  Serial.println("Scanare bus I2C...");
  byte error, address;
  int nDevices = 0;
  for(address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("Dispozitiv I2C gasit la adresa 0x");
      if (address < 16) Serial.print("0");
      Serial.print(address, HEX);
      Serial.println();
      nDevices++;
    }
  }
  if (nDevices == 0) {
    Serial.println("NU s-au gasit dispozitive I2C!");
  } else {
    Serial.print("Total dispozitive gasite: ");
    Serial.println(nDevices);
  }

  // BMP085
  Serial.print("BMP085: ");
  bmpOk = bmp.begin();
  if (bmpOk) {
    Serial.println("OK");
  } else {
    Serial.println("EROARE - nu s-a putut initializa!");
  }

  // SCD4x
  Serial.print("SCD4x: ");
  scd4x.begin(Wire, 0x62);
  Serial.println("OK");
  uint16_t scdError = scd4x.stopPeriodicMeasurement();
  if (scdError) {
    Serial.print("EROARE la stopPeriodicMeasurement: 0x");
    Serial.println(scdError, HEX);
  }
  delay(500);
  scdError = scd4x.startPeriodicMeasurement();
  if (scdError) {
    Serial.print("EROARE la startPeriodicMeasurement: 0x");
    Serial.println(scdError, HEX);
  } else {
    Serial.println("SCD4x: Măsurători periodice pornite");
    Serial.println("ATENTIE: SCD4x are nevoie de ~5 secunde pentru prima măsurătoare!");
  }
  
  Serial.println("=== Setup complet ===\n");
}

void loop() {
  static unsigned long lastMeasure = 0;
  const unsigned long interval = 10000UL;  // 10 secunde

  unsigned long now = millis();

  if (now - lastMeasure < interval && lastMeasure != 0) {
    delay(1000);
    return;
  }
  lastMeasure = now;

  Serial.println("=== Citire senzori ===");
  
  float shtTemp = readSHT21TemperatureC();
  float shtRH   = readSHT21Humidity();
  Serial.print("SHT21 - Temp: ");
  if (isnan(shtTemp)) Serial.println("NAN");
  else Serial.println(shtTemp);
  Serial.print("SHT21 - RH: ");
  if (isnan(shtRH)) Serial.println("NAN");
  else Serial.println(shtRH);

  float bmpTemp = NAN;
  float bmpPress_hPa = NAN;
  if (bmpOk) {
    bmpTemp = bmp.readTemperature();
    bmpPress_hPa = bmp.readPressure() / 100.0;
    Serial.print("BMP085 - Temp: ");
    if (isnan(bmpTemp)) Serial.println("NAN");
    else Serial.println(bmpTemp);
    Serial.print("BMP085 - Presiune: ");
    if (isnan(bmpPress_hPa)) Serial.println("NAN");
    else Serial.println(bmpPress_hPa);
  } else {
    Serial.println("BMP085: Nu este initializat");
  }

  uint16_t co2 = 0;
  float scdTemp = NAN;
  float scdRH   = NAN;

  uint16_t error = scd4x.readMeasurement(co2, scdTemp, scdRH);
  if (error) {
    Serial.print("SCD4x: Eroare la citire: 0x");
    Serial.println(error, HEX);
    co2 = 0;
    scdTemp = NAN;
    scdRH = NAN;
  } else if (co2 == 0xFFFF) {
    Serial.println("SCD4x: CO2 invalid (0xFFFF)");
    co2 = 0;
    scdTemp = NAN;
    scdRH = NAN;
  } else {
    Serial.print("SCD4x - Temp: ");
    if (isnan(scdTemp)) Serial.println("NAN");
    else Serial.println(scdTemp);
    Serial.print("SCD4x - RH: ");
    if (isnan(scdRH)) Serial.println("NAN");
    else Serial.println(scdRH);
    Serial.print("SCD4x - CO2: ");
    Serial.println(co2);
  }

  float tempFinal  = avg3(shtTemp, bmpTemp, scdTemp);
  float rhFinal    = avg2(shtRH, scdRH);
  float pressFinal = bmpPress_hPa;

  Serial.println();
  Serial.print("Temperatura (C): "); Serial.println(tempFinal);
  Serial.print("Umiditate (%): ");   Serial.println(rhFinal);
  Serial.print("Presiune (hPa): ");  Serial.println(pressFinal);
  Serial.print("CO2 (ppm): ");       Serial.println(co2);
  
  Serial.println();
  sendToServer(tempFinal, rhFinal, pressFinal, co2);
}
