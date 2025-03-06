/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "thingProperties.h"
#include "utility/watchdog/Watchdog.h"
#include "CSRHandler.h"
#include "SecretsHelper.h"
#include <Arduino_SecureElement.h>
#include <utility/SElementArduinoCloudDeviceId.h>
#include <utility/SElementArduinoCloudCertificate.h>

#if defined(ARDUINO_OPTA) || defined(ARDUINO_PORTENTA_H7_M7)
#include "mbed.h"
#include "mbed_mem_trace.h"
#endif
extern "C" char* sbrk(int incr);
void display_freeram();
#if defined(ARDUINO_OPTA)
#define RESETCRED_BUTTON BTN_USER
#elif defined(ARDUINO_SAMD_MKRWIFI1010) || defined(ARDUINO_SAMD_NANO_33_IOT)
#define RESETCRED_BUTTON 7
#else 
#define RESETCRED_BUTTON 13
#endif

enum class DeviceState { FIRST_CONFIG,
                         CSR,
                         BEGIN_CLOUD,
                         RUN };

DeviceState _state = DeviceState::FIRST_CONFIG;
SecureElement secureElement;
uint32_t lastUpdate = 0;
String uhwid = "";

CSRHandlerClass *CSRHandler;

bool clearStoredCredentials() {
  const uint8_t empty[4] = {0x00,0x00,0x00,0x00};
  /*if(!NetworkConfigurator.resetStoredConfiguration() || \
   !secureElement.writeSlot(static_cast<int>(SElementArduinoCloudSlot::DeviceId), (byte*)empty, sizeof(empty)) || \
   !secureElement.writeSlot(static_cast<int>(SElementArduinoCloudSlot::CompressedCertificate), (byte*)empty, sizeof(empty))) {
    return false;
  }*/
 if(!NetworkConfigurator.resetStoredConfiguration()) {
    Serial.println("reset network config failed");
    return false;
  }

  if(!secureElement.writeSlot(static_cast<int>(SElementArduinoCloudSlot::DeviceId), (byte*)empty, sizeof(empty))) {
    Serial.println("reset device id failed");
    return false;
  }

  if(!secureElement.writeSlot(static_cast<int>(SElementArduinoCloudSlot::CompressedCertificate), (byte*)empty, sizeof(empty))) {
    Serial.println("reset certificate failed");
    return false;
  }

  _state = DeviceState::FIRST_CONFIG;
  return true;
}

