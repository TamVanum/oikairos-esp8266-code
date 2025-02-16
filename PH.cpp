#include "PH.h"

float PH::calculate(float offset, float rawValue) {
    // Replace with your pH calculation logic
    // float voltage = rawValue * 0.1875 / 1000.0; // Convert raw value to voltage
    // float phValue = 5 + (voltage - offset);   // Example calculation

    float voltage = rawValue * (4.096 / 32768.0);
    float phValue =  3.5 * voltage + offset;
    return phValue;
}
