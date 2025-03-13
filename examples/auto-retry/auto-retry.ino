#include "arduino_secrets.h"
/* 
  Sketch generated by the Arduino IoT Cloud Thing "Untitled"
  https://create.arduino.cc/cloud/things/4074e850-7086-4682-ba53-0f3bbcb52f12 

  Arduino IoT Cloud Variables description

  The following variables are automatically generated and updated when changes are made to the Thing

  int counter;

  Variables which are marked as READ/WRITE in the Cloud Thing will also have functions
  which are called when their values are changed from the Dashboard.
  These functions are generated with the Thing and added at the end of this sketch.
*/
#include "thingProperties.h"
#include "utility/watchdog/Watchdog.h"
#if defined(ARDUINO_OPTA)
#define RESETCRED_BUTTON BTN_USER
#elif defined(ARDUINO_SAMD_MKRWIFI1010) || defined(ARDUINO_SAMD_NANO_33_IOT)
#define RESETCRED_BUTTON 7
#else
#define RESETCRED_BUTTON 13
#endif
uint32_t lastUpdate = 0;
bool networkConfigured = false;
bool provisioningCompleted = false;
enum class DeviceMode { CONFIG,
                        RUN };
DeviceMode deviceMode = DeviceMode::CONFIG;
bool toUpdate = false;

void changeMode(DeviceMode nextMode) {
  if (nextMode == DeviceMode::RUN) {
    deviceMode = DeviceMode::RUN;
  } else if (nextMode == DeviceMode::CONFIG) {
    deviceMode = DeviceMode::CONFIG;
  }
}

void setup() {
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

  // Connect to Arduino IoT Cloud
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
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
  ProvisioningSystem.begin();

  ArduinoCloud.printDebugInfo();
}

void loop() {
  if (provisioningCompleted && networkConfigured) {
    AgentsManager.disconnect();
    if (AgentsManager.isBLEAgentEnabled()) {
      AgentsManager.enableBLEAgent(false);
    }
  }
  if (provisioningCompleted == false && ProvisioningSystem.poll()) {
    ProvisioningSystem.end();
    provisioningCompleted = true;
  }

  if (deviceMode == DeviceMode::CONFIG) {
#if defined(ARDUINO_ARCH_SAMD) || defined(ARDUINO_ARCH_MBED)
    watchdog_reset();
#endif
    NetworkConfiguratorStates s = NetworkConfigurator.poll();
    if (s == NetworkConfiguratorStates::CONFIGURED && !toUpdate) {
      networkConfigured = true;
      changeMode(DeviceMode::RUN);
    }else if (s == NetworkConfiguratorStates::UPDATING_CONFIG){
      toUpdate = false;
    }

  } else if (deviceMode == DeviceMode::RUN) {
    ArduinoCloud.update();
#if defined(ARDUINO_OPTA)
    if (digitalRead(RESETCRED_BUTTON) == LOW) {
#else
    if (digitalRead(RESETCRED_BUTTON) == HIGH) {
#endif
      Serial.println("Update config");
      if (!AgentsManager.isBLEAgentEnabled()) {
        AgentsManager.enableBLEAgent(true);
      }

      networkConfigured = false;
    }

    if (NetworkConfigurator.poll() == NetworkConfiguratorStates::UPDATING_CONFIG) {
      networkConfigured = false;
      changeMode(DeviceMode::CONFIG);
      return;
    }

    // Your code here
    if (millis() - lastUpdate > 10000) {
      Serial.println("alive");
      lastUpdate = millis();
      counter++;
    }
  }
}

/*
  Since Counter is READ_WRITE variable, onCounterChange() is
  executed every time a new value is received from IoT Cloud.
*/
void onCounterChange() {
  // Add your code here to act upon Counter change
}
