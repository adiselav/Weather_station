#pragma once
#ifndef CONFIG_H
#define CONFIG_H

// WiFi configuration
const char* WIFI_SSID     = "Andrei’s Home";
const char* WIFI_PASSWORD = "dragos03$";

// Server configuration
const char* SERVER_URL = "https://weather-station-gateway.onrender.com/api/readings";

// API Key for authentication
const char* API_KEY = "api_wxJAMcFVioe4kcuasrdpDIjrQPq1bWcS";

// WiFi reconnect settings
#define WIFI_RECONNECT_TIMEOUT 30000  // 30 seconds timeout
#define WIFI_MAX_RETRIES 5

// HTTP retry settings
#define HTTP_MAX_RETRIES 3
#define HTTP_RETRY_DELAY 2000  // 2 seconds between retries

#endif
