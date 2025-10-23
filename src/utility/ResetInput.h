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
 * @class ResetInput
 * @brief A singleton class to handle input of the reset functionality with interrupt-based monitoring.
 *
 * This class provides methods to configure and monitor a reset input pin. It allows
 * setting up a custom callback function to be executed when the pin status changes.
 */
class ResetInput{
public:
  /**
   * @typedef ResetInput::ResetInputCallback
   * @brief A type definition for the callback function to be executed on pin status change.
   */
  typedef std::function<void()> ResetInputCallback;
  /**
   * @brief Get the singleton instance of the ResetInput class.
   * @return A reference to the ResetInput instance.
   */
  static ResetInput& getInstance();
  /**
   * @brief Initialize the reset input by setting up the interrupt pin.
   */
  void begin();
  /**
   * @brief Check if the reset event has been fired.
   * @return True if the event is fired, otherwise false.
   */
  bool isEventFired();
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
   */
  void setPin(int pin);
private:
  /**
   * @brief Private constructor to enforce the singleton pattern.
   */
  ResetInput();
  static inline ResetInputCallback _pressedCustomCallback;
  static inline int _pin;
  static inline int _ledFeedbackPin;
  static inline int _ledOff;
  static inline int _ledOn;
  static inline volatile bool _expired;
  static inline volatile bool _fireEvent;
  static inline volatile uint32_t _startPressed;
  #if defined(ARDUINO_NANO_RP2040_CONNECT)
  static inline volatile uint32_t _lastEvent;
  #endif
  /**
   * @brief Internal callback function to handle pin press events.
   */
  static void _pressedCallback();
};