void setup() {
  // put your setup code here, to run once:
  // Initialize serial and wait for port to open:
  Serial.begin(9600);
  // This delay gives the chance to wait for a Serial Monitor without blocking if none is found
  delay(1500);
  /*
     The following function allows you to obtain more information
     related to the state of network and IoT Cloud connection and errors
     the higher number the more granular information you’ll get.
     The default is 0 (only errors).
     Maximum is 4
 */
  setDebugMessageLevel(4);
  // Defined in thingProperties.h
  initProperties();
  // Init the secure element
  while (!secureElement.begin()) {
    Serial.println("No secureElement present!");
    delay(100);
  }

  if (!secureElement.locked()) {

    if (!secureElement.writeConfiguration()) {
      Serial.println("Writing secureElement configuration failed!");
      Serial.println("Stopping Provisioning");
      while (1)
        ;
    }

    if (!secureElement.lock()) {
      Serial.println("Locking secureElement configuration failed!");
      Serial.println("Stopping Provisioning");
      while (1)
        ;
    }

    Serial.println("secureElement locked successfully");
    Serial.println();
  }

  uhwid = GetUHWID();
  // Need for restoring the scan after got UHWID
#if defined(ARDUINO_UNOR4_WIFI)
  WiFi.end();
#endif

#if defined(ARDUINO_SAMD_MKRWIFI1010) || defined(ARDUINO_SAMD_NANO_33_IOT) || defined(ARDUINO_PORTENTA_H7_M7)
  pinMode(RESETCRED_BUTTON, INPUT_PULLDOWN);
#else
  pinMode(RESETCRED_BUTTON, INPUT);
#endif

#if defined(ARDUINO_OPTA) 
  if(digitalRead(RESETCRED_BUTTON) == LOW){
#else
  if (digitalRead(RESETCRED_BUTTON) == HIGH) {
#endif
    Serial.println("Resetting cred");
    NetworkConfigurator.resetStoredConfiguration();
  }

  NetworkConfigurator.updateNetworkOptions();
  NetworkConfigurator.begin();
  ClaimingHandler.begin(&secureElement, &uhwid, clearStoredCredentials);

  Serial.println("end setup");
  display_freeram();
}

DeviceState handleFirstConfig() {
  DeviceState nextState = _state;
  ClaimingHandler.poll();
  NetworkConfiguratorStates s = NetworkConfigurator.poll();
  if (s == NetworkConfiguratorStates::CONFIGURED) {
    String deviceId = "";
    SElementArduinoCloudDeviceId::read(secureElement, deviceId, SElementArduinoCloudSlot::DeviceId);

    if (deviceId == "") {
      Serial.println("Starting CSR");
      display_freeram();
      CSRHandler = new CSRHandlerClass();
      CSRHandler->begin(&ArduinoIoTPreferredConnection, &secureElement, &uhwid);
      Serial.println("CSR started");
      display_freeram();
      nextState = DeviceState::CSR;
    } else {
      Serial.println("Starting cloud");
      display_freeram();
      nextState = DeviceState::BEGIN_CLOUD;
    }
  }
  return nextState;
}

DeviceState handleCSR() {
  DeviceState nextState = _state;
  ClaimingHandler.poll();
  if (CSRHandler->poll() == CSRHandlerClass::CSRHandlerStates::COMPLETED) {


    CSRHandler->end();
    delete CSRHandler;
    Serial.println("CSR done");
    display_freeram();
    nextState = DeviceState::BEGIN_CLOUD;
  }
  return nextState;
}

DeviceState handleBeginCloud() {
  // Close the connection to the peer (App mobile, FE, etc)
  AgentsManager.disconnect();
  if (AgentsManager.isBLEAgentEnabled()) {
    AgentsManager.enableBLEAgent(false);
  }
  // Connect to Arduino IoT Cloud
  ArduinoCloud.begin(ArduinoIoTPreferredConnection, false, "mqtts-sa.iot.oniudra.cc");
  ArduinoCloud.printDebugInfo();
  Serial.println("cloud inited");
  display_freeram();
  return DeviceState::RUN;
}

DeviceState handleRun() {
  DeviceState nextState = _state;
  ArduinoCloud.update();

  ClaimingHandler.poll();
  if (NetworkConfigurator.poll() == NetworkConfiguratorStates::UPDATING_CONFIG) {
    nextState = DeviceState::FIRST_CONFIG;
  }
  
  // Your code here
  if (millis() - lastUpdate > 10000) {
    Serial.println("alive");
    display_freeram();
    lastUpdate = millis();
    counter++;
  }

  return nextState;
}


void loop() {
  // put your main code here, to run repeatedly:
  switch (_state) {
    case DeviceState::FIRST_CONFIG: _state = handleFirstConfig(); break;
    case DeviceState::CSR:          _state = handleCSR        (); break;
    case DeviceState::BEGIN_CLOUD:  _state = handleBeginCloud (); break;
    case DeviceState::RUN:          _state = handleRun        (); break;
    default:                                                      break;
  }
}

/*
  Since Counter is READ_WRITE variable, onCounterChange() is
  executed every time a new value is received from IoT Cloud.
*/
void onCounterChange() {
  // Add your code here to act upon Counter change
}

int freeRam() {
  char top;
#if defined(ARDUINO_OPTA) || defined(ARDUINO_PORTENTA_H7_M7)
  int cnt = osThreadGetCount();
  mbed_stats_stack_t* stats = (mbed_stats_stack_t*)malloc(cnt * sizeof(mbed_stats_stack_t));

  cnt = mbed_stats_stack_get_each(stats, cnt);
  for (int i = 0; i < cnt; i++) {
    Serial.print("Thread: ");
    Serial.println(stats[i].thread_id, HEX);
    Serial.print("Stack size: ");
    Serial.print(stats[i].max_size);
    Serial.print("/");
    Serial.println(stats[i].reserved_size);
  }
  free(stats);

  // Grab the heap statistics
  mbed_stats_heap_t heap_stats;
  mbed_stats_heap_get(&heap_stats);
  Serial.print("Heap size: ");
  Serial.print(heap_stats.current_size);
  Serial.print("/");
  Serial.println(heap_stats.reserved_size);

  return 0;
#else
  return &top - reinterpret_cast<char*>(sbrk(0));
#endif
}

void display_freeram() {
  Serial.print(F("- SRAM left: "));
  Serial.println(freeRam());
}
