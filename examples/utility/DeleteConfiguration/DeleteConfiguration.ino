/*
  Copyright (c) 2025 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

/*
 * This sketch wipes out the stored network configuration
 * using the NetworkConfigurator library.
 * This sketch doesn't use the reconfiguration procedure.
*/

#include <Arduino_ConnectionHandler.h>
#include <GenericConnectionHandler.h>
#include <Arduino_KVStore.h>
#include <Arduino_NetworkConfigurator.h>

KVStore kvstore;
GenericConnectionHandler conMan;
NetworkConfiguratorClass NetworkConfigurator(conMan);

bool waitResponse() {
  bool confirmation = false;
  bool proceed = false;
  while (confirmation == false) {
    if (Serial.available()) {
      char choice = Serial.read();
      switch (choice) {
        case 'y':
        case 'Y':
          confirmation = true;
          proceed = true;
          break;
        case 'n':
        case 'N':
          confirmation = true;
          proceed = false;
          break;
        default:
          continue;
      }
    }
  }
  return proceed;
}

void setup() {
  /* Initialize serial debug port and wait up to 5 seconds for port to open */
  Serial.begin(9600);

  /* Set the debug message level:
  * - DBG_ERROR: Only show error messages
  * - DBG_WARNING: Show warning and error messages
  * - DBG_INFO: Show info, warning, and error messages
  * - DBG_DEBUG: Show debug, info, warning, and error messages
  * - DBG_VERBOSE: Show all messages
  */
  setDebugMessageLevel(DBG_INFO);

  while (!Serial);

  /* Set the KVStore */
  NetworkConfigurator.setStorage(kvstore);

  Serial.println("\nWARNING! Running the sketch the stored network configuration will be erased.");
  Serial.println("Do you want to proceed? Y/[n]");

  if (true == waitResponse()) {
    /* Wipe out the network configuration */
    if (NetworkConfigurator.resetStoredConfiguration()) {
      Serial.println("Network configuration reset successfully.");
    } else {
      Serial.println("Failed to reset network configuration.");
    }
  } else {
    Serial.println("Aborted.");
  }
}

void loop() {
}
