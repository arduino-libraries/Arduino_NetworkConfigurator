/*
  Copyright (c) 2025 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/
#pragma once
#include "Arduino.h"

#define INVALID_LED_PIN -1

class LEDFeedbackClass {
public:
    enum class LEDFeedbackMode {
        NONE,
        BLE_AVAILABLE,
        PEER_CONNECTED,
        PEER_CONNECTED_BLE,
        PEER_CONNECTED_SERIAL,
        CONNECTING_TO_NETWORK,
        CONNECTED_TO_CLOUD,
        ERROR
    };
    static LEDFeedbackClass& getInstance();
    void begin();
    void setMode(LEDFeedbackMode mode);
    void stop();
    void restart();
    void update();
private:
    LEDFeedbackClass() {};
    void turnON();
    void turnOFF();
    void configurePeerConnectedMode();
    LEDFeedbackMode _mode = LEDFeedbackMode::NONE;
    uint32_t _lastUpdate = 0;
    uint32_t _count = 0;
    bool _ledState = false;
    int _ledPin = INVALID_LED_PIN;
    uint32_t* _framePtr = nullptr;
    int32_t _ledChangeInterval = 0;
    bool stopped = false;
};
