/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once
#include "Arduino.h"
#define MAX_WIFI_NETWORKS 20

/*
 * Structures for storing the available network options
 */

enum class NetworkOptionsClass { NONE,
                                 WIFI };

typedef struct {
  char *SSID;
  int RSSI;
  size_t SSIDsize;

} DiscoveredWiFiNetwork;

struct WiFiOption {
  DiscoveredWiFiNetwork discoveredWifiNetworks[MAX_WIFI_NETWORKS];
  int numDiscoveredWiFiNetworks = 0;
};

struct NetworkOptions {
  NetworkOptionsClass type;
  union {
    WiFiOption wifi;
  } option;
};
