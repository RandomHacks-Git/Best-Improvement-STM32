void clearDigit(byte section, byte digitNumber) {
  byte address;
  switch (section) {
    case MAIN:
      if (digitNumber == 1) address = 17;
      if (digitNumber == 2) address = 19;
      if (digitNumber == 3) address = 21;
      break;
    case LEFT:
      if (digitNumber == 1) address = 25;
      if (digitNumber == 2) address = 27;
      if (digitNumber == 3) address = 29;
      break;
    case RIGHT:
      if (digitNumber == 1) address = 11;
      if (digitNumber == 2) address = 13;
      if (digitNumber == 3) address = 15;
      break;
    case SMALL:
      if (digitNumber == 1) address = 8;
      break;
  }
  ht.writeMem(address, B0000);
  ht.writeMem(address + 1, B0000);
}

byte printNumber(byte section, short number) {
  if ((number > 999 && otherSettings.tempUnit) || (number < 0 && selectedSection != 4)) return 0;
  else if (number > 999 && (section == MAIN || section == LEFT)) number = 999;
  if (section == SMALL && number > 9) return 0;
  byte hundreds, tens, units;
  if (number < 0)number *= -1; //turn number positive, negative numbers are only used for temperature calibration

  if (section != SMALL && number > 99) {
    hundreds = ((number / 100) % 10);
    tens = ((number / 10) % 10);
    units = number % 10;
    if (section == MAIN) {
      digitPrint(17, units);
      digitPrint(19, tens);
      digitPrint(21, hundreds);
      if (standby) changeSegment(22, 3, 1); //turn the moon/star back on if in standby
    }
    else if (section == LEFT) {
      digitPrint(25, units);
      digitPrint(27, tens);
      digitPrint(29, hundreds);
    }
    else if (section == RIGHT) {
      digitPrint(11, units);
      digitPrint(13, tens);
      digitPrint(15, hundreds);
      if (!timer) changeSegment(16, 3, 1); //turn the "off" icon back on if timer is off
    }
  }
  else if (section != SMALL && number > 9) {
    tens = number / 10;
    units = number % 10;
    clearDigit(section, 3);
    if (section == MAIN) {
      digitPrint(17, units);
      digitPrint(19, tens);
      if (standby) changeSegment(22, 3, 1); //turn the moon/star back on if in standby
    }
    else if (section == LEFT) {
      digitPrint(25, units);
      digitPrint(27, tens);
      if (selectedSection == 4 && handleTempUnit(otherSettings.calTemp, otherSettings.tempUnit) < 0) changeSegment(30, 1, 1); //enable negative sign
    }
    else if (section == RIGHT) {
      digitPrint(11, units);
      digitPrint(13, tens);
      if (!timer) changeSegment(16, 3, 1); //turn the "off" icon back on if timer is off
    }
  }
  else {
    units = number;
    if (section != SMALL) {
      clearDigit(section, 3);
      clearDigit(section, 2);
    }
    if (section == MAIN) {
      digitPrint(17, units);
      if (standby) changeSegment(22, 3, 1); //turn the moon/star back on if in standby
    }
    else if (section == LEFT) {
      digitPrint(25, units);
      if (selectedSection == 4 && otherSettings.calTemp < 0) changeSegment(28, 1, 1); //enable negative sign
    }
    else if (section == RIGHT) {
      digitPrint(11, units);
      if (!timer) changeSegment(16, 3, 1); //turn the "off" icon back on if timer is off
    }
    else if (section == SMALL) {
      digitPrint(8, units);
    }
  }
  return 1;
}

//print digit

void digitPrint(byte address, byte number) {
  switch (number) {
    case 0:
      ht.writeMem(address, B1111);
      ht.writeMem(address + 1, B0101);
      break;
    case 1:
      ht.writeMem(address, B0110);
      ht.writeMem(address + 1, B0000);
      break;
    case 2:
      ht.writeMem(address, B1011);
      ht.writeMem(address + 1, B0110);
      break;
    case 3:
      ht.writeMem(address, B1111);
      ht.writeMem(address + 1, B0010);
      break;
    case 4:
      ht.writeMem(address, B0110);
      ht.writeMem(address + 1, B0011);
      break;
    case 5:
      ht.writeMem(address, B1101);
      ht.writeMem(address + 1, B0011);
      break;
    case 6:
      ht.writeMem(address, B1101);
      ht.writeMem(address + 1, B0111);
      break;
    case 7:
      ht.writeMem(address, B0111);
      ht.writeMem(address + 1, B0000);
      break;
    case 8:
      ht.writeMem(address, B1111);
      ht.writeMem(address + 1, B0111);
      break;
    case 9:
      ht.writeMem(address, B1111);
      ht.writeMem(address + 1, B0011);
      break;
  }
}

