/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once
#include "Arduino.h"
#include <functional>

#define DISABLE_PIN -1

/**
 * @class ResetInputBase
 * @brief A singleton class to handle input of the reset functionality with interrupt-based monitoring.
 *
 * This class provides methods to configure and monitor a reset input pin. It allows
 * setting up a custom callback function to be executed when the pin status changes.
 */
class ResetInputBase{
public:
  /**
   * @typedef ResetInputBase::ResetInputCallback
   * @brief A type definition for the callback function to be executed on pin status change.
   */
  typedef std::function<void()> ResetInputCallback;
  /**
   * @brief Get the singleton instance of the ResetInputBase class.
   * @return A reference to the ResetInputBase instance.
   */
  static ResetInputBase& getInstance();
  /**
   * @brief Initialize the reset input by setting up the interrupt pin.
   */
  virtual void begin();
  /**
   * @brief Check if the reset event has been fired.
   * @return True if the event is fired, otherwise false.
   */
  virtual bool isEventFired();
  /**
   * @brief Set a custom callback function to be called when the pin status changes.
   * This function must be called before invoking the `begin` method.
   * @param callback The custom callback function to be executed.
   */
  void setPinChangedCallback(ResetInputCallback callback);
  /**
   * @brief Set the pin to be monitored for reset events.
   * By default, the pin is set as INPUT_PULLUP.
   * Use the value DISABLE_PIN to disable the pin and the reset procedure.
   * This function must be called before invoking the `begin` method.
   * @param pin The pin number to be monitored. The pin must
   * be in the list of digital pins usable for interrupts.
   * Please refer to the Arduino documentation for more details:
   * https://docs.arduino.cc/language-reference/en/functions/external-interrupts/attachInterrupt/
   * N.B.: For Portenta Machine Control, the pin must be one of the Digital Inputs 0-7.
   * The parameter must be one of the range DIN_READ_CH_PIN_00-DIN_READ_CH_PIN_07.
   * The defines are available in the Portenta Machine Control library and you must include
   * Arduino_PortentaMachineControl.h for having access to them.
   */
  void setPin(int pin);


protected:
  static inline ResetInputCallback _pressedCustomCallback;
  static inline int _pin;
  /**
   * @brief Private constructor to enforce the singleton pattern.
   */
  ResetInputBase();

private:
  ResetInputBase *_instance;
};
