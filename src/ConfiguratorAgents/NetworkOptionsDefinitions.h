#pragma once
#include "Arduino.h"
#define MAX_WIFI_NETWORKS 20

enum class NetworkOptionsClass {NONE, WIFI, LAN};

typedef struct {
  char  *SSID;
  int RSSI;
  size_t SSIDsize;

}DiscoveredWiFiNetwork;

struct WiFiOption{
  DiscoveredWiFiNetwork discoveredWifiNetworks[MAX_WIFI_NETWORKS];
  int numDiscoveredWiFiNetworks = 0;
};


struct NetworkOptions {
  NetworkOptionsClass type;
  union {
    WiFiOption wifi;
  }option;
} ;
