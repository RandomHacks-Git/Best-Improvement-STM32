byte readTouch() { //ask touch ic which button was touched
  Wire.beginTransmission(touchAddress);//(0x56); Begin transmission to the ic
  Wire.write(byte(keyValues)); //Ask the particular register for data
  Wire.endTransmission(); // Ends the transmission and transmits the data from the register
  Wire.requestFrom(touchAddress, 1);  // Request the transmitted byte from the register
  if (Wire.available()) return Wire.read(); // Reads and returns the data from the register
}

void reactTouch() { //take action according to touched key
  //Serial.println(touchedButton);
  byte stepAmount = 5;
  if ((touchedButton == UP || touchedButton == DOWN) && millis() - touchMillis > 1000) {
    if (!selectedSection) {
      otherSettings.serialOutput = !otherSettings.serialOutput;
      changeSegment(7, 1, otherSettings.serialOutput); //turn "RS232 On" segment on LCD on or off depending on serialOutput bool status
      EEPROM_put(20, otherSettings); //save to eeprom
      if (otherSettings.buzzer) {
        tone(PIEZO, 4000, 70);
        delay(70);
        tone(PIEZO, 5000, 100);
        lastReact = millis();
        lastToneMillis = millis();
      }
      touched = false;
    }
    else stepAmount = 10; //increase stepAmount if any setting is selected and up or down buttons are long pressed
  }
  //if (!otherSettings.tempUnit) stepAmount /= 1.8; //so the step amount remains 5 or 10 even when set to fahrenheit LOOK FOR A BETTER SOLUTION FOR THIS (I changed stepAmount's type back to byte as I'm not using this)
  if (touchedButton == SET && millis() - touchMillis > 1000) {
    stopBlinking();
    otherSettings.buzzer = !otherSettings.buzzer;
    if (otherSettings.buzzer) {
      tone(PIEZO, 4000, 70);
      delay(70);
      tone(PIEZO, 5000, 100);
      lastToneMillis = millis();
    }
    else { //no sound, flash backlight instead?
      tone(PIEZO, 5000, 70);
      delay(70);
      tone(PIEZO, 4000, 100);
      lastToneMillis = millis();
    }
    EEPROM_put(20, otherSettings);
    touched = false;
    touchedButton = 0;
    lastReact = millis();
    selectedSection = 0;
  }

  if (touchedButton == SET && touchReleased && selectedSection != 4 && millis() - lastReact >= 200) {
    if (selectedSection < 2 || (selectedSection < 3 && !reedStatus)) { // || (timer && selectedSection < 3 && !heating) initially I planned to manage seconds and minutes so there was another section to toggle between them // timer can only be changed if the handle is in the base
      selectedSection += 1;
      printNumber(LEFT, setBlow); //if in settings menu print the blower value imediately or else if currently heating "set temperature" might appear instead
      ht.writeMem(31, B0010); //enable fan icon disable set icon
      changeSegment(24, 0, 0); //disable ºC
      changeSegment(24, 1, 0); //disable ºF
      if (!heating) {
        printNumber(RIGHT, setTimer); //timer value
        changeSegment(10, 3, 1); //enable S icon
      }
    }
    else {
      selectedSection = 0;
      stopBlinking();
    }
    touched = false;
    touchedButton = 0;
    lastReact = millis();
  }

  else if (touchedButton == UP) { //UP BUTTON
    if (selectedSection == 1) { //temp
      if (setTemp + stepAmount < MAXTEMP) {
        setTemp += stepAmount;
        if (otherSettings.selectedCh != 4) setTemp = 5 * int(setTemp / 5); //if it is the first time I modify the temperature with the touch channel (otherSettings.selectedCh isn't 4 yet) I round up the value to the nearest multiple of 5
      }

      else {
        setTemp = MAXTEMP;
        if (otherSettings.buzzer) {
          tone(PIEZO, 1000, 100);
          lastToneMillis = millis();
        }
      }
      printNumber(MAIN, handleTempUnit(setTemp, otherSettings.tempUnit));
      printChannel(4); //touch channel
      otherSettings.selectedCh = 4;
      lastReact = millis();
      setPointReached = false;
      if (heating) setPointChanged = 2;
    }
    else if (selectedSection == 2) { //blower
      if (setBlow + stepAmount / 5 < MAXBLOW) setBlow += stepAmount / 5;
      else {
        setBlow = MAXBLOW;
        if (otherSettings.buzzer) {
          tone(PIEZO, 1000, 100);
          lastToneMillis = millis();
        }
      }
      printNumber(LEFT, setBlow);
      printChannel(4); //touch channel
      otherSettings.selectedCh = 4;
      lastReact = millis();
    }
    else if (selectedSection == 3) { //timer
      if (setTimer + stepAmount / 5 <= 999) {
        setTimer += stepAmount / 5;
      }
      else {
        setTimer = 999;
        if (otherSettings.buzzer) {
          tone(PIEZO, 1000, 100);
          lastToneMillis = millis();
        }
      }
      timer = true;
      timerTemporary = setTimer;
      printNumber(RIGHT, setTimer);
      changeSegment(16, 3, 0); //disable off icon
      changeSegment(24, 3, 1);
      changeSegment(10, 3, 1); //enable S icon
      timeUnit = 0;
      lastReact = millis();
    }
    //    else if (selectedSection == 4) { //timer unit
    //      if (timeUnit) { //seconds
    //        changeSegment(10, 2, 0); ////disable M icon
    //        changeSegment(10, 3, 1); //enable S icon
    //        timeUnit = 0;
    //      }
    //      else { //minutes
    //        changeSegment(10, 3, 0); ////disable s icon
    //        changeSegment(10, 2, 1); //enable M icon
    //        timeUnit = 1;
    //      }
    //      touched = false;
    //      lastReact = millis();
    //    }
    else if (selectedSection == 4) { //temperature calibration
      if (otherSettings.calTemp < 50)otherSettings.calTemp += 1;
      else if (otherSettings.buzzer) {
        tone(PIEZO, 1000, 100);
        lastToneMillis = millis();
      }
      if (otherSettings.tempUnit) printNumber(LEFT, otherSettings.calTemp); //replace two lines with function? this code is repeated in lcdStuff tab - blinkSelection function, also below
      else printNumber(LEFT, otherSettings.calTemp * 1.8);
      lastReact = millis();
    }
  }
  else if (touchedButton == DOWN) { //DOWN BUTTON
    if (selectedSection == 1) {
      if (setTemp - stepAmount > MINTEMP) {
        setTemp -= stepAmount;
        if (otherSettings.selectedCh != 4) setTemp = 5 * round(setTemp / 5.0F + 0.4); //if it is the first time I modify the temperature with the touch channel (otherSettings.selectedCh isn't 4 yet) I round up the value to the nearest multiple of 5
      }
      else {
        setTemp = MINTEMP;
        if (otherSettings.buzzer) {
          tone(PIEZO, 1000, 100);
          lastToneMillis = millis();
        }
      }
      printNumber(MAIN, handleTempUnit(setTemp, otherSettings.tempUnit));
      printChannel(4); //touch channel
      otherSettings.selectedCh = 4;
      lastReact = millis();
      setPointReached = false;
      if (heating) setPointChanged = 1;
    }
    else if (selectedSection == 2) {
      if (setBlow - stepAmount / 5 > MINBLOW) setBlow -= stepAmount / 5;
      else {
        setBlow = MINBLOW;
        if (otherSettings.buzzer) {
          tone(PIEZO, 1000, 100);
          lastToneMillis = millis();
        }
      }
      printNumber(LEFT, setBlow);
      printChannel(4); //touch channel
      otherSettings.selectedCh = 4;
      lastReact = millis();
    }
    else if (selectedSection == 3) { //timer
      if (setTimer - stepAmount / 5 > 0) {
        setTimer -= stepAmount / 5;
        printNumber(RIGHT, setTimer);
      }
      else {
        setTimer = 0;
        timerTemporary = 0;
        timer = false;
        printNumber(RIGHT, setTimer);
        changeSegment(16, 3, 1); //enable OFF icon
        if (otherSettings.buzzer) {
          tone(PIEZO, 1000, 100);
          lastToneMillis = millis();
        }
      }
      timerTemporary = setTimer;
      lastReact = millis();
    }
    //    else if (selectedSection == 4) { //timer unit
    //      if (timeUnit) { //seconds
    //        changeSegment(10, 2, 0); ////disable M icon
    //        changeSegment(10, 3, 1); //enable S icon
    //        timeUnit = 0;
    //      }
    //      else { //minutes
    //        changeSegment(10, 3, 0); ////disable S icon
    //        changeSegment(10, 2, 1); //enable M icon
    //        timeUnit = 1;
    //      }
    //      touched = false;
    //      lastReact = millis();
    //    }
    else if (selectedSection == 4) { //temperature calibration
      if (otherSettings.calTemp > -50) otherSettings.calTemp -= 1;
      else if (otherSettings.buzzer) {
        tone(PIEZO, 1000, 100);
        lastToneMillis = millis();
      }
      if (otherSettings.tempUnit) printNumber(LEFT, otherSettings.calTemp); //replace two lines with function? this code is repeated in lcdStuff tab - blinkSelection function
      else printNumber(LEFT, otherSettings.calTemp * 1.8);
      lastReact = millis();
    }
  }

  else if (touchedButton == CF && touchReleased) { // ºC/ºF BUTTON
    otherSettings.tempUnit = !otherSettings.tempUnit;
    if (otherSettings.tempUnit) { //ºC
      changeSegment(10, 1, 0);
      changeSegment(10, 0, 1);
      if (selectedSection == 4) {
        changeSegment(24, 1, 0); //turn off ºF
        changeSegment(24, 0, 1); //turn on ºC
      }
    }
    else { //ºF
      changeSegment(10, 0, 0);
      changeSegment(10, 1, 1);
      if (selectedSection == 4) {
        changeSegment(24, 0, 0); //turn off ºC
        changeSegment(24, 1, 1); //turn on ºF
      }
    }
    if (!selectedSection || selectedSection == 4) printNumber(MAIN, handleTempUnit(setTemp, otherSettings.tempUnit));
    EEPROM_put(20, otherSettings);
    lastReact = millis();
    touched = false;
    touchedButton = 0;
  }
  if (touchedButton == CF && !setTimer && (!selectedSection || selectedSection == 4) && millis() - touchMillis > 1000) {
    if (!selectedSection) {
      selectedSection = 4;
      changeSegment(31, 0, 1); //turn on Set icon
      changeSegment(31, 1, 0); //disable fan icon
      changeSegment(24, 2, 1); //turn on Cal. icon
      if (otherSettings.tempUnit) { //turn on ºC
        changeSegment(24, 1, 0); //turn off ºF
        changeSegment(24, 0, 1); //turn on ºC
      }
      else { //turn on ºF
        changeSegment(24, 0, 0); //turn off ºC
        changeSegment(24, 1, 1); //turn on ºF
      }
    }
    else {
      stopBlinking();
      changeSegment(31, 0, 0); //turn off Set icon
      changeSegment(31, 1, 1); //turn on fan icon
      changeSegment(24, 2, 0); //turn off Cal. icon
      changeSegment(24, 0, 0); //turn off ºC
      changeSegment(24, 1, 0); //turn off ºF
      EEPROM_put(20, otherSettings);
    }
    if (otherSettings.buzzer) {
      tone(PIEZO, 1000, 100);
      lastToneMillis = millis();
    }
    touchedButton = 0;
    touched = false;
    lastReact = millis();
  }
}

