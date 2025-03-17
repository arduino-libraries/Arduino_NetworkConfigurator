/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "Arduino.h"

class ResetInput{
public:
  static ResetInput& getInstance();
  // Setup the interrupt pin
  void begin();
  // Monitor if the event is fired
  bool isEventFired();
  // Add a custom function to be called when the pin status changes. It must be set before calling the begin method
  void setPinChangedCallback(void (*callback)());
  // Set the pin to be monitored. It must be set before calling the begin method
  void setPin(uint32_t pin);
private:
  ResetInput();
  static inline void (*_pressedCustomCallback)();
  uint32_t _pin;
  static inline volatile bool _pressed;
  static inline volatile bool _fireEvent;
  static inline volatile uint32_t _startPressed;
  static void _pressedCallback();
};
