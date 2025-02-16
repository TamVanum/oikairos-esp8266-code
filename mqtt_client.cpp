#include "mqtt_client.h"
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>

// Declare the MQTT client instance
WiFiClient espClient;
PubSubClient client(espClient);

// Global variable for the room ID
String roomId = "uACrMnf7hLoDGxKUu6md";

// Initialize MQTT connection
void initMQTT(const char* server, int port, void (*callback)(char*, byte*, unsigned int)) {
    client.setServer(server, port);
    client.setCallback(callback);
}

// Reconnect to the MQTT broker
void reconnectMQTT() {
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        if (client.connect("ESP8266Client")) {
            Serial.println("connected");
            client.subscribe("esp8266/uACrMnf7hLoDGxKUu6md/setup"); // Topic to receive roomId
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

// Publish a message to an MQTT topic
void publishMessage(const char* topic, const char* payload) {
    client.publish(topic, payload);
}

// MQTT callback to handle incoming messages
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Message arrived on topic: ");
    Serial.println(topic);

    String message;
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.println("Message: " + message);

    // Parse the JSON payload to extract roomId
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, message);

    if (!error) {
        const char* receivedRoomId = doc["roomId"];
        if (receivedRoomId) {
            roomId = String(receivedRoomId); // Update the global roomId
            Serial.print("Updated roomId to: ");
            Serial.println(roomId);
        }
    } else {
        Serial.print("Failed to parse message: ");
        Serial.println(error.c_str());
    }
}

// Retrieve the current roomId
String getRoomId() {
    return roomId;
}
