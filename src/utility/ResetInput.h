/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "ANetworkConfigurator_Config.h"
#include "ResetInputBase.h"
#include "LEDFeedback.h"

class ResetInput : public ResetInputBase {
public:
  static ResetInput &getInstance() {
    static ResetInput instance;
    return instance;
  }

  void begin() override {
    if(_pin == DISABLE_PIN){
      return;
    }
  #ifdef ARDUINO_OPTA
    pinMode(_pin, INPUT);
  #else
    pinMode(_pin, INPUT_PULLUP);
  #endif
    pinMode(LED_RECONFIGURE, OUTPUT);
    digitalWrite(LED_RECONFIGURE, LED_OFF);
    attachInterrupt(digitalPinToInterrupt(_pin),_pressedCallback, CHANGE);
  }

  bool isEventFired() override  {
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

private:
  ResetInput() {
    _pin = PIN_RECONFIGURE;
    _expired = false;
    _startPressed = 0;
    _fireEvent = false;
  }
  static inline volatile bool _expired;
  static inline volatile bool _fireEvent;
  static inline volatile uint32_t _startPressed;
  /**
   * @brief Internal callback function to handle pin press events.
   */
  static void _pressedCallback() {
  #if defined(ARDUINO_NANO_RP2040_CONNECT)
    if(digitalRead(_pin) == HIGH){
  #else
    if(digitalRead(_pin) == LOW){
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
};

