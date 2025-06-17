/*
  Copyright (c) 2025 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "ANetworkConfigurator_Config.h"
#include <Arduino_PortentaMachineControl.h>
#include "ResetInputBase.h"
#include "LEDFeedback.h"

#define PORTENTA_MACHINE_CONTROL_DI_INTERRUPT PB_4
#define PIN_RECONFIGURE_MACHINE_CONTROL DIN_READ_CH_PIN_00

class ResetInput_PortentaMachineControl : public ResetInputBase {
public:
  static ResetInput_PortentaMachineControl &getInstance() {
    static ResetInput_PortentaMachineControl instance;
    return instance;
  }

  void begin() override {
    if (_pin == DISABLE_PIN) {
      return;
    }

    pinMode(LED_RECONFIGURE, OUTPUT);
    digitalWrite(LED_RECONFIGURE, LED_OFF);
    Wire.begin();
    attachInterrupt(PORTENTA_MACHINE_CONTROL_DI_INTERRUPT, _inputCallback, FALLING);
    MachineControl_DigitalInputs.begin();
  }

  bool isEventFired() override {
    if(_interruptFlag) {
      bool pinChanged = false;
      _interruptFlag = false;
      int pinStatus = MachineControl_DigitalInputs.read(_pin);
      if(pinStatus == -1) {
        return false;
      } else {
        pinChanged = (pinStatus != _pinStatus);
        _pinStatus = pinStatus;
      }

      if (!pinChanged) {
        return false;
      }

      if (pinStatus == 1) {
        LEDFeedbackClass::getInstance().stop();
        digitalWrite(LED_RECONFIGURE, LED_ON);
        _startPressedTs = _interruptEventTs;

      } else {
        // Pin released
        if(_startPressedTs != 0){
          digitalWrite(LED_RECONFIGURE, LED_OFF);
          if(_interruptEventTs - _startPressedTs > RESET_HOLD_TIME) {
            return true;
          }
          LEDFeedbackClass::getInstance().restart();
          _startPressedTs = 0;
        }
      }
    }

    if(_pinStatus == 1) {
      // Pin is pressed
      if(_startPressedTs != 0 && micros() - _startPressedTs > RESET_HOLD_TIME) {
        // Turn off the led after the hold time
        digitalWrite(LED_RECONFIGURE, LED_OFF);
      }
    }

    return false;
  }


private:
  ResetInput_PortentaMachineControl() {
    _pin = PIN_RECONFIGURE_MACHINE_CONTROL;
    _interruptFlag = false;
    _startPressedTs = 0;
    _interruptEventTs = 0;
    _pinStatus = -1;
  }
  int _pinStatus;
  uint32_t _startPressedTs;
  static inline volatile bool _interruptFlag;
  static inline volatile uint32_t _interruptEventTs;
  static void _inputCallback() {

    _interruptFlag = true;
    _interruptEventTs = micros();
    if (_pressedCustomCallback) {
      _pressedCustomCallback();
    }
  }
};
