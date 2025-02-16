#include "wifi_manager.h"
#include "mqtt_client.h"
#include "data_formatter.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <PubSubClient.h>
#include <Adafruit_ADS1X15.h>
#include "PH.h"
#include "TDS.h"

// =========================================================
// Configuration
// =========================================================
void mqttCallback(char* topic, byte* payload, unsigned int length);

// // WiFi credentials Casa
// const char* ssid = "Daniela R-2.4G-xkrq";
// const char* password = "Dani0404";

// // MQTT server credentials
// const char* mqtt_server = "192.168.100.76";

// // WiFi credentials Celular
const char* ssid = "Hydro";
const char* password = "hydro123";

// MQTT server credentials
const char* mqtt_server = "192.168.82.135";

const int mqtt_port = 1883;

const char* api_url = "http://192.168.82.135:3000/api/hydroponic/metrics/uACrMnf7hLoDGxKUu6md/in-use";


// Temperature sensors setup
#define ONE_WIRE_BUS 0  // Data pin for temperature sensors

// 74HC595 Shift Register pins
const int dataPin = D7;   // SER (Data)
const int clockPin = D6;  // SHCP (Clock)
const int latchPin = D5;  // STCP (Latch)

// Range definitions
float phMin = 0.0;
float phMax = 14.0;
float tdsMin = 0.0;
float tdsMax = 5000.0;
float ambTempMin = 0.0;
float ambTempMax = 50.0;
float waterTempMin = 0.0;
float waterTempMax = 50.0;
// Pump timing variables
unsigned long pumpStopTime = 0;  // When the pump should stop
bool pumpIsOn = false;           // Track if the pump is currently on

// Relays (connected to 74HC595 outputs)
// If active-low: '0' bit = ON, '1' bit = OFF.
// Adjust mapping as per your wiring.
#define RELAY_PH_HIGH 1
#define RELAY_PH_LOW 2
#define RELAY_TDS_LOW 3
#define RELAY_FLOW_PUMP 4
#define RELAY_MAIN_WATER_PUMP 5
#define RELAY_SPARE 6

// Simulation mode (Set to false when real ADS is available)
bool simulateADS = false;

// Simulation ranges for pH and TDS
float simulatedPHMin = 5.5;
float simulatedPHMax = 8.0;
float simulatedTDSMin = 50.0;
float simulatedTDSMax = 500.0;

// =========================================================
// Global Objects and Variables
// =========================================================

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
Adafruit_ADS1115 ads;

PH phSensor;    // PH object
TDS tdsSensor;  // TDS object

int numberOfDevices;
DeviceAddress ambientSensorAddress;
DeviceAddress waterSensorAddress;

// Current relay states stored in a byte (1 bit per relay)
// Start with all OFF (assuming active-high logic here, adjust if needed)
uint8_t relayStates = 0xFF;



// Activation cycle counters for relays
int phHighCycleCount = 0;
int phLowCycleCount = 0;
int tdsLowCycleCount = 0;

// Waiting cycle counters for relays
int phHighWaitCount = 0;
int phLowWaitCount = 0;
int tdsLowWaitCount = 0;

// Define constants for the number of cycles
const int activeCycles = 1;  // Number of cycles a relay can stay active
const int waitCycles = 3;


// =========================================================
// Helper Functions
// =========================================================

// Update relay outputs via the 74HC595 shift register
void setRelayState(uint8_t relay, bool state) {
  if (state) {
    relayStates |= (1 << relay);  // ON = set bit to 1
  } else {
    relayStates &= ~(1 << relay);  // OFF = set bit to 0
  }

  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, relayStates);
  digitalWrite(latchPin, HIGH);
}

// =========================================================
// Initialization Functions
// =========================================================

