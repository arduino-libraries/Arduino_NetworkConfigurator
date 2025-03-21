/*
  Copyright (c) 2025 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "LEDFeedback.h"

#if defined(ARDUINO_SAMD_MKRWIFI1010)
#define BOARD_HAS_RGB
#define GREEN_LED 25
#define BLUE_LED 27
#define RED_LED 26
#define BOARD_USE_NINA
#define LED_ON HIGH
#define LED_OFF LOW
#include "WiFiNINA.h"
#include "utility/wifi_drv.h"
#elif defined(ARDUINO_NANO_RP2040_CONNECT)
#define BOARD_HAS_RGB
#define GREEN_LED 25
#define BLUE_LED 26
#define RED_LED 27
#define BOARD_USE_NINA
#define LED_ON LOW
#define LED_OFF HIGH
#include "WiFiNINA.h"
#include "utility/wifi_drv.h"
#elif defined(ARDUINO_SAMD_NANO_33_IOT)
#define GREEN_LED LED_BUILTIN
#define LED_ON HIGH
#define LED_OFF LOW
#elif  defined(ARDUINO_PORTENTA_H7_M7) || defined(ARDUINO_PORTENTA_C33) || defined(ARDUINO_NICLA_VISION)
#define BOARD_HAS_RGB
#define GREEN_LED LEDG
#define BLUE_LED  LEDB
#define RED_LED LEDR
#define LED_ON LOW
#define LED_OFF HIGH
#elif defined(ARDUINO_GIGA)
#define BOARD_HAS_RGB
#define GREEN_LED LEDG
#define BLUE_LED  LEDB
#define RED_LED LEDR
#define LED_ON LOW
#define LED_OFF HIGH
#elif defined(ARDUINO_OPTA)
#define BOARD_HAS_RGB
#define GREEN_LED LED_BUILTIN
#define BLUE_LED  LED_USER
#define RED_LED LEDR
#define LED_ON HIGH
#define LED_OFF LOW
#elif defined(ARDUINO_UNOR4_WIFI)
#define GREEN_LED LED_BUILTIN
#define LED_ON HIGH
#define LED_OFF LOW
#define BOARD_HAS_LED_MATRIX
#include "Arduino_LED_Matrix.h"
#endif

#ifdef BOARD_HAS_LED_MATRIX
ArduinoLEDMatrix matrix;
const uint32_t bluetooth[3] = {

  0x401600d,
  0x600600,
  0xd0160040,
};

const uint32_t cloud[][4] = {
	{
		0xc013,
		0x82644424,
		0x24023fc,
		66
	},
	{
		0xc013,
		0x82644424,
		0x24023fc,
		66
	},
	{
		0xc013,
		0x82644824,
		0x24023fc,
		66
	}
};



#endif


#define FASTBLINK_INTERVAL 200
#define HEARTBEAT_INTERVAL 250
#define SLOWBLINK_INTERVAL 2000
#define ALWAYS_ON_INTERVAL -1

LEDFeedbackClass &LEDFeedbackClass::getInstance() {
  static LEDFeedbackClass instance;
  return instance;
}


void LEDFeedbackClass::begin() {
#if defined(BOARD_HAS_RGB)
#if defined(BOARD_USE_NINA)
  WiFiDrv::pinMode(GREEN_LED, OUTPUT);
  WiFiDrv::digitalWrite(GREEN_LED, LED_OFF);
  WiFiDrv::pinMode(BLUE_LED, OUTPUT);
  WiFiDrv::digitalWrite(BLUE_LED, LED_OFF);
  WiFiDrv::pinMode(RED_LED, OUTPUT);
  WiFiDrv::digitalWrite(RED_LED, LED_OFF);
#else
  pinMode(GREEN_LED, OUTPUT);
  digitalWrite(GREEN_LED, LED_OFF);
  pinMode(BLUE_LED, OUTPUT);
  digitalWrite(BLUE_LED, LED_OFF);
  pinMode(RED_LED, OUTPUT);
  digitalWrite(RED_LED, LED_OFF);
#endif
#else
  pinMode(GREEN_LED, OUTPUT);
#endif

#ifdef BOARD_HAS_LED_MATRIX
  matrix.begin();
#endif

}

void LEDFeedbackClass::setMode(LEDFeedbackMode mode) {
  if(_mode == mode) {
    return;
  }

  if(_mode == LEDFeedbackMode::ERROR){
    return;
  }

  _mode = mode;
  _lastUpdate = 0;
  switch (_mode) {
    case LEDFeedbackMode::NONE:
      {
        _ledChangeInterval = 0;
        #ifdef BOARD_HAS_LED_MATRIX
          matrix.clear();
        #endif
      }
      break;
    case LEDFeedbackMode::BLE_AVAILABLE:{
      #ifdef BOARD_HAS_RGB
        turnOFF();
        _ledPin = BLUE_LED;
      #else
        _ledPin = GREEN_LED;
      #endif
      #ifdef BOARD_HAS_LED_MATRIX
        matrix.loadFrame(bluetooth);
        _framePtr = (uint32_t*)bluetooth;
      #endif
      _ledChangeInterval = HEARTBEAT_INTERVAL;
      _count = 0;
    }
      break;
    case LEDFeedbackMode::PEER_CONNECTED:
    {
    #ifdef BOARD_HAS_RGB
      turnOFF();
      _ledPin = BLUE_LED;
    #else
      _ledPin = GREEN_LED;
    #endif
    #ifdef BOARD_HAS_LED_MATRIX
      matrix.loadFrame(bluetooth);
      _framePtr = (uint32_t*)bluetooth;
    #endif
      _ledChangeInterval = ALWAYS_ON_INTERVAL;
    }
      break;
    case LEDFeedbackMode::CONNECTING_TO_NETWORK:
    {
      #ifdef BOARD_HAS_RGB
        turnOFF();
      #endif
      _ledPin = GREEN_LED;
      #ifdef BOARD_HAS_LED_MATRIX
        _framePtr = nullptr;
        matrix.loadSequence(LEDMATRIX_ANIMATION_WIFI_SEARCH);
        matrix.play(true);
      #endif
      _ledChangeInterval = ALWAYS_ON_INTERVAL;

    }
      break;
    case LEDFeedbackMode::CONNECTED_TO_CLOUD:
    {
      #ifdef BOARD_HAS_RGB
      turnOFF();
      #endif
      #ifdef BOARD_HAS_LED_MATRIX
      _framePtr = nullptr;
      matrix.loadSequence(cloud);
      matrix.play(true);
      #endif
      _ledPin = GREEN_LED;
      _ledChangeInterval = SLOWBLINK_INTERVAL;
    }
      break;
    case LEDFeedbackMode::ERROR:
    {
      #ifdef BOARD_HAS_RGB
        turnOFF();
        _ledPin = RED_LED;
      #else
        _ledPin = GREEN_LED;
      #endif
      #ifdef BOARD_HAS_LED_MATRIX
        _framePtr = (uint32_t*)LEDMATRIX_EMOJI_SAD;
        matrix.loadFrame(LEDMATRIX_EMOJI_SAD);
      #endif
      _ledChangeInterval = FASTBLINK_INTERVAL;
    }
      break;
    default:
      _ledChangeInterval = 0;
      break;
  }
}

void LEDFeedbackClass::stop() {
  stopped = true;
  turnOFF();
}

void LEDFeedbackClass::restart() {
  stopped = false;
}

void LEDFeedbackClass::poll() {
  if(stopped) {
    return;
  }

  if(_ledChangeInterval == 0) {
    turnOFF();
    return;
  } else if (_ledChangeInterval == ALWAYS_ON_INTERVAL) {
    turnON();
    return;
  }

  if(millis() - _lastUpdate > _ledChangeInterval/2) {
    _lastUpdate = millis();
    if(_mode == LEDFeedbackMode::BLE_AVAILABLE) {
      if (_count == 0){
        turnOFF();
        _count++;
        return;
      }else if(_count >= 5 && _count < 15){
        turnON();
        _count++;
        return;
      }else if(_count >= 15){
        _count = 0;
        return;
      }else{
        _count++;
      }
    }

    if(_ledState) {
      turnOFF();
    } else {
      turnON();
    }
  }

}

void LEDFeedbackClass::turnOFF() {
#ifdef BOARD_USE_NINA
  WiFiDrv::digitalWrite(_ledPin, LED_OFF);
#else
  digitalWrite(_ledPin, LED_OFF);
#endif
#ifdef BOARD_HAS_LED_MATRIX
  if(_framePtr != nullptr){
    matrix.clear();
  }

#endif
  _ledState = false;
}

void LEDFeedbackClass::turnON() {
#ifdef BOARD_USE_NINA
  WiFiDrv::digitalWrite(_ledPin, LED_ON);
#else
  digitalWrite(_ledPin, LED_ON);
#endif
#ifdef BOARD_HAS_LED_MATRIX
  if(_framePtr != nullptr){
    matrix.loadFrame(_framePtr);
  }
#endif
  _ledState = true;
}
