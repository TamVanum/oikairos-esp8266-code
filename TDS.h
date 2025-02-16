
#ifndef TDS_H
#define TDS_H

#include <Arduino.h>

#define VREF 4.096   // Reference voltage for the ADC
#define SCOUNT 30    // Number of samples for filtering

class TDS {
public:
  // Constructor
  TDS();

  // Set temperature for compensation
  void setTemperature(float temp);

  // Process the TDS value based on voltage input
  float process(float voltage);

private:
  float temperature = 25.0;          // Default temperature in °C
  int analogBuffer[SCOUNT];          // Buffer for storing samples
  int analogBufferIndex = 0;

  // Helper to calculate the median of sampled data
  int getMedianNum(int bArray[], int iFilterLen);
};

#endif