void handleButton() {
  if (btn1) {
    if (digitalRead(BTN1)) { //short press
      if (selectedSection && selectedSection != 4) stopBlinking();
      if (otherSettings.selectedCh == 1) {
        defineBlower();
        defineTemp();
      }
      else {
        otherSettings.selectedCh = 1;
        EEPROM_put(20, otherSettings);
        printChannel(1);
        printNumber(MAIN, handleTempUnit(ch1Settings.temp, otherSettings.tempUnit));
        printNumber(LEFT, ch1Settings.blow);
        if (setTemp > ch1Settings.temp && heating) setPointChanged = 1;
        else setPointChanged = 2;
        setTemp = ch1Settings.temp;
        setBlow = ch1Settings.blow;
      }
      setPointReached = false;
      buttonFlag = false;
      btn1 = 0;
    }
    else if (millis() - btnMillis >= 1000) { //long press
      ch1Settings.temp = setTemp;
      ch1Settings.blow = setBlow;
      EEPROM_put(4, ch1Settings);
      if (otherSettings.buzzer) {
        tone(PIEZO, 4000, 50);
        delay(50);
        tone(PIEZO, 5000, 100);
        lastToneMillis = millis();
      }
      buttonFlag = false;
      btn1 = 0;
      otherSettings.selectedCh = 1;
      EEPROM_put(20, otherSettings);
      printChannel(otherSettings.selectedCh);
    }
  }
  else if (btn2) {
    if (digitalRead(BTN2)) { //short press
      if (selectedSection && selectedSection != 4) stopBlinking();
      if (otherSettings.selectedCh == 2) {
        defineBlower();
        defineTemp();
      }
      else {
        otherSettings.selectedCh = 2;
        EEPROM_put(20, otherSettings);
        printChannel(2);
        printNumber(MAIN, handleTempUnit(ch2Settings.temp, otherSettings.tempUnit));
        printNumber(LEFT, ch2Settings.blow);
        if (setTemp > ch2Settings.temp && heating) setPointChanged = 1;
        else setPointChanged = 2;
        setTemp = ch2Settings.temp;
        setBlow = ch2Settings.blow;
      }
      setPointReached = false;
      buttonFlag = false;
      btn2 = 0;
    }
    else if (millis() - btnMillis >= 1000) { //long press
      ch2Settings.temp = setTemp;
      ch2Settings.blow = setBlow;
      EEPROM_put(8, ch2Settings);
      if (otherSettings.buzzer) {
        tone(PIEZO, 4000, 50);
        delay(50);
        tone(PIEZO, 5000, 100);
        lastToneMillis = millis();
      }
      buttonFlag = false;
      btn2 = 0;
      otherSettings.selectedCh = 2;
      EEPROM_put(20, otherSettings);
      printChannel(otherSettings.selectedCh);
    }
  }
  else if (btn3) {
    if (digitalRead(BTN3)) { //short press
      if (selectedSection && selectedSection != 4) stopBlinking();
      if (otherSettings.selectedCh == 3) {
        defineBlower();
        defineTemp();
      }
      else {
        otherSettings.selectedCh = 3;
        EEPROM_put(20, otherSettings);
        printChannel(3);
        printNumber(MAIN, handleTempUnit(ch3Settings.temp, otherSettings.tempUnit));
        printNumber(LEFT, ch3Settings.blow);
        if (setTemp > ch3Settings.temp && heating) setPointChanged = 1;
        else setPointChanged = 2;
        setTemp = ch3Settings.temp;
        setBlow = ch3Settings.blow;
      }
      setPointReached = false;
      buttonFlag = false;
      btn3 = 0;
    }
    else if (millis() - btnMillis >= 1000) { //long press
      ch3Settings.temp = setTemp;
      ch3Settings.blow = setBlow;
      EEPROM_put(12, ch3Settings);
      if (otherSettings.buzzer) {
        tone(PIEZO, 4000, 50);
        delay(50);
        tone(PIEZO, 5000, 100);
        lastToneMillis = millis();
      }
      buttonFlag = false;
      btn3 = 0;
      otherSettings.selectedCh = 3;
      EEPROM_put(20, otherSettings);
      printChannel(otherSettings.selectedCh);
    }
  }
}

