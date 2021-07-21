//-----------------------------------------------------------------------------
// global.h

// only include this header file once per compilation unit
#pragma once


//-----------------------------------------------------------------------------
// includes
#include <Arduino.h>
#include <Wire.h> //for touch ic

#include <EEPROM.h>

// #include "ht1621.h" //LCD controller https://github.com/altLab/HT1621
// #include "max6675.h" //https://learn.adafruit.com/thermocouple/arduino-code
#include <ht1621.h> //LCD controller https://github.com/altLab/HT1621
#include <max6675.h> //https://learn.adafruit.com/thermocouple/arduino-code
#include <PID_v1.h> //https://github.com/br3ttb/Arduino-PID-Library 

// c++ template helper functions
#include "eeprom.h"
