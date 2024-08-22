#pragma once
#include <settings/settings.h>
#define MAX_WIFI_NETWORKS 20

enum class ConfiguratorStates {INIT, WAITING_FOR_CONFIG, CONFIG_RECEIVED, REQUEST_UPDATE_OPT, END, ERROR};
enum class AgentTypes {BLE, AP, WIRED_USB};
enum class NetworkOptionsClass {WIFI, LAN};

typedef struct {
  const char  *SSID;
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