void initSensors() {
  sensors.begin();
  numberOfDevices = sensors.getDeviceCount();
  Serial.print("Found ");
  Serial.print(numberOfDevices);
  Serial.println(" temperature sensors.");

  if (!sensors.getAddress(ambientSensorAddress, 0)) {
    Serial.println("Ambient temperature sensor not found.");
  }
  if (!sensors.getAddress(waterSensorAddress, 1)) {
    Serial.println("Water temperature sensor not found.");
  }
}

void initShiftRegister() {
  pinMode(dataPin, OUTPUT);
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);

  // Initialize all relays to OFF
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, relayStates);
  digitalWrite(latchPin, HIGH);
}

void initADS() {
  ads.begin();
}


// =========================================================
// Valve Activation Timer Logic
// =========================================================

// Call this function whenever a valve (pH High/Low or TDS Low) is activated.
// It ensures the flow pump runs for at least 1 minute after the last valve activation.
void valveActivated() {
  // Print the current and updated pump stop times for debugging
  Serial.print("Pump timer extended. Old stop time: ");
  Serial.println(pumpStopTime);

  pumpStopTime = millis() + 60000;  // 1 minute = 60,000 ms

  Serial.print("New stop time: ");
  Serial.println(pumpStopTime);

  // If pump is off, turn it on now
  if (!pumpIsOn) {
    controlFlowPump(true);
    pumpIsOn = true;
    Serial.println("Pump turned ON due to valve activation.");
  } else {
    Serial.println("Pump timer reset due to new valve activation.");
  }
}

// After setting relays based on conditions, check which valves are ON and which were just turned ON
// We'll compare old and new states to detect new activations.
bool wasRelayOn(uint8_t relay) {
  return (relayStates & (1 << relay)) != 0;
}


// =========================================================
// Sensor Reading Functions
// =========================================================

float getAmbientTemperature() {
  return sensors.getTempC(ambientSensorAddress);
}

float getWaterTemperature() {
  return sensors.getTempC(waterSensorAddress);
}

// Returns a pH reading (simulate if simulateADS = true)
float getPH(float waterTemp) {
  if (simulateADS) {
    return random((int)(simulatedPHMin * 100), (int)(simulatedPHMax * 100)) / 100.0;
  } else {
    int16_t rawPHValue = ads.readADC_SingleEnded(1);  // pH sensor channel
    float phValue = phSensor.calculate(0.0, rawPHValue);
    return phValue;
  }
}

// Returns a TDS reading (simulate if simulateADS = true)
float getTDS(float waterTemp) {
  if (simulateADS) {
    return random((int)(simulatedTDSMin * 10), (int)(simulatedTDSMax * 10)) / 10.0;
  } else {
    int16_t rawTDSValue = ads.readADC_SingleEnded(0);  // TDS sensor channel
    float tdsVoltage = rawTDSValue * 0.1875 / 1000.0;
    tdsSensor.setTemperature(waterTemp);
    float tdsValue = tdsSensor.process(tdsVoltage);
    return tdsValue;
  }
}

// =========================================================
// Temperature Warnings
// =========================================================

void printTemperatureWarnings(float ambTemp, float waterTemp) {
  if (ambTemp < ambTempMin || ambTemp > ambTempMax) {
    Serial.println("Warning: Ambient temperature out of range.");
  } else {
    Serial.println("Ambient temperature is within range.");
  }

  if (waterTemp < waterTempMin || waterTemp > waterTempMax) {
    Serial.println("Warning: Water temperature out of range.");
  } else {
    Serial.println("Water temperature is within range.");
  }
}

// =========================================================
// Control Functions
// =========================================================

