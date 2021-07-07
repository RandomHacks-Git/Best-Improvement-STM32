//uses core https://github.com/stm32duino/Arduino_Core_STM32
//uses bootloader generic_boot20_pc13_fastboot.bin https://github.com/rogerclarkmelbourne/STM32duino-bootloader/tree/master/binaries installed with STM "Flash Loader demonstrator" over serial with Boot0 set high (first install generic_boot20_pc13 and only then install the fastboot version with the setting "Erase necessary pages" selected or else it won't be detected by your pc)
//to install windows drivers see https://github.com/rogerclarkmelbourne/Arduino_STM32/wiki/Maple-drivers
//if there is an error while compiling you might need to install SAMD M0+ boards on Arduino IDE

#include <Wire.h> //for touch ic
#include <EEPROM.h>
#include "ht1621.h" //LCD controller https://github.com/altLab/HT1621
#include "max6675.h" //https://learn.adafruit.com/thermocouple/arduino-code
#include <PID_v1.h> //https://github.com/br3ttb/Arduino-PID-Library 

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
HT1621 ht(PB9, PA15, PA8, PB15); // LCD data,wr,rd,cs
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
MAX6675 thermocouple(THERMO_CLK, THERMO_CS, THERMO_DO);

//PID
#define KP 3.2F
#define KI 0.17F
#define KD 1.5F
#define HIGH_KP 3.2F
#define HIGH_KI 0.08F
#define HIGH_KD 0.32F

//Miscellaneous
#define DEBOUNCETIME 10 //button debounce time
#define LCDBRIGHTNESS 1 //default brightness, closer to 0 -> brighter, closer to 65535 -> dimmer
#define LCDBRIGHTNESSDIM 25000 //standby brightness
#define MINTEMP 100
#define MAXTEMP 550
#define MINBLOW 35
#define MAXBLOW 100
#define SHUTDOWNTEMP 80 //below this temp the blower shuts off
#define SETTINGSEXITTIME 7000 //if you are inside settings (blinking screen), it will automatically exit after this time (ms)
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

volatile unsigned long touchMillis;
volatile bool touched, touchReleased = 1;

unsigned long lastTempPrint;
unsigned long lastTempRead;
unsigned long lastTempIcon;
unsigned long lastSerialOutput;
volatile bool reedStatus;
bool btn1, btn2, btn3;
volatile unsigned long btnMillis;
volatile bool buttonFlag;
volatile bool toneFlag, longToneFlag;
unsigned long lastToneMillis;
volatile bool reedFlag;
volatile byte touchedButton;
volatile bool readTouchFlag;
unsigned long lastReact;
bool standby;
bool timer = false;
bool timeUnit = 1;
volatile bool timerFlag;
volatile bool setPointReached;
volatile byte setPointChanged = 2; //0 = no change, 1 = decreased, 2 = increased
bool newPotValue;
unsigned long potMillis;
unsigned long windowStartTime;
unsigned long setPointReachedTime;
volatile bool heating;
bool blowerOn;
volatile bool coolingAfterTimer;

unsigned short heaterVal, blowerVal; //analog pot values for heater and blower
unsigned short setTemp, setBlow, lastSetBlow, setTimer, timerTemporary; //current set values
float currentTemp;

byte selectedSection;
unsigned long lastBlink;
bool sectionOff;
bool switchDisplayed;

struct chSettings {
  unsigned short temp;
  unsigned short blow;
};

struct otherSettings {
  bool tempUnit; //1 for ºC, 0 for ºF
  bool buzzer;
  byte selectedCh;
  short calTemp; //temperature calibration value, can be negative or positive
  bool serialOutput;
};

chSettings ch1Settings, ch2Settings, ch3Settings, touchSettings;
otherSettings otherSettings;

unsigned int eepromCheck = 1234567890; //used in setup to check if settings where previously saved to flash (emulated eeprom)

//PID
//P - how fast it shoots towards set point, if set too high creates overshoot, I - remove oscillations and offset, can increase overshoot D - like a break, removes overshoot, if set too high leads to unresponsiveness
double setPoint = 0, input = 0, output = 0;

