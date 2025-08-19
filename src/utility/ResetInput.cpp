/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "ANetworkConfigurator_Config.h"
#if NETWORK_CONFIGURATOR_COMPATIBLE

#include "ResetInput.h"
#include "utility/LEDFeedback.h"

#if defined(ARDUINO_NICLA_VISION)
#include <pinDefinitions.h>
#endif

#if defined(ARDUINO_PORTENTA_H7_M7)
#include <Wire.h>


bool isPortentaMachineControlAttached() {
  Wire.begin();
  Wire.beginTransmission(I2C_ADD_DETECT_MACHINE_CONTROL_1);
  if (Wire.endTransmission() != 0) {
    return false;
  }

  Wire.beginTransmission(I2C_ADD_DETECT_MACHINE_CONTROL_2);
  if (Wire.endTransmission() != 0) {
    return false;
  }

  Wire.end();
  return true;
}

#endif

ResetInput &ResetInput::getInstance() {
  static ResetInput instance;
  return instance;
}

ResetInput::ResetInput() {
  #if defined(ARDUINO_NICLA_VISION)
  _pin = PinNameToIndex(PIN_RECONFIGURE);
  #else
  _pin = PIN_RECONFIGURE;
  #endif
  _ledFeedbackPin = LED_RECONFIGURE;
  _expired = false;
  _startPressed = 0;
  _fireEvent = false;
  _pressedCustomCallback = nullptr;
}

void ResetInput::begin() {
#if defined(ARDUINO_PORTENTA_H7_M7)
  if(isPortentaMachineControlAttached()) {
    return; // Portenta Machine Control is not supported
  }
#endif

  if(_pin == DISABLE_PIN){
    return;
  }
#ifdef ARDUINO_OPTA
  pinMode(_pin, INPUT);
  if(_getPid_() != OPTA_WIFI_PID){
    _ledFeedbackPin = GREEN_LED;
  }
#else
  pinMode(_pin, INPUT_PULLUP);
#endif
  pinMode(_ledFeedbackPin, OUTPUT);
  digitalWrite(_ledFeedbackPin, LED_OFF);
  attachInterrupt(digitalPinToInterrupt(_pin),_pressedCallback, CHANGE);
}

bool ResetInput::isEventFired() {
  if(_startPressed != 0){
#if defined(ARDUINO_NANO_RP2040_CONNECT) || defined(ARDUINO_SAMD_MKRWIFI1010)
    LEDFeedbackClass::getInstance().stop();
#endif
    if(micros() - _startPressed > RESET_HOLD_TIME){
      digitalWrite(_ledFeedbackPin, LED_OFF);
      _expired = true;
    }
  }

  return _fireEvent;
}

void ResetInput::setPinChangedCallback(ResetInputCallback callback) {
  _pressedCustomCallback = callback;
}

void ResetInput::setPin(int pin) {
  if(pin < 0){
    _pin = DISABLE_PIN;
  }else {
    _pin = pin;
  }
}

void ResetInput::_pressedCallback() {
#if defined(ARDUINO_NANO_RP2040_CONNECT)
  if(digitalRead(_pin) == HIGH){
#else
  if(digitalRead(_pin) == LOW){
#endif
#if !defined(ARDUINO_NANO_RP2040_CONNECT) && !defined(ARDUINO_SAMD_MKRWIFI1010)
    LEDFeedbackClass::getInstance().stop();
#endif
    _startPressed = micros();
    digitalWrite(_ledFeedbackPin, LED_ON);
  } else {
    digitalWrite(_ledFeedbackPin, LED_OFF);
    if(_startPressed != 0 && _expired){
      _fireEvent = true;
    }else{
      LEDFeedbackClass::getInstance().restart();
    }
    _startPressed = 0;
  }

  if (_pressedCustomCallback) {
    _pressedCustomCallback();
  }
}

#endif // NETWORK_CONFIGURATOR_COMPATIBLE
