#include "wifi_manager.h"
#include <ESP8266HTTPClient.h>

WiFiClient wifiClient;

// Function to set up WiFi
void setupWiFi(const char* ssid, const char* password) {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// Function to make the HTTP GET request and return the attributes data
JsonArray fetchMetrics(const char* api_url, StaticJsonDocument<2048>& doc) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(wifiClient, api_url);  // Use WiFiClient and URL

    int httpCode = http.GET();  // Make the GET request

    if (httpCode > 0) {
      Serial.printf("HTTP GET request code: %d\n", httpCode);

      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        Serial.println("HTTP response payload:");
        Serial.println(payload);

        // Parse the JSON response into the provided document
        DeserializationError error = deserializeJson(doc, payload);

        if (error) {
          Serial.print("Failed to parse JSON: ");
          Serial.println(error.f_str());
        } else {
          Serial.println("Parsed JSON document:");
          serializeJsonPretty(doc, Serial);

          // Extract and return the metrics array
          if (doc.containsKey("attributes")) {
            return doc["attributes"].as<JsonArray>();
          } else {
            Serial.println("No 'attributes' key found in the JSON response.");
          }
        }
      } else {
        Serial.printf("Unexpected HTTP code: %d\n", httpCode);
      }
    } else {
      Serial.printf("HTTP GET request failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();  // Close the connection
  } else {
    Serial.println("WiFi not connected, cannot make HTTP request.");
  }

  // Return an empty JsonArray if something fails
  return JsonArray();
}