void controlPHRelays(float phValue) {
  // pH High Relay Logic
  if (phValue > phMax && phHighWaitCount == 0) {
    if (phHighCycleCount < activeCycles) {
      setRelayState(RELAY_PH_HIGH, true);
      delay(1000);
      setRelayState(RELAY_PH_HIGH, false);
      phHighCycleCount++;
      Serial.println("pH is above range. Relay PH_HIGH activated.");
    } else {
      setRelayState(RELAY_PH_HIGH, false);
      phHighWaitCount = waitCycles;  // Start wait cycle
      Serial.println("Relay PH_HIGH deactivated after active cycles.");
    }
  } else {
    setRelayState(RELAY_PH_HIGH, false);
    if (phHighWaitCount > 0) phHighWaitCount--;  // Decrease wait count if waiting
    phHighCycleCount = 0;                        // Reset active counter if not active
  }

  // pH Low Relay Logic
  if (phValue < phMin && phLowWaitCount == 0) {
    if (phLowCycleCount < activeCycles) {
      setRelayState(RELAY_PH_LOW, true);
      delay(1000);
      setRelayState(RELAY_PH_LOW, false);
      phLowCycleCount++;
      Serial.println("pH is below range. Relay PH_LOW activated.");
    } else {
      setRelayState(RELAY_PH_LOW, false);
      phLowWaitCount = waitCycles;  // Start wait cycle
      Serial.println("Relay PH_LOW deactivated after active cycles.");
    }
  } else {
    setRelayState(RELAY_PH_LOW, false);
    if (phLowWaitCount > 0) phLowWaitCount--;  // Decrease wait count if waiting
    phLowCycleCount = 0;                       // Reset active counter if not active
  }
}

void controlTDSRelays(float tdsValue) {
  if (tdsValue < tdsMin && tdsLowWaitCount == 0) {
    if (tdsLowCycleCount < activeCycles) {
      setRelayState(RELAY_TDS_LOW, true);
      delay(1000);
      setRelayState(RELAY_TDS_LOW, false);
      tdsLowCycleCount++;
      Serial.println("TDS is below range. Relay TDS_LOW activated.");
    } else {
      setRelayState(RELAY_TDS_LOW, false);
      tdsLowWaitCount = waitCycles;  // Start wait cycle
      Serial.println("Relay TDS_LOW deactivated after active cycles.");
    }
  } else {
    setRelayState(RELAY_TDS_LOW, false);
    if (tdsLowWaitCount > 0) tdsLowWaitCount--;  // Decrease wait count if waiting
    tdsLowCycleCount = 0;  // Reset active counter if not active
  }
}


void controlMainWaterPump(bool state) {
  setRelayState(RELAY_MAIN_WATER_PUMP, state);
  if (state) Serial.println("Main Water Pump ON.");
  else Serial.println("Main Water Pump OFF.");
}

void controlFlowPump(bool state) {
  setRelayState(RELAY_FLOW_PUMP, state);
  if (state) Serial.println("Water Flow Pump ON.");
  else Serial.println("Water Flow Pump OFF.");
}

// =========================================================
// Main Setup and Loop
// =========================================================

void setup() {
  Serial.begin(115200);

  setupWiFi(ssid, password);  // From WiFi manager

  StaticJsonDocument<2048> doc;  // Allocate memory for the JSON document
  JsonArray metrics = fetchMetrics(api_url, doc);

  if (!metrics.isNull()) {
    Serial.println("\nMetrics received:");
    setRanges(metrics);  // Set ranges dynamically from metrics
  } else {
    Serial.println("No metrics returned or an error occurred.");
  }


  initMQTT(mqtt_server, mqtt_port, mqttCallback);  // From MQTT client
  initADS();
  initSensors();
  initShiftRegister();

  setRelayState(RELAY_MAIN_WATER_PUMP, true);
  setRelayState(RELAY_FLOW_PUMP, true);

  setRelayState(RELAY_PH_HIGH, false);
  setRelayState(RELAY_PH_LOW, false);
  setRelayState(RELAY_TDS_LOW, false);
  setRelayState(RELAY_SPARE, false);

  Serial.println("System initialized.");
}

