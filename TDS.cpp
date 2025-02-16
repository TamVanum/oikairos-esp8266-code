
#include "TDS.h"

// Constructor
TDS::TDS() {}

// Set the temperature for compensation
void TDS::setTemperature(float temp) {
  temperature = temp;
}

// Process the TDS value based on the provided voltage
float TDS::process(float voltage) {
  // Temperature compensation
  float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);
  float compensationVoltage = voltage / compensationCoefficient;

  // Convert voltage to Electrical Conductivity (EC)
  float ecValue = (compensationVoltage * 1000) / VREF * 500;  // Assuming a constant factor
  float ecValueResponse = (ecValue / 100);
  return ecValueResponse;  // Return the EC value in µS/cm
}
// Helper to calculate the median of sampled data
int TDS::getMedianNum(int bArray[], int iFilterLen) {
  int bTab[iFilterLen];
  for (byte i = 0; i < iFilterLen; i++) {
    bTab[i] = bArray[i];
  }

  // Bubble sort
  for (int j = 0; j < iFilterLen - 1; j++) {
    for (int i = 0; i < iFilterLen - j - 1; i++) {
      if (bTab[i] > bTab[i + 1]) {
        int temp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = temp;
      }
    }
  }

  // Return median value
  if ((iFilterLen & 1) > 0) {  // Odd length
    return bTab[(iFilterLen - 1) / 2];
  } else {  // Even length
    return (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  }
}