//double KP = 3.2, KI = 0.17, KD = 1.5; //<400ºC
//double HIGH_KP = 3.2, HIGH_KI = 0.08, HIGH_KD = 0.32; //>=400ºC

PID myPID(&input, &output, &setPoint, KP, KI, KD, P_ON_E, DIRECT);

HardwareTimer hwTimer(3);//pwm
HardwareTimer timerSeconds(1);//time timer

void setup() {
  Serial.begin(115200);
  unsigned int tempEepromCheck;
  EEPROM_get(0, tempEepromCheck);
  if (tempEepromCheck == eepromCheck) { //checks if the variable eepromCheck is already stored in flash, if it is,reads the settings else stores them.
    EEPROM_get(4, ch1Settings);
    EEPROM_get(8, ch2Settings);
    EEPROM_get(12, ch3Settings);
    EEPROM_get(16, touchSettings);
    EEPROM_get(20, otherSettings);
    Serial.println("Settings loaded from EEPROM");
  }
  else {
    ch1Settings.temp = 200;
    ch1Settings.blow = 100;
    ch2Settings.temp = 360;
    ch2Settings.blow = 50;
    ch3Settings.temp = 500;
    ch3Settings.blow = 70;
    //    ch1Settings = {200, 100};
    //    ch2Settings = {360, 50};
    //    ch3Settings = {500, 70};
    otherSettings.tempUnit = 1;
    otherSettings.buzzer = 1;
    otherSettings.selectedCh = 1;
    otherSettings.calTemp = 7;
    otherSettings.serialOutput = 0;

    EEPROM_put(4, ch1Settings);
    EEPROM_put(8, ch2Settings);
    EEPROM_put(12, ch3Settings);
    EEPROM_put(16, touchSettings);
    EEPROM_put(20, otherSettings);
    EEPROM_put(0, eepromCheck);
    Serial.println("Default settings saved to EEPROM");
  }

  //initialize pins
  pinMode(BACKLIGHT, PWM_OPEN_DRAIN);   //initialize backlight pin
  pwmWrite(BACKLIGHT, 65535); //backlight off until initializing LCD
  pinMode(PIEZO, OUTPUT);
  pinMode(BLOWER, PWM);
  pwmWrite(BLOWER, 2520); //turn on blower to get a more accurate reading of the temperature (in case there was a short power failure or you turned off the station while it was still hot the blower will turn on to avoid damage to the heater element/handle)
  pinMode(HEATER, OUTPUT);
  pinMode(AHEAT, INPUT_ANALOG);
  pinMode(ABLOW, INPUT_ANALOG);
  pinMode(BTN1, INPUT_PULLUP);
  pinMode(BTN2, INPUT_PULLUP);
  pinMode(BTN3, INPUT_PULLUP);
  pinMode(REEDINT, INPUT_PULLUP);
  pinMode(TOUCHINT, INPUT);

  //change PWM frequency for the blower pin
  hwTimer.pause();
  //hwTimer.setPrescaleFactor(2);
  hwTimer.setOverflow(hwTimer.setPeriod(35)); //change PWM frequency to ~28.571kHz (to get rid of audible whine from the blower), overflow (max) is 2520
  hwTimer.refresh();
  hwTimer.resume();

  //initialize LCD
  ht.begin();
  ht.sendCommand(HT1621::BIAS_THIRD_4_COM);
  ht.sendCommand(HT1621::SYS_EN);

  for (int i = 0 ; i < 32 ; i++) { //turn all LCD segments off
    ht.writeMem(i, 0);
  }

  ht.sendCommand(HT1621::LCD_ON); //turn lcd on

  blowerVal = analogRead(ABLOW);
  heaterVal = analogRead(AHEAT);

  switch (otherSettings.selectedCh) {
    case 0: //pot
      defineBlower();
      defineTemp();
      break;
    case 1: //ch1
      setTemp = ch1Settings.temp;
      setBlow = ch1Settings.blow;
      printNumber(MAIN, handleTempUnit(setTemp, otherSettings.tempUnit));
      printNumber(LEFT, setBlow);
      break;
    case 2: //ch2
      setTemp = ch2Settings.temp;
      setBlow = ch2Settings.blow;
      printNumber(MAIN, handleTempUnit(setTemp, otherSettings.tempUnit));
      printNumber(LEFT, setBlow);
      break;
    case 3: //ch3
      setTemp = ch3Settings.temp;
      setBlow = ch3Settings.blow;
      printNumber(MAIN, handleTempUnit(setTemp, otherSettings.tempUnit));
      printNumber(LEFT, setBlow);
      break;
    case 4: //touch
      setTemp = touchSettings.temp;
      setBlow = touchSettings.blow;
      printNumber(MAIN, handleTempUnit(setTemp, otherSettings.tempUnit));
      printNumber(LEFT, setBlow);
  }

  //turn on touch keys segments
  ht.writeMem(6, B1111);

  //turn on fan icon
  ht.writeMem(31, B0010);

  //turn on clock icon
  changeSegment(24, 3, 1);

  //turn on "off" icon
  changeSegment(16, 3, 1);

  //turn on ºC or ºF on main display
  if (otherSettings.tempUnit)ht.writeMem(10, B0001); //ºC
  else ht.writeMem(10, B0010); //ºF

  printChannel(otherSettings.selectedCh); //Print channel to screen

  reedStatus = digitalRead(REEDINT); //check if handle is in base
  if (reedStatus) changeSegment(31, 2, 1); //handle out of base turn on "handle" icon
  else changeSegment(31, 2, 0); //handle in base turn off "handle" icon

  changeSegment(7, 1, otherSettings.serialOutput); //turn "RS232 On" segment on or off according to last setting

  pwmWrite(BACKLIGHT, LCDBRIGHTNESS); //turn on backlight at default brightness

  Wire.begin(); // Initiate the Wire library

  // Config APT8L08 (touch IC)
  Wire.beginTransmission(touchAddress);//(0x56);
  Wire.write(byte(sysCon));//(sysCon);
  Wire.write(byte(0x5A));//(90);// Set to config mode (0x5A)
  Wire.endTransmission();

  Wire.beginTransmission(touchAddress);
  Wire.write(byte(kdr0));//(kdr0);
  Wire.write(byte(15));// Disable first 4 inputs (0000 1111)
  Wire.endTransmission();

  Wire.beginTransmission(touchAddress);
  Wire.write(byte(mcon));//(mcon);
  Wire.write(byte(81));// interrupt changes when releasing key 81 (0101 0001) multitouch 85 (0101 0101), pulse interrupt + multitouch 93
  Wire.endTransmission();

  Wire.beginTransmission(touchAddress);
  Wire.write(byte(gsr));//(gsr);
  Wire.write(byte(0x01));// Lower sensitivity (0x01), chip default 0x02, max 0x0F
  Wire.endTransmission();

  Wire.beginTransmission(touchAddress);
  Wire.write(byte(sysCon));//(sysCon);
  Wire.write(byte(0x00));// Set to work mode (0x00)
  Wire.endTransmission();

  delay(600); //so temperature reaches the thermocouple for a more accurate reading
  currentTemp = readTemp(1);
  if (currentTemp > SHUTDOWNTEMP + 20) {
    pwmWrite(BLOWER, 2520); //turn on blower at max
    blowerOn = true;
  }
  else pwmWrite(BLOWER, 0);

  //Setup interrupts
  attachInterrupt(digitalPinToInterrupt(TOUCHINT), touchAction, CHANGE);
  attachInterrupt(digitalPinToInterrupt(REEDINT), reedAction, CHANGE);
  attachInterrupt(digitalPinToInterrupt(BTN1), btnAction, CHANGE);
  attachInterrupt(digitalPinToInterrupt(BTN2), btnAction, CHANGE);
  attachInterrupt(digitalPinToInterrupt(BTN3), btnAction, CHANGE);

  myPID.SetOutputLimits(0, WINDOWSIZE); //PWM on time varies between 0ms and 205ms
  myPID.SetSampleTime(150); //PID refresh rate

  timerSeconds.pause();
  timerSeconds.setPeriod(1000000); // Set up period, 1 second in microseconds
  timerSeconds.setMode(TIMER_CH2, TIMER_OUTPUTCOMPARE); // Set up an interrupt on channel 2
  timerSeconds.setCompare(TIMER_CH2, 1);  // Interrupt 1 count after each update
  //timerSeconds.atachInterrupt(TIMER_CH2, timerHandler);
  timerSeconds.refresh(); // Refresh the timer's count, prescale, and overflow
  timerSeconds.resume(); // Start the timer counting
}