void loop() {

  static int loopCounter = 0;

  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  sensors.requestTemperatures();

  float ambTemp = getAmbientTemperature();
  float waterTemp = getWaterTemperature();
  float phValue = getPH(waterTemp);
  float tdsValue = getTDS(waterTemp);


  uint8_t oldRelayStates = relayStates;

  // Control relays based on sensor values
  controlPHRelays(phValue);
  controlTDSRelays(tdsValue);

  // Check which valves are currently on
  bool phHighNow = ((relayStates & (1 << RELAY_PH_HIGH)) != 0);
  bool phLowNow = ((relayStates & (1 << RELAY_PH_LOW)) != 0);
  bool tdsLowNow = ((relayStates & (1 << RELAY_TDS_LOW)) != 0);

  // Check which valves were on previously
  bool phHighOld = ((oldRelayStates & (1 << RELAY_PH_HIGH)) != 0);
  bool phLowOld = ((oldRelayStates & (1 << RELAY_PH_LOW)) != 0);
  bool tdsLowOld = ((oldRelayStates & (1 << RELAY_TDS_LOW)) != 0);


  // If a valve just turned on this cycle (was off before, on now), extend pump time
  if ((phHighNow && !phHighOld) || (phLowNow && !phLowOld) || (tdsLowNow && !tdsLowOld)) {
    valveActivated();
  }

  // If the pump is on and the current time passed pumpStopTime, turn it off
  if (pumpIsOn && millis() > pumpStopTime) {
    controlFlowPump(false);
    pumpIsOn = false;
  }

  StaticJsonDocument<200> sensorData = formatSensorData(
    phValue, tdsValue, waterTemp, ambTemp, true, pumpIsOn);

  String jsonString;
  serializeJson(sensorData, jsonString);

  // Print the JSON string to the Serial Monitor
  Serial.println("Generated JSON:");
  Serial.println(jsonString);

  char jsonBuffer[512];
  serializeJson(sensorData, jsonBuffer);


  if (roomId.isEmpty()) {
    Serial.println("Room ID not set. Using default topic.");
    publishMessage("esp8266/uACrMnf7hLoDGxKUu6md/data", jsonBuffer);
  } else {
    String topic = "esp8266/" + roomId + "/data";
    publishMessage(topic.c_str(), jsonBuffer);
  }
  printTemperatureWarnings(ambTemp, waterTemp);


  loopCounter++;

  // Perform API call and set ranges every 5 loops
  if (loopCounter >= 5) {
    StaticJsonDocument<2048> doc;  // Allocate memory for the JSON document
    JsonArray metrics = fetchMetrics(api_url, doc);

    if (!metrics.isNull()) {
      Serial.println("\nMetrics received:");
      setRanges(metrics);  // Set ranges dynamically from metrics
    } else {
      Serial.println("No metrics returned or an error occurred.");
    }

    loopCounter = 0;  // Reset the counter
  }
  delay(5000);
}



void setRanges(JsonArray metrics) {
  for (JsonObject metric : metrics) {
    const char* name = metric["name"];
    float minimum = metric["minimum"];
    float maximum = metric["maximum"];

    if (strcmp(name, "ph_level") == 0) {
      phMin = minimum;
      phMax = maximum;
      Serial.printf("Set pH range: Min = %.2f, Max = %.2f\n", phMin, phMax);
    } else if (strcmp(name, "electrical_conductivity") == 0) {
      tdsMin = minimum;
      tdsMax = maximum;
      Serial.printf("Set TDS range: Min = %.2f, Max = %.2f µS/cm\n", tdsMin, tdsMax);
    } else if (strcmp(name, "ambient_temperature") == 0) {
      ambTempMin = minimum;
      ambTempMax = maximum;
      Serial.printf("Set Ambient Temperature range: Min = %.2f°C, Max = %.2f°C\n", ambTempMin, ambTempMax);
    } else if (strcmp(name, "water_temperature") == 0) {
      waterTempMin = minimum;
      waterTempMax = maximum;
      Serial.printf("Set Water Temperature range: Min = %.2f°C, Max = %.2f°C\n", waterTempMin, waterTempMax);
    }
  }
}
