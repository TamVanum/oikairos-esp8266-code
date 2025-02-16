#include "data_formatter.h"

// Format real sensor data into JSON
StaticJsonDocument<200> formatSensorData(float pH, float tds, float waterTemp, float ambientTemp, bool water_pump, bool water_flow) {
  StaticJsonDocument<200> doc;
  doc["ph_level"] = pH;
  doc["electrical_conductivity"] = tds;
  doc["water_temperature"] = waterTemp;
  doc["ambient_temperature"] = ambientTemp;
  doc["water_pump"] = water_pump;
  doc["water_flow"] = water_flow;
  return doc;
}

// Generate default test data for testing
StaticJsonDocument<200> generateDefaultTestData() {
  StaticJsonDocument<200> doc;
  doc["ph_level"] = random(65, 75) / 10.0;                     // Random pH value between 6.5 and 7.5
  doc["electrical_conductivity"] = random(100, 500);                         // Random TDS value between 100 and 500 ppm
  doc["water_temperature"] = random(200, 250) / 10.0;    // Random water temp between 20.0 and 25.0°C
  doc["ambient_temperature"] = random(150, 300) / 10.0;  // Random ambient temp between 15.0 and 30.0°C
  doc["water_pump"] = true;
  doc["water_flow"] = true;
  return doc;
}

// Serialize JSON data to a String
String serializeData(const StaticJsonDocument<200>& doc) {
  String jsonBuffer;
  serializeJson(doc, jsonBuffer);
  return jsonBuffer;
}
