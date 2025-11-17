/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/
#pragma once

#define ANetworkConfigurator_LIB_VERSION "0.5.1"

#if defined(ARDUINO_SAMD_MKRWIFI1010)
  #define NETWORK_CONFIGURATOR_COMPATIBLE 1
  #define PIN_RECONFIGURE 7
  #define LED_RECONFIGURE LED_BUILTIN
  #define BOARD_HAS_RGB
  #define GREEN_LED 25
  #define BLUE_LED 27
  #define RED_LED 26
  #define BOARD_USE_NINA
  #define LED_ON HIGH
  #define LED_OFF LOW
#endif

#if defined(ARDUINO_SAMD_NANO_33_IOT)
  #define NETWORK_CONFIGURATOR_COMPATIBLE 1
  #define PIN_RECONFIGURE 2
  #define LED_RECONFIGURE LED_BUILTIN
  #define GREEN_LED LED_BUILTIN
  #define LED_ON HIGH
  #define LED_OFF LOW
#endif

#if defined(ARDUINO_NANO_RP2040_CONNECT)
  #define NETWORK_CONFIGURATOR_COMPATIBLE 1
  #define PIN_RECONFIGURE 2
  #define LED_RECONFIGURE LED_BUILTIN
  #define BOARD_HAS_RGB
  #define GREEN_LED 25
  #define BLUE_LED 26
  #define RED_LED 27
  #define BOARD_USE_NINA
  #define LED_ON LOW
  #define LED_OFF HIGH
#endif

#if defined(ARDUINO_UNOR4_WIFI)
  #define NETWORK_CONFIGURATOR_COMPATIBLE 1
  #define PIN_RECONFIGURE 2
  #define LED_RECONFIGURE LED_BUILTIN
  #define GREEN_LED LED_BUILTIN
  #define LED_ON HIGH
  #define LED_OFF LOW
  #define BOARD_HAS_LED_MATRIX
#endif

#if defined(ARDUINO_PORTENTA_H7_M7)
  #define NETWORK_CONFIGURATOR_COMPATIBLE 1
  #define ZERO_TOUCH_ENABLED 1
  #define I2C_ADD_DETECT_MACHINE_CONTROL_1 0x23
  #define I2C_ADD_DETECT_MACHINE_CONTROL_2 0x22
  #define PIN_RECONFIGURE 0
  #define LED_RECONFIGURE LED_BUILTIN
  #define BOARD_HAS_RGB
  #define GREEN_LED LEDG
  #define BLUE_LED  LEDB
  #define RED_LED LEDR
  #define LED_ON LOW
  #define LED_OFF HIGH
#endif

#if defined(ARDUINO_OPTA)
  #define NETWORK_CONFIGURATOR_COMPATIBLE 1
  #define ZERO_TOUCH_ENABLED 1
  #define OPTA_WIFI_PID  _BOARD_PRODUCTID
  #define PIN_RECONFIGURE BTN_USER
  #define LED_RECONFIGURE LED_USER
  #define BOARD_HAS_RGB
  #define GREEN_LED LED_BUILTIN
  #define BLUE_LED  LED_USER
  #define RED_LED LEDR
  #define LED_ON HIGH
  #define LED_OFF LOW
#endif


#if defined(ARDUINO_GIGA)
  #define NETWORK_CONFIGURATOR_COMPATIBLE 1
  #define PIN_RECONFIGURE 7
  #define LED_RECONFIGURE LEDG
  #define BOARD_HAS_RGB
  #define GREEN_LED LEDG
  #define BLUE_LED  LEDB
  #define RED_LED LEDR
  #define LED_ON LOW
  #define LED_OFF HIGH
#endif

#if defined(ARDUINO_NICLA_VISION)
  #define NETWORK_CONFIGURATOR_COMPATIBLE 1
  #define PIN_RECONFIGURE PA_13
  #define LED_RECONFIGURE LED_BUILTIN
  #define BOARD_HAS_RGB
  #define GREEN_LED LEDG
  #define BLUE_LED  LEDB
  #define RED_LED LEDR
  #define LED_ON LOW
  #define LED_OFF HIGH
#endif

#if defined(ARDUINO_PORTENTA_C33)
  #define NETWORK_CONFIGURATOR_COMPATIBLE 1
  #define ZERO_TOUCH_ENABLED 1
  #define PIN_RECONFIGURE 0
  #define LED_RECONFIGURE LEDG
  #define BOARD_HAS_RGB
  #define GREEN_LED LEDG
  #define BLUE_LED  LEDB
  #define RED_LED LEDR
  #define LED_ON LOW
  #define LED_OFF HIGH
#endif

#ifdef CI_TEST
  #define NETWORK_CONFIGURATOR_COMPATIBLE 1
#endif

#ifndef NETWORK_CONFIGURATOR_COMPATIBLE
  #define NETWORK_CONFIGURATOR_COMPATIBLE 0
  #define ZERO_TOUCH_ENABLED 0
#endif

#ifndef RESET_HOLD_TIME
#define RESET_HOLD_TIME 3000000
#endif

#ifndef BCP_DEBUG_PACKET
#define BCP_DEBUG_PACKET 0
#endif

// Set to 1 for logging newtwork configurations secrets 
// Be careful the secrets will be printed in the serial monitor in clear text
#define DEBUG_NETWORK_CREDENTIALS 0