void printText(byte address, char text[], bool loopText) { //this function is blocking!! modify and use timer interrupt or handle inside loop if needed, since I'm only using it to show errors there is no need
  digitalWrite(HEATER, LOW); //in case you did not read/understand the above comment :)
  text = strlwr(text);
  uint8_t textLength = strlen(text);
  //  char textBuffer[textLength + 4];
  //  char finalSpaces[4] = "   ";
  //  strcat(textBuffer, finalSpaces);

  do {
    for (int i = 0; i < textLength; i++) {
      printLetter(address, text[i]);
      if (i > 0) printLetter(address + 2, text[i - 1]); //shift letter to next 7 segments on the left like ■■a -> ■ab if text[i] = b
      if (i > 1) printLetter(address + 4, text[i - 2]); //shift letter to next 7 segments on the left like ■ab -> abc if text[i] = c
      //Serial.println(text[i]);
      delay(200);
    }
  } while (loopText);
}

void printLetter(byte address, char letter) {
  switch (letter) {
    case '#':
      ht.writeMem(address, B0000);
      ht.writeMem(address + 1, B0000);
      break;
    case '0':
      ht.writeMem(address, B1111);
      ht.writeMem(address + 1, B0101);
      break;
    case '1':
      ht.writeMem(address, B0110);
      ht.writeMem(address + 1, B0000);
      break;
    case '2':
      ht.writeMem(address, B1011);
      ht.writeMem(address + 1, B0110);
      break;
    case '3':
      ht.writeMem(address, B1111);
      ht.writeMem(address + 1, B0010);
      break;
    case '4':
      ht.writeMem(address, B0110);
      ht.writeMem(address + 1, B0011);
      break;
    case '5':
      ht.writeMem(address, B1101);
      ht.writeMem(address + 1, B0011);
      break;
    case '6':
      ht.writeMem(address, B1101);
      ht.writeMem(address + 1, B0111);
      break;
    case '7':
      ht.writeMem(address, B0111);
      ht.writeMem(address + 1, B0000);
      break;
    case '8':
      ht.writeMem(address, B1111);
      ht.writeMem(address + 1, B0111);
      break;
    case '9':
      ht.writeMem(address, B1111);
      ht.writeMem(address + 1, B0011);
      break;
    case ' ':
      ht.writeMem(address, B0000);
      ht.writeMem(address + 1, B0000);
      break;
    case 'a':
      ht.writeMem(address, B0111);
      ht.writeMem(address + 1, B0111);
      break;
    case 'b':
      ht.writeMem(address, B1100);
      ht.writeMem(address + 1, B0111);
      break;
    case 'c':
      ht.writeMem(address, B1001);
      ht.writeMem(address + 1, B0101);
      break;
    case 'd':
      ht.writeMem(address, B1110);
      ht.writeMem(address + 1, B0110);
      break;
    case 'e':
      ht.writeMem(address, B1001);
      ht.writeMem(address + 1, B0111);
      break;
    case 'f':
      ht.writeMem(address, B0001);
      ht.writeMem(address + 1, B0111);
      break;
    case 'g':
      ht.writeMem(address, B1111);
      ht.writeMem(address + 1, B0011);
      break;
    case 'h':
      ht.writeMem(address, B0100);
      ht.writeMem(address + 1, B0111);
      break;
    case 'i':
      ht.writeMem(address, B0110);
      ht.writeMem(address + 1, B0000);
      break;
    case 'j':
      ht.writeMem(address, B1110);
      ht.writeMem(address + 1, B0000);
      break;
    case 'k':
      ht.writeMem(address, B0110);
      ht.writeMem(address + 1, B0111);
      break;
    case 'l':
      ht.writeMem(address, B1000);
      ht.writeMem(address + 1, B0101);
      break;
    case 'm':
      ht.writeMem(address, B0111);
      ht.writeMem(address + 1, B0101);
      break;
    case 'n':
      ht.writeMem(address, B0100);
      ht.writeMem(address + 1, B0110);
      break;
    case 'o':
      ht.writeMem(address, B1100);
      ht.writeMem(address + 1, B0110);
      break;
    case 'p':
      ht.writeMem(address, B0011);
      ht.writeMem(address + 1, B0111);
      break;
    case 'q':
      ht.writeMem(address, B0111);
      ht.writeMem(address + 1, B0011);
      break;
    case 'r':
      ht.writeMem(address, B0000);
      ht.writeMem(address + 1, B0110);
      break;
    case 's':
      ht.writeMem(address, B1101);
      ht.writeMem(address + 1, B0011);
      break;
    case 't':
      ht.writeMem(address, B1000);
      ht.writeMem(address + 1, B0111);
      break;
    case 'u':
      ht.writeMem(address, B1100);
      ht.writeMem(address + 1, B0100);
      break;
    case 'v':
      ht.writeMem(address, B1110);
      ht.writeMem(address + 1, B0101);
      break;
    case 'w':
      ht.writeMem(address, B1110);
      ht.writeMem(address + 1, B0111);
      break;
    case 'x':
      ht.writeMem(address, B0110);
      ht.writeMem(address + 1, B0111);
      break;
    case 'y':
      ht.writeMem(address, B1110);
      ht.writeMem(address + 1, B0011);
      break;
    case 'z':
      ht.writeMem(address, B1011);
      ht.writeMem(address + 1, B0110);
      break;
  }
}

