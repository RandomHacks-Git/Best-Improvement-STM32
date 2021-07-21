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



//-----------------------------------------------------------------------------
// global constants

//LCD section addresses
#define MAIN 17
#define LEFT 25
#define RIGHT 11
#define SMALL 8

//Touch buttons mapping
#define UP 128
#define DOWN 64
#define SET 32
#define CF 16

//Pins
// HT1621 ht(PB9, PA15, PA8, PB15); // LCD data,wr,rd,cs
#define BACKLIGHT PB8
#define TOUCHINT PA2 //touch interrupt
#define PIEZO PA3
#define REEDINT PB5 //reed switch pin
#define BLOWER PB0 //blower pin
#define HEATER PB3 //heater pin
#define BTN1 PB12 //physical button 1
#define BTN2 PB13 //physical button 2
#define BTN3 PB14 //physical button 3
#define AHEAT PA0 //temperature pot analog pin 
#define ABLOW PA1 //blower pot analog pin                                          

//thermocouple pins
#define THERMO_CLK PA5
#define THERMO_CS PA4
#define THERMO_DO PA6
// MAX6675 thermocouple(THERMO_CLK, THERMO_CS, THERMO_DO);

//PID
#define KP 3.2F
#define KI 0.17F
#define KD 1.5F
#define HIGH_KP 3.2F
#define HIGH_KI 0.08F
#define HIGH_KD 0.32F

//Miscellaneous
#define FIRMWARE_VERSION 101 //firmware version (1.01)
#define DEBOUNCETIME 10 //button debounce time
#define LCDBRIGHTNESS 1 //default brightness, closer to 0 -> brighter, closer to 65535 -> dimmer
#define LCDBRIGHTNESSDIM 25000 //standby brightness
#define MINTEMP 100
#define MAXTEMP 550
#define MINBLOW 35
#define MAXBLOW 100
#define SHUTDOWNTEMP 85 //below this temp the blower shuts off
#define SETTINGSEXITTIME 8000 //if you are inside settings (blinking screen), it will automatically exit after this time (ms)
#define WINDOWSIZE 205 //Heater PWM period size (ms)
#define SERIALTIME 500 //Serial output time (ms)

//Calibration factors (so the air that comes OUT OF THE NOZZLE actually reaches the set temperature)
#define RANGE200 1.05
#define RANGE300 1.08
#define RANGE400 1.11

// touch IC address / registers
#define touchAddress 0x56 // Device address
#define keyValues 0x34 // Hexadecimal address for the key values register
#define sysCon 0x3A //hex value for config register
#define kdr0 0x23 //hex value for key disable register
#define mcon 0x21 //hex value for mode control register
#define gsr 0x20 //hex address for global sensitivity register


//-----------------------------------------------------------------------------
// export globals defined in main.cpp, into all the other *.cpp files (except for main.cpp)
#ifndef __MAIN_CPP__

extern volatile unsigned long touchMillis;
extern volatile bool touched, touchReleased;
extern unsigned long lastTempPrint;
extern unsigned long lastTempRead;
extern unsigned long lastTempIcon;
extern unsigned long lastSerialOutput;
extern volatile bool reedStatus;
extern bool btn1, btn2, btn3;
extern volatile unsigned long btnMillis;
extern volatile bool buttonFlag;
extern volatile bool toneFlag, longToneFlag;
extern unsigned long lastToneMillis;
extern volatile bool reedFlag;
extern volatile byte touchedButton;
extern volatile bool readTouchFlag;
extern unsigned long lastReact;
extern bool standby;
extern bool timer;
extern bool timeUnit;
extern volatile bool timerFlag;
extern volatile bool setPointReached;
extern volatile byte setPointChanged;
extern bool converted;
extern bool newPotValue;
extern unsigned long potMillis;
extern unsigned long windowStartTime;
extern unsigned long setPointReachedTime;
extern volatile bool heating;
extern bool blowerOn;
extern volatile bool coolingAfterTimer;
extern unsigned short heaterVal, blowerVal;
extern unsigned short setTemp, setBlow, lastSetBlow, setTimer, timerTemporary;
extern float currentTemp;
extern byte selectedSection;
extern unsigned long lastBlink;
extern bool sectionOff;
extern bool switchDisplayed;
extern bool displayingVersion;

struct chSettings {
  unsigned short temp;
  unsigned short blow;
};

struct structOtherSettings {
  bool tempUnit; //1 for ºC, 0 for ºF
  bool buzzer;
  byte selectedCh;
  short calTemp; //temperature calibration value, can be negative or positive
  bool serialOutput;
};

extern chSettings ch1Settings, ch2Settings, ch3Settings, touchSettings;
extern structOtherSettings otherSettings;
extern unsigned int eepromCheck;
extern double setPoint, input, output;
extern PID myPID;
extern HardwareTimer hwTimer;
extern HardwareTimer timerSeconds;
extern HT1621 ht;
extern MAX6675 thermocouple;

#endif


