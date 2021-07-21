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


//-----------------------------------------------------------------------------
// global functions, shared between all *.cpp files

// humanInputs.cpp
byte readTouch();
void reactTouch();
void handleButton();
void defineBlower();
void defineTemp();

// interruptsHandlers.cpp
void touchAction();
void reedAction();
void btnAction();
void timerHandler();

// lcfStuff.cpp
void clearDigit(byte section, byte digitNumber);
byte printNumber(byte section, short number);
void digitPrint(byte address, byte number);
void printText(byte address, char text[], bool loopText);
void printLetter(byte address, char letter);
void printChannel (byte channel);
void blinkSelection();
void stopBlinking();
void changeSegment(byte address, byte bit, bool value);

// temperatureFunctions.cpp
float readTemp(bool unit);
short handleTempUnit (unsigned short temp, bool unit);
short convertToC(unsigned short temp);
void heat();
int calibrateTemp(bool type);
// void pwmWrite( uint32_t pin, uint32_t freq );



