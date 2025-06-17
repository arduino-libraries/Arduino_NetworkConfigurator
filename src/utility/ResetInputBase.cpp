/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "ANetworkConfigurator_Config.h"
#if NETWORK_CONFIGURATOR_COMPATIBLE

#include "ResetInputBase.h"
#include "ResetInput.h"


#if defined(ARDUINO_PORTENTA_H7_M7)
#include "ResetInput_PortentaMachineControl.h"
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

ResetInputBase &ResetInputBase::getInstance() {
  static ResetInputBase instance;
  return instance;
}

void ResetInputBase::begin() {

  ResetInputCallback pressedCustomCallback = _pressedCustomCallback;
  int pin = _pin;

  #if defined(ARDUINO_PORTENTA_H7_M7)
  if(isPortentaMachineControlAttached()) {
    _instance = &ResetInput_PortentaMachineControl::getInstance();
  }else{
    _instance = &ResetInput::getInstance();
  }
  #else
  _instance = &ResetInput::getInstance();
  #endif
  if(pin != -2) {
    _instance->setPin(pin);
  }

  if(pressedCustomCallback) {
    _instance->setPinChangedCallback(pressedCustomCallback);
  }

  _instance->begin();
}

bool ResetInputBase::isEventFired() {
  if (_instance == nullptr) {
    return false;
  }
  return _instance->isEventFired();
}

void ResetInputBase::setPinChangedCallback(ResetInputCallback callback) {
  _pressedCustomCallback = callback;
}

void ResetInputBase::setPin(int pin) {
  if(pin < 0){
    _pin = DISABLE_PIN;
  }else {
    _pin = pin;
  }
}

ResetInputBase::ResetInputBase() {
  _instance = nullptr;
  _pressedCustomCallback = nullptr;
  _pin = -2;
}

#endif