void printChannel (byte channel) {
  switch (channel) {
    case 0: //Print P (for Potentiometer)
      ht.writeMem(8, B0011);
      ht.writeMem(9, B1111);
      break;
    case 1: //Print 1
      ht.writeMem(8, B0110);
      ht.writeMem(9, B1000);
      break;
    case 2: //Print 2
      ht.writeMem(8, B1011);
      ht.writeMem(9, B1110);
      break;
    case 3: //Print 3
      ht.writeMem(8, B1111);
      ht.writeMem(9, B1010);
      break;
    case 4:
      ht.writeMem(8, B1000);
      ht.writeMem(9, B1111);
  }
}

void blinkSelection() {
  switch (selectedSection) {
    case 1:
      if (!sectionOff) { //turn off section
        for (byte i = 17; i < 23; i++) {
          ht.writeMem(i, B0000);
        }
        sectionOff = true;
      }
      else { //turn on section
        printNumber(MAIN, handleTempUnit(setTemp, otherSettings.tempUnit));
        sectionOff = false;
      }
      lastBlink = millis();
      break;

    case 2:
      printNumber(MAIN, handleTempUnit(setTemp, otherSettings.tempUnit));
      if (!sectionOff) {
        for (byte i = 25; i < 31; i++) {
          ht.writeMem(i, B0000);
        }
        sectionOff = true;
      }
      else {
        printNumber(LEFT, setBlow);
        sectionOff = false;
      }
      lastBlink = millis();
      break;

    case 3:
      printNumber(LEFT, setBlow);
      if (!sectionOff) {
        for (byte i = 11; i < 17; i++) {
          ht.writeMem(i, B0000);
        }
        if (!timer) changeSegment(16, 3, 1); //turn on "off" icon
        sectionOff = true;
      }
      else {
        printNumber(RIGHT, setTimer);
        if (!timer) changeSegment(16, 3, 1); //turn on "off" icon
        sectionOff = false;
      }
      lastBlink = millis();
      break;
    //    case 4:
    //      printNumber(RIGHT, setTimer);
    //      if (!sectionOff) {
    //        changeSegment(10, 2, 0); ////disable M icon
    //        changeSegment(10, 3, 0); //disable S icon
    //        sectionOff = true;
    //      }
    //      else {
    //        if (timeUnit) {
    //          changeSegment(10, 2, 0); ////disable M icon
    //          changeSegment(10, 3, 1); //enable S icon
    //        }
    //        else {
    //          changeSegment(10, 3, 0); ////disable S icon
    //          changeSegment(10, 2, 1); //enable M icon
    //        }
    //        sectionOff = false;
    //      }
    //      lastBlink = millis();
    //      break;
    case 4:
      if (!sectionOff) {
        for (byte i = 25; i < 31; i++) {
          ht.writeMem(i, B0000);
        }
        sectionOff = true;
      }
      else {
        if (otherSettings.tempUnit) printNumber(LEFT, otherSettings.calTemp);
        else printNumber(LEFT, otherSettings.calTemp * 1.8);
        //printNumber(LEFT, handleTempUnit(otherSettings.calTemp, otherSettings.tempUnit));
        //handle CF icon function thingymabob
        sectionOff = false;
      }
      lastBlink = millis();
      break;
  }
}


void stopBlinking() {
  printNumber(MAIN, handleTempUnit(setTemp, otherSettings.tempUnit));
  printNumber(LEFT, setBlow);
  if (!heating) {
    if (timer) {
      printNumber(RIGHT, setTimer);
      if (timeUnit) changeSegment(10, 3, 1); //enable S icon
      //else changeSegment(10, 2, 1); //enable M icon
    }
    else {
      ht.writeMem(11, 0); //disable timer section
      ht.writeMem(12, 0);
      //changeSegment(10, 2, 0); //disable M icon
      changeSegment(10, 3, 0); //disable S icon
    }
  }
  selectedSection = 0;
  if (otherSettings.selectedCh == 4) {
    touchSettings.temp = setTemp;
    touchSettings.blow = setBlow;
    EEPROM_put(16, touchSettings);
    EEPROM_put(20, otherSettings);
  }
}

void changeSegment(byte address, byte bit, bool value) { //changes a single segment
  byte nowSet = ht.readMem(address); //check what is currently being displayed on address so we only change what we want
  nowSet = bitWrite(nowSet, bit, value); //change only the segment we want (bit)
  ht.writeMem(address, nowSet);
}
