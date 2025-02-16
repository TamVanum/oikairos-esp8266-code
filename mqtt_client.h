#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <PubSubClient.h>

// Expose the MQTT client instance
extern PubSubClient client;

// Declare the roomId variable
extern String roomId;

// Functions
void initMQTT(const char* server, int port, void (*callback)(char*, byte*, unsigned int));
void reconnectMQTT();
void publishMessage(const char* topic, const char* payload);
String getRoomId(); // Retrieve the current roomId

#endif
