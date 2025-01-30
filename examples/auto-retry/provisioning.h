#pragma once
#include "Arduino.h"
#include "ConfiguratorAgents/AgentsManager.h"

class Provisioning {
public:
  Provisioning(AgentsManagerClass &agc);
  void begin();
  void end();
  bool poll();
private:
  bool _agentInitialized = false;
  AgentsManagerClass *_agentManager = nullptr;
  bool _reqCompleted = false;
  static inline uint64_t _ts = 0;
  static inline bool _reqReceived = false;
  static void getIdRequestHandler();
  static void setTimestamp(uint64_t ts);
};