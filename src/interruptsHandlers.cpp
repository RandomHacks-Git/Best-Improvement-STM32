#include "global.h"

void touchAction() {
  if (!digitalRead(TOUCHINT)) {
    touchMillis = millis();
    touched = true;
    touchReleased = false;
    readTouchFlag = true; //because using i2c inside the interrupt is probably not a good idea
    if (otherSettings.buzzer && millis() - lastReact >= 100) toneFlag = true;
  }
  else {
    touchReleased = true;
    if (touchedButton == UP || touchedButton == DOWN) {
      touched = false;
      touchedButton = 0;
    }
  }
}

void reedAction() {
  reedStatus = digitalRead(REEDINT);
  reedFlag = true;
}

void btnAction() {
  if (millis() - btnMillis >= DEBOUNCETIME) {
    if (!buttonFlag) {
      btn1 = !digitalRead(BTN1);
      btn2 = !digitalRead(BTN2);
      btn3 = !digitalRead(BTN3);
      if (btn1 || btn2 || btn3) {
        buttonFlag = true;
        if (otherSettings.buzzer)toneFlag = true;
      }
    }
  }
  btnMillis = millis();
}

void timerHandler() {
  if (timer) {
    if (timerTemporary < setTimer || millis() - setPointReachedTime >= 900) {
      if (timerTemporary - 1 > 0) {
        timerTemporary -= 1;
      }
      else {
        timerTemporary = 0;
        if (otherSettings.buzzer)longToneFlag = true;
        timerSeconds.detachInterrupt(TIMER_CH2);
        myPID.SetMode(MANUAL); //turn off PID
        digitalWrite(HEATER, LOW); //turn off heater
        //pwmWrite(BLOWER, 2520); //setting the blower to max like this might blow components off at the end of the timer!
        heating = false;
        coolingAfterTimer = true;
        setPointReached = false;
        setPointChanged = 2;
      }
    }
  }
  else if (timerTemporary > 0 || millis() - setPointReachedTime >= 900) timerTemporary++; // >>>>>>>>>>>>>>>>>>>>>>>>>> manage seconds/minutes?
  timerFlag = true;
}
