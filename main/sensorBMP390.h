#pragma once
#include <Arduino.h>

bool setupBMP390();
bool readBMP390(float &tempC, float &pressurePa, float &altM);
