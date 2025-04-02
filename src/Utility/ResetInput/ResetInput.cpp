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
    _expired = false;
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
  digitalWrite(LED_RECONFIGURE, LED_OFF);

  attachInterrupt(digitalPinToInterrupt(PIN_RECONFIGURE),_pressedCallback, CHANGE);
}

bool ResetInput::isEventFired() {
  if(_startPressed != 0){
#if defined(ARDUINO_NANO_RP2040_CONNECT) || defined(ARDUINO_SAMD_MKRWIFI1010)
    LEDFeedbackClass::getInstance().stop();
#endif
    if(micros() - _startPressed > RESET_HOLD_TIME){
      digitalWrite(LED_RECONFIGURE, LED_OFF);
      _expired = true;
    }
  }

  return _fireEvent;
}

void ResetInput::setPinChangedCallback(ResetInputCallback callback) {
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
    digitalWrite(LED_RECONFIGURE, LED_ON);
  } else {
    digitalWrite(LED_RECONFIGURE, LED_OFF);
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
