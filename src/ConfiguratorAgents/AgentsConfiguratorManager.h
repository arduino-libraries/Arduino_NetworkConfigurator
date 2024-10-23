#pragma once
#include <list>
#include "Arduino.h"
#include "agents/ConfiguratorAgent.h"
#include "MessagesDefinitions.h"

enum class AgentsConfiguratorManagerStates {INIT, SEND_INITIAL_STATUS, SEND_NETWORK_OPTIONS, CONFIG_IN_PROGRESS, END};

typedef void (*ConfiguratorRequestHandler)();
typedef void (*ReturnTimestamp)(uint64_t ts);
typedef void (*ReturnNetworkSettings)(models::NetworkSetting *netSetting);

enum class RequestType {NONE, CONNECT, SCAN, GET_ID};

class AgentsConfiguratorManager {
  public:
    AgentsConfiguratorManager(): _netOptions{.type=NetworkOptionsClass::NONE} {};
    bool begin(uint8_t id);
    bool end(uint8_t id);
    AgentsConfiguratorManagerStates poll();
    bool setStatusMessage(StatusMessage msg);
    bool setNetworkOptions(NetworkOptions netOptions);
    bool setUID(String uid, String signature);
    bool addAgent(ConfiguratorAgent &agent);
    bool addRequestHandler(RequestType type, ConfiguratorRequestHandler callback);
    void removeRequestHandler(RequestType type) { _reqHandlers[(int)type-1] = nullptr; };
    bool addReturnTimestampCallback(ReturnTimestamp callback);
    void removeReturnTimestampCallback() { _returnTimestampCb = nullptr; };
    bool addReturnNetworkSettingsCallback(ReturnNetworkSettings callback);
    void removeReturnNetworkSettingsCallback(){ _returnNetworkSettingsCb = nullptr; };
    inline bool isConfigInProgress(){return _state == AgentsConfiguratorManagerStates::CONFIG_IN_PROGRESS;};
  private:
    AgentsConfiguratorManagerStates _state = AgentsConfiguratorManagerStates::END;
    std::list<ConfiguratorAgent *> _agentsList;
    std::list<uint8_t> _servicesList;
    ConfiguratorRequestHandler _reqHandlers[3];
    ReturnTimestamp _returnTimestampCb = nullptr;
    ReturnNetworkSettings _returnNetworkSettingsCb = nullptr;
    ConfiguratorAgent * _selectedAgent = nullptr;
    uint8_t _instances = 0;
    
    StatusMessage _initStatusMsg = MessageTypeCodes::NONE;
    NetworkOptions _netOptions;
    typedef struct {
      void reset(){
        pending = false;
        key = RequestType::NONE;
      };
      bool pending;
      RequestType key;
    }StatusRequest;

    StatusRequest _statusRequest = {.pending=false, .key=RequestType::NONE};

    AgentsConfiguratorManagerStates handleInit();
    AgentsConfiguratorManagerStates handleSendInitialStatus();
    AgentsConfiguratorManagerStates handleSendNetworkOptions();
    AgentsConfiguratorManagerStates handleConfInProgress();
    void handleReceivedCommands(RemoteCommands cmd);
    void handleReceivedData();
    void handleConnectCommand();
    void handleUpdateOptCommand();
    void handleGetUIDCommand();
    size_t  computeOptionsToSendLength();
    bool sendNetworkOptions();
    bool sendStatus(StatusMessage msg);

    void handlePeerDisconnected();
    void callHandler(RequestType key);
#if defined(ARDUINO_SAMD_MKRWIFI1010) || defined(ARDUINO_SAMD_NANO_33_IOT) || \
defined(ARDUINO_AVR_UNO_WIFI_REV2) || defined(ARDUINO_NANO_RP2040_CONNECT)
    void stopBLEAgent();
    void startBLEAgent();
#endif
};

extern AgentsConfiguratorManager ConfiguratorManager;
