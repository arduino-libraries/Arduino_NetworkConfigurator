/*
  Copyright (c) 2025 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "ANetworkConfigurator_Config.h"
#if NETWORK_CONFIGURATOR_COMPATIBLE

#include "LEDFeedback.h"

#ifdef BOARD_USE_NINA
#include "WiFiNINA.h"
#include "utility/wifi_drv.h"
#endif

#ifdef BOARD_HAS_LED_MATRIX
#include "Arduino_LED_Matrix.h"

ArduinoLEDMatrix ledMatrixAnimationHandler;

const uint32_t usb_image[3] = {
  0x3c,
  0x267fe0,
  0x160e0000,

};

const uint32_t bluetooth[3] = {

  0x401600d,
  0x600600,
  0xd0160040,
};

const uint32_t cloud[][4] = {
	{
		0x1c023,
		0x4484044,
		0x43f8000,
		500
	},
	{
		0xe011,
		0x82242022,
		0x21fc000,
		500
	}
};

const uint32_t snake[][4] = {
  { 0xffffffff, 0xffffffff, 0xffffffff, 2000 },
	{ 0x7fffffff, 0xffffffff, 0xfffff7ff, 66 },
	{ 0x3fe7ffff, 0xffffffff, 0xff7ff3fe, 66 },
	{ 0x1fc3fe7f, 0xfffffff7, 0xff3fe1fc, 66 },
	{ 0xf81fc3f, 0xe7ff7ff3, 0xfe1fc0f8, 66 },
	{ 0x500f81f, 0xc3fe3fe1, 0xfc0f8070, 66 },
	{ 0x500f, 0x81fc1fc0, 0xf8070020, 66 },
	{ 0x5, 0xf80f80, 0x70020000, 66 },
	{ 0x5, 0xa80880, 0x50020000, 600 },
	{ 0xd812, 0x41040880, 0x50020000, 200 },
	{ 0x5, 0xa80880, 0x50020000, 600 },
	{ 0xd812, 0x41040880, 0x50020000, 200 },
	{ 0x5, 0xa80880, 0x50020000, 130 },
	{ 0x45, 0xa80880, 0x50020000, 130 },
	{ 0x45, 0xa80880, 0x10060000, 130 },
	{ 0x45, 0xa80080, 0x100e0000, 130 },
	{ 0x45, 0x280080, 0x101e0000, 130 },
	{ 0x41, 0x280080, 0x103e0000, 130 },
	{ 0x41, 0x80080, 0x107e0000, 130 },
	{ 0x40, 0x80084, 0x107e0000, 130 },
	{ 0x40, 0x4084, 0x107e0000, 130 },
	{ 0x40, 0x4004084, 0x107e0000, 130 },
	{ 0x40, 0x4204084, 0x107e0000, 130 },
	{ 0x60, 0x4204004, 0x107e0000, 130 },
	{ 0x70, 0x4204004, 0x7e0000, 130 },
	{ 0x70, 0x5204004, 0x7c0000, 130 },
	{ 0x70, 0x5a04004, 0x780000, 130 },
	{ 0x70, 0x5e04004, 0x700000, 130 },
	{ 0x70, 0x45e04004, 0x700000, 130 },
	{ 0x70, 0x45f04004, 0x600000, 130 },
	{ 0x71, 0x45f04004, 0x400000, 130 },
	{ 0x71, 0xc5f04004, 0x400000, 130 },
	{ 0x71, 0xc5f04004, 0x400002, 130 },
	{ 0x71, 0xe5f04004, 0x2, 130 },
	{ 0x71, 0xe5f24000, 0x2, 130 },
	{ 0x71, 0xe5f20020, 0x2, 130 },
	{ 0x71, 0xe1f20020, 0x2000002, 130 },
	{ 0x71, 0xe1f22020, 0x2002002, 130 },
	{ 0x31, 0xe1f22020, 0x2002006, 130 },
	{ 0x11, 0xe1f22020, 0x200200e, 130 },
	{ 0x1, 0xe1f22020, 0x200201e, 130 },
	{ 0x1, 0xe0f22020, 0x200203e, 130 },
	{ 0x1, 0xe0722020, 0x200207e, 130 },
	{ 0x1, 0xe0322020, 0x20020fe, 130 },
	{ 0x1, 0xe0122020, 0x20820fe, 130 },
	{ 0x1, 0xe0022020, 0x21820fe, 130 },
	{ 0x0, 0xe0022022, 0x221820fe, 130 },
	{ 0x0, 0x60022022, 0x221820fe, 130 },
	{ 0x0, 0x21022022, 0x221820fe, 130 },
	{ 0x0, 0x1822022, 0x221820fe, 130 },
	{ 0x0, 0x1802422, 0x2218a0fe, 130 },
	{ 0x0, 0x1802402, 0x2218a0fe, 130 },
	{ 0x0, 0x1802402, 0x2019a0fe, 130 },
	{ 0x0, 0x1802442, 0x201980fe, 130 },
	{ 0x0, 0x1802442, 0x241980fc, 130 },
	{ 0x4000, 0x1802442, 0x241980f8, 130 },
	{ 0x4000, 0x1882442, 0x241980f0, 130 },
	{ 0x4000, 0x1982442, 0x241980e0, 130 },
	{ 0x4001, 0x1982442, 0x241980c0, 130 },
	{ 0x6001, 0x1982442, 0x24198080, 130 },
	{ 0x6001, 0x1982442, 0x24198000, 130 },
	{ 0x6001, 0x1982442, 0x24198000, 130 },
	{ 0x6001, 0x1982442, 0x24198000, 130 },
	{ 0x6001, 0x1982442, 0x24198000, 600 }
};

#endif


#define FASTBLINK_INTERVAL 200
#define HEARTBEAT_INTERVAL 250
#define SLOWBLINK_INTERVAL 2000
#define ALWAYS_ON_INTERVAL -1

/* The BLE_AVAILABLE animation follows the pattern:
 *  _|¯|_|¯|_|¯¯¯¯¯¯¯¯¯¯¯|_
 * HIGH: led on
 * LOW: led off
 */
