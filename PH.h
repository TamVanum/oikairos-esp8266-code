#ifndef PH_H
#define PH_H

#include <Arduino.h>

class PH {
public:
    float calculate(float offset = 0.0, float rawValue = 0.0);
};

#endif