#ifndef DATA_FORMATTER_H
#define DATA_FORMATTER_H

#include <ArduinoJson.h>

// Functions for formatting data
StaticJsonDocument<200> formatSensorData(float pH, float tds, float waterTemp, float ambientTemp, bool water_pump, bool water_flow);
StaticJsonDocument<200> generateDefaultTestData();
String serializeData(const StaticJsonDocument<200>& doc);

#endif