void loop() {
  if (timerFlag) {
    printNumber(RIGHT, timerTemporary);
    timerFlag = false;
  }

  if (toneFlag) {
    tone(PIEZO, 7000, 100);
    toneFlag = false;
    lastToneMillis = millis(); //tone interferes with the analog readings so I'm using this do ignore the readings while buzzer is beeping
  }

  if (longToneFlag) {
    tone(PIEZO, 6000, 800);
    longToneFlag = false;
    lastToneMillis = millis(); //tone interferes with the analog readings so I'm using this do ignore the readings while buzzer is beeping
  }

  if (readTouchFlag) { //read which button was touched
    touchedButton = readTouch();
    readTouchFlag = false;
  }

  if (reedFlag) {
    if (reedStatus) { //out of base
      timerTemporary = setTimer;
      changeSegment(31, 2, 1); //turn on iron icon
      pwmWrite(BLOWER, map(setBlow, 0, 100, 0, 2520)); //turn blower on at current setting, map percentage to duty cycle
      myPID.SetMode(AUTOMATIC); //turn on PID
      windowStartTime = millis();
      heating = true;
      blowerOn = true;
      ht.writeMem(23, B1000); //thermometer icon on
    }
    else { //in base, reset LCD values and set blower to max
      timerSeconds.detachInterrupt(TIMER_CH2);
      changeSegment(31, 2, 0); //turn off iron icon
      myPID.SetMode(MANUAL); //turn off PID
      output = 0;
      digitalWrite(HEATER, LOW);
      heating = false;
      setPointReached = false;
      setPointChanged = 2;
      coolingAfterTimer = false;
      if (readTemp(1) > SHUTDOWNTEMP && blowerOn) pwmWrite(BLOWER, 2520); //turns blower to full if the temperature is above the shutdown temperature and the blower isn't already off after using the timer
      ht.writeMem(23, 0); //turn of temp icon
      changeSegment(24, 0, 0); //turn off ºC
      changeSegment(24, 1, 0); //turn off ºF
      changeSegment(31, 0, 0); //turn off Set
      changeSegment(31, 1, 1); //turn on fan icon
      printNumber(LEFT, setBlow); //display set blower percentage
      setPointChanged = 2; // change back to setpoint decreased
      if (setTimer != 0) { //set the timer back to whatever it was before removing handle from base
        printNumber(RIGHT, setTimer);
        changeSegment(10, 3, 1); //enable S icon
      }
      else {
        for (byte i = 11; i < 17; i++) { //disable timer number segments
          ht.writeMem(i, B0000);
        }
        changeSegment(16, 3, 1); //enable OFF icon
        changeSegment(10, 3, 0); //disable S icon
      }
    }
    reedFlag = false;
  }

  if (blowerOn && reedStatus && (!timer || timerTemporary > 0) && !coolingAfterTimer) { //Turn the heater on/off to regulate temp
    //noInterrupts();
    heat();
    //interrupts();
  }
  else digitalWrite(HEATER, LOW);

  unsigned long millisecs = millis(); //to avoid wasting time getting millis a milion times
  if (!standby && !reedStatus && !selectedSection && millisecs - touchMillis > 6000 && millisecs - btnMillis > 6000 &&  millisecs - potMillis > 6000) { //Standby
    pwmWrite(BACKLIGHT, LCDBRIGHTNESSDIM); //decrease backlight brightness to the default standby value
    standby = true;
    changeSegment(22, 3, 1); //turn moon/star icon on
  }
  else if (standby && (reedStatus || millisecs - touchMillis < 6000 || millisecs - btnMillis < 6000 || millisecs - potMillis < 6000)) { //Active
    pwmWrite(BACKLIGHT, LCDBRIGHTNESS); //increase backlight brightness to the default value
    changeSegment(22, 3, 0); //turn moon/star icon off
    standby = false;
  }

  if (touched) {
    if (millisecs - lastReact >= 100) reactTouch();
    if (touchReleased) touched = false;
  }

  if (selectedSection != 0 && millisecs - lastBlink >= 300) { //Blink selection (settings)
    blinkSelection();
    if (selectedSection != 4 && touchReleased && millisecs - touchMillis >= SETTINGSEXITTIME) { //stop blinking if last time touched > SETTINGSEXITTIME (I should have used underscores for the defines, this one officially changed to setting sexy time in my head)
      stopBlinking();
    }
  }

  millisecs = millis();
  if ((!setPointReached || selectedSection == 4) && blowerOn && (reedStatus || readTemp(1) >= SHUTDOWNTEMP + otherSettings.calTemp) && (selectedSection == 0 || selectedSection == 4) && millisecs - lastTempPrint >= 200 && millisecs - potMillis >= 1000) { //Print real temp to LCD
    printNumber(MAIN, calibrateTemp(0));
    lastTempPrint = millis();
  }

  if (buttonFlag) handleButton(); //check which button was pressed (channel buttons)

  if (millis() - lastToneMillis >= 180) { //read pots, tone interferes with analog readings :/
    if (analogRead(ABLOW) > blowerVal + 15 || analogRead(ABLOW) < blowerVal - 15) defineBlower();
    if (analogRead(AHEAT) > heaterVal + 12 || analogRead(AHEAT) < heaterVal - 12) defineTemp();
  }

  if (setPointReached && newPotValue && millis() - potMillis >= 1000) { //because the temperature changes almost instantly I only change the setPoinReached bool if the last time I moved a pot was atleast a second ago, otherwise the buzzer would beep while I was still changing the temperature
    setPointReached = false;
    newPotValue = false;
  }

  if (heating && reedStatus && (setBlow != lastSetBlow)) { //commit new pwm for blower
    pwmWrite(BLOWER, map(setBlow, 0, 100, 0, 2520));
    lastSetBlow = setBlow;
  }

  if (blowerOn && !heating && readTemp(1) < SHUTDOWNTEMP) { //turn blower off
    pwmWrite(BLOWER, 0);//turn off blower
    digitalWrite(HEATER, LOW); //turn off heater, just to be triple safe
    printNumber(MAIN, handleTempUnit(setTemp, otherSettings.tempUnit)); //display set temp on main section
    blowerOn = false;
    if (reedStatus) {
      ht.writeMem(23, 0); //turn of temp icon
      changeSegment(24, 0, 0); //turn off ºC
      changeSegment(24, 1, 0); //turn off ºF
      changeSegment(31, 0, 0); //turn off Set
      changeSegment(31, 1, 1); //turn on fan icon
      printNumber(LEFT, setBlow); //display set blower percentage
      setPointChanged = 2;
      if (setTimer != 0) {
        printNumber(RIGHT, setTimer);
        changeSegment(10, 3, 1); //enable S icon
        coolingAfterTimer = false;
      }
      else {
        for (byte i = 11; i < 17; i++) { //disable timer number segments
          ht.writeMem(i, B0000);
        }
        changeSegment(16, 3, 1); //enable OFF icon
        changeSegment(10, 3, 0); //disable S icon
      }
    }
  }

  if (heating && millisecs - lastTempIcon >= 1000) { //increase thermometer icon segments and display set blower/temperature on left screen section
    int current = ht.readMem(23); //read current segment state
    if (current != B1111) { //if they aren't all on
      current *= -1; //turn number negative to be able to shift 1s to the right instead of 0s (change bit sign)
      current >>= 1; // Shift to the right (for example 1000 turns 1100)
      current *= -1; // turn number positive again
      bitSet(current, 3); // turn on the 3rd bit (thermometer icon and first segment)
      ht.writeMem(23, current); //write
    }
    else ht.writeMem(23, B1000); //turn all segments off
    if (!selectedSection && !setPointReached) { //toggle between set blower value and set temperature while the station is trying to reach setPoint and no settings section is selected
      if (switchDisplayed) {
        changeSegment(24, 0, 0); //turn off ºC
        changeSegment(24, 1, 0); //turn off ºF
        changeSegment(31, 0, 1); //turn on Set
        changeSegment(31, 1, 1); //turn on fan icon
        printNumber(LEFT, setBlow); //print set blower
      }
      else {
        changeSegment(31, 0, 1); //turn on Set
        changeSegment(31, 1, 0); //turn off fan icon
        if (otherSettings.tempUnit) { //turn on ºC
          changeSegment(24, 1, 0); //turn off ºF
          changeSegment(24, 0, 1); //turn on ºC
        }
        else { //turn on ºF
          changeSegment(24, 0, 0); //turn off ºC
          changeSegment(24, 1, 1); //turn on ºF
        }
        printNumber(LEFT, handleTempUnit(setTemp, otherSettings.tempUnit)); //print temperature
      }
      switchDisplayed = !switchDisplayed;
    }
    lastTempIcon = millisecs;
  }

  else if (!heating && currentTemp >= MINTEMP && millisecs - lastTempIcon >= 1000) { //decrease thermometer icon segments
    int current = ht.readMem(23);
    if (current != B1000) {
      current <<= 1;
      ht.writeMem(23, current);
    }
    else ht.writeMem(23, B1111);
    lastTempIcon = millisecs;
  }

  if (!selectedSection && !setPointReached && heating && millis() - potMillis >= 1000 && ((setPointChanged == 1 && readTemp(1) <= calibrateTemp(1)) || (setPointChanged == 2 && readTemp(1) >= calibrateTemp(1))))  {//beep and start counting when setPoint is reached
    setPointReached = true;
    printNumber(MAIN, handleTempUnit(setTemp, otherSettings.tempUnit)); //display set temp on main section
    changeSegment(24, 0, 0); //turn off ºC
    changeSegment(24, 1, 0); //turn off ºF
    changeSegment(31, 0, 1); //turn on Set
    changeSegment(31, 1, 1); //turn on fan icon
    printNumber(LEFT, setBlow); //print blower value on left section
    if (otherSettings.buzzer) { //if sound is active buzz
      tone(PIEZO, 1000, 100);
      lastToneMillis = millis();
    }
    setPointReachedTime = millis();
    timerTemporary = setTimer;
    timerSeconds.refresh();
    timerSeconds.attachInterrupt(TIMER_CH2, timerHandler);
    changeSegment(10, 3, 1); //enable S icon
    printNumber(RIGHT, 0);
    setPointChanged = false;
    newPotValue = false;
  }

  if (otherSettings.serialOutput && millis() - lastSerialOutput >= SERIALTIME) { //this can be used to overlay settings/temperature to OBS for example, just make the Serial.println you want inside this if statement make sure you use a real serial terminal for this like putty, arduino serial monitor won't work with the commands
    Serial.write(27);       // ESC command
    Serial.print("[1;31m"); // red foreground (text)
    Serial.write(27);       // ESC command
    Serial.print("[2J");    // clear screen
    Serial.write(27);       // ESC command
    Serial.print("[H");     // cursor to home
    Serial.print("Temp: ");
    if (heating || blowerOn) {
      if (!setPointReached) Serial.print(calibrateTemp(0));
      else Serial.print(handleTempUnit(setTemp, otherSettings.tempUnit));
    }
    else Serial.print("OFF");
    Serial.print("/");
    if (heating) {
      Serial.print(handleTempUnit(setTemp, otherSettings.tempUnit));
      if (otherSettings.tempUnit) Serial.println("ºC");
      else Serial.println("ºF");
    }
    else Serial.println("OFF");
    Serial.print("Blower: ");
    Serial.print(setBlow);
    Serial.println("%");

    //print whatever other variable you want like setTimer, timerTemporary, standby, otherSettings.selectedCh, and so on.
    //make sure you use a real serial terminal for this like putty, arduino serial monitor won't work with the commands

    lastSerialOutput = millis();
  }
}
