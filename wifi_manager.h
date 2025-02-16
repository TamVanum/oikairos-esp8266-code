#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <ESP8266WiFi.h>
#include <ArduinoJson.h>

// Functions
void setupWiFi(const char* ssid, const char* password);
JsonArray fetchMetrics(const char* api_url, StaticJsonDocument<2048>& doc);


#endif
