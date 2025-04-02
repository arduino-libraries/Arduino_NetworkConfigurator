/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "ANetworkConfigurator_Config.h"
#if NETWORK_CONFIGURATOR_COMPATIBLE

#include "ResetInput.h"
#include "Utility/LEDFeedback/LEDFeedback.h"

ResetInput &ResetInput::getInstance() {
  static ResetInput instance;
  return instance;
}

ResetInput::ResetInput():
  _pin {PIN_RECONFIGURE}
  {
    _pressed = false;
    _startPressed = 0;
    _fireEvent = false;
    _pressedCustomCallback = nullptr;
  }

void ResetInput::begin() {
#ifdef ARDUINO_OPTA
  pinMode(PIN_RECONFIGURE, INPUT);
#else
  pinMode(PIN_RECONFIGURE, INPUT_PULLUP);
#endif
  pinMode(LED_RECONFIGURE, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(PIN_RECONFIGURE),_pressedCallback, CHANGE);
}

bool ResetInput::isEventFired() {
  if(_pressed){
#if defined(ARDUINO_NANO_RP2040_CONNECT) || defined(ARDUINO_SAMD_MKRWIFI1010)
    LEDFeedbackClass::getInstance().stop();
#endif
    if(micros() - _startPressed > RESET_HOLD_TIME){
#if defined(ARDUINO_PORTENTA_H7_M7) || defined(ARDUINO_GIGA)
    digitalWrite(LED_RECONFIGURE, HIGH);
#else
    digitalWrite(LED_RECONFIGURE, LOW);
#endif
    }
  }

  return _fireEvent;
}

void ResetInput::setPinChangedCallback(void (*callback)()) {
  _pressedCustomCallback = callback;
}

void ResetInput::setPin(uint32_t pin) {
  _pin = pin;
}

void ResetInput::_pressedCallback() {
#if defined(ARDUINO_NANO_RP2040_CONNECT)
  if(digitalRead(PIN_RECONFIGURE) == HIGH){
#else
  if(digitalRead(PIN_RECONFIGURE) == LOW){
#endif
#if !defined(ARDUINO_NANO_RP2040_CONNECT) && !defined(ARDUINO_SAMD_MKRWIFI1010)
    LEDFeedbackClass::getInstance().stop();
#endif
    _startPressed = micros();
    _pressed = true;
#if defined(ARDUINO_PORTENTA_H7_M7) || defined(ARDUINO_GIGA)
    digitalWrite(LED_RECONFIGURE, LOW);
#else
    digitalWrite(LED_RECONFIGURE, HIGH);
#endif
  } else {
#if defined(ARDUINO_PORTENTA_H7_M7) || defined(ARDUINO_GIGA)
    digitalWrite(LED_RECONFIGURE, HIGH);
#else
    digitalWrite(LED_RECONFIGURE, LOW);
#endif

    _pressed = false;
    if(_startPressed != 0 && micros() - _startPressed > RESET_HOLD_TIME){
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