void defineBlower() {
  if (otherSettings.selectedCh != 0) {
    otherSettings.selectedCh = 0;
    EEPROM_put(20, otherSettings);
    printChannel(0);
  }
  blowerVal = analogRead(ABLOW);
  //Serial.println(blowerVal);
  byte tempMap = map(blowerVal, 5, 4085, MAXBLOW, MINBLOW);
  if (tempMap < MINBLOW) setBlow = MINBLOW;
  else if (tempMap > MAXBLOW) setBlow = MAXBLOW;
  else setBlow = tempMap;
  printNumber(LEFT, setBlow);
  //pwmWrite(BLOWER, map(setBlow, 0, 100, 0, 65535));
  potMillis = millis();
}

void defineTemp() {
  heaterVal = analogRead(AHEAT);
  //Serial.println(heaterVal);
  unsigned short tempMap = map(heaterVal, 3, 4085, MAXTEMP, MINTEMP);
  int change = setTemp - tempMap;
  if (millis() - potMillis <= 2000 || abs(change) >= 2) { //to avoid analogRead noise, remains responsive while turning knob but only accepts any change equal or greater than 2 if last pot change was more than 2 seconds ago
    if (otherSettings.selectedCh != 0) {
      otherSettings.selectedCh = 0;
      EEPROM_put(20, otherSettings);
      printChannel(0);
    }
    if (heating && setTemp > tempMap) setPointChanged = 1;
    else setPointChanged = 2;
    if (tempMap < MINTEMP) setTemp = MINTEMP;
    else if (tempMap > MAXTEMP) setTemp = MAXTEMP;
    else setTemp = tempMap;
    printNumber(MAIN, handleTempUnit(setTemp, otherSettings.tempUnit));
    if (heating) newPotValue = true;
  }
  potMillis = millis();
}