#define N_IMPULSES_START_ON_PERIOD 5
#define N_IMPULSES_END_ON_PERIOD 15

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

  pinMode(RED_LED, OUTPUT);
  digitalWrite(RED_LED, LED_OFF);
  #if defined(ARDUINO_OPTA)
    if(_getPid_() == OPTA_WIFI_PID){
      pinMode(BLUE_LED, OUTPUT);
      digitalWrite(BLUE_LED, LED_OFF);
    }
  #endif
#endif
#else
  pinMode(GREEN_LED, OUTPUT);
#endif

#ifdef BOARD_HAS_LED_MATRIX
  ledMatrixAnimationHandler.begin();
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
        turnOFF();
        #ifdef BOARD_HAS_LED_MATRIX
          ledMatrixAnimationHandler.clear();
        #endif
      }
      break;
    case LEDFeedbackMode::BLE_AVAILABLE:{
      #ifdef BOARD_HAS_RGB
        turnOFF();
        #ifdef ARDUINO_OPTA
          if(_getPid_() != OPTA_WIFI_PID){
            _ledPin = GREEN_LED;
          } else {
            _ledPin = BLUE_LED;
          }
        #else
        _ledPin = BLUE_LED;
        #endif
      #else
        _ledPin = GREEN_LED;
      #endif
      #ifdef BOARD_HAS_LED_MATRIX
        _framePtr = nullptr;
        ledMatrixAnimationHandler.loadSequence(snake);
        ledMatrixAnimationHandler.play(true);
        /* For fixing the issue that the first
         * frame of the first run is not shown */
        ledMatrixAnimationHandler.loadSequence(snake);
      #endif
      _ledChangeInterval = HEARTBEAT_INTERVAL;
      _count = 0;
    }
      break;
    case LEDFeedbackMode::PEER_CONNECTED:
    {
      configurePeerConnectedMode();
    }
      break;
    case LEDFeedbackMode::PEER_CONNECTED_BLE:
    {
      configurePeerConnectedMode();
    #ifdef BOARD_HAS_LED_MATRIX
      ledMatrixAnimationHandler.loadFrame(bluetooth);
      _framePtr = (uint32_t*)bluetooth;
    #endif
    }
      break;
    case LEDFeedbackMode::PEER_CONNECTED_SERIAL:
    {
      configurePeerConnectedMode();
    #ifdef BOARD_HAS_LED_MATRIX
      ledMatrixAnimationHandler.loadFrame(usb_image);
      _framePtr = (uint32_t*)usb_image;
    #endif
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
        ledMatrixAnimationHandler.loadSequence(LEDMATRIX_ANIMATION_WIFI_SEARCH);
        ledMatrixAnimationHandler.play(true);
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
      ledMatrixAnimationHandler.loadSequence(cloud);
      ledMatrixAnimationHandler.play(true);
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
        _ledChangeInterval = SLOWBLINK_INTERVAL;
      #else
        _ledPin = GREEN_LED;
        _ledChangeInterval = FASTBLINK_INTERVAL;
      #endif
      #ifdef BOARD_HAS_LED_MATRIX
        _framePtr = (uint32_t*)LEDMATRIX_EMOJI_SAD;
        ledMatrixAnimationHandler.loadFrame(LEDMATRIX_EMOJI_SAD);
      #endif
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

void LEDFeedbackClass::update() {
  if(stopped || _mode == LEDFeedbackMode::NONE) {
    return;
  }

  if (_ledChangeInterval == ALWAYS_ON_INTERVAL) {
    turnON();
    return;
  }

  if(millis() - _lastUpdate > _ledChangeInterval/2) {
    _lastUpdate = millis();
    //Implement the pattern for BLE_AVAILABLE
    if(_mode == LEDFeedbackMode::BLE_AVAILABLE) {
      if (_count == 0){
        turnOFF();
        _count++;
        return;
      }else if(_count >= N_IMPULSES_START_ON_PERIOD && _count < N_IMPULSES_END_ON_PERIOD){
        turnON();
        _count++;
        return;
      }else if(_count >= N_IMPULSES_END_ON_PERIOD){
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
  if(_ledPin != INVALID_LED_PIN) {
  #ifdef BOARD_USE_NINA
    WiFiDrv::digitalWrite(_ledPin, LED_OFF);
  #else
    digitalWrite(_ledPin, LED_OFF);
  #endif
  }
#ifdef BOARD_HAS_LED_MATRIX
  if(_framePtr != nullptr){
    ledMatrixAnimationHandler.clear();
  }

#endif
  _ledState = false;
}

void LEDFeedbackClass::turnON() {
  if(_ledPin != INVALID_LED_PIN) {
  #ifdef BOARD_USE_NINA
    WiFiDrv::digitalWrite(_ledPin, LED_ON);
  #else
    digitalWrite(_ledPin, LED_ON);
  #endif
  }
#ifdef BOARD_HAS_LED_MATRIX
  if(_framePtr != nullptr){
    ledMatrixAnimationHandler.loadFrame(_framePtr);
  }
#endif
  _ledState = true;
}

void LEDFeedbackClass::configurePeerConnectedMode() {
  #ifdef BOARD_HAS_RGB
    turnOFF();
    _ledPin = BLUE_LED;
    #if defined(ARDUINO_OPTA)
    if(_getPid_() != OPTA_WIFI_PID){
      _ledPin = GREEN_LED;
    }
    #endif
  #else
    _ledPin = GREEN_LED;
  #endif
    _ledChangeInterval = ALWAYS_ON_INTERVAL;
}

#endif // NETWORK_CONFIGURATOR_COMPATIBLE
