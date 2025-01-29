#include "provisioning.h"
#include "ConfiguratorAgents/MessagesDefinitions.h"
const char * UHWID = "identifier1212123210111213141516";
const char * TOKEN = "eyJhbGciOiJFUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJjODcxODE3ZTRkN2VhY2QxOGI5NGYwYWE4YjAwNDZkZDE5Zjg0MGEwMDgzMzZiNTA5YWJmOTkwYjQ3ZTNiMTg3IiwiaWF0IjoxNzI4NDc1MTA4LCJleHAiOjE3Mjg0Nzg3MDh9.-f_UodDftALaC27UXj3lmp8SCWXCpHLBYl70Pl6DbHzb4lc-GqLYt2I_g_pquiNPym2YApUPL_7yqO-NfWuQHQ";
#if defined(ARDUINO_OPTA) || defined(ARDUINO_PORTENTA_H7_M7)
#include "mbed.h"
#include "mbed_mem_trace.h"
#endif
extern "C" char* sbrk(int incr);
int freeRam() {
  char top;
#if defined(ARDUINO_OPTA) || defined(ARDUINO_PORTENTA_H7_M7)
    int cnt = osThreadGetCount();
    mbed_stats_stack_t *stats = (mbed_stats_stack_t*) malloc(cnt * sizeof(mbed_stats_stack_t));

    cnt = mbed_stats_stack_get_each(stats, cnt);
    for (int i = 0; i < cnt; i++) {
        Serial.print("Thread: ");
        Serial.println(stats[i].thread_id, HEX);
        Serial.print("Stack size: ");
        Serial.print( stats[i].max_size);
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

void display_freeram(){
  Serial.print(F("- SRAM left: "));
  Serial.println(freeRam());
}
#define PROVISIONING_SERVICEID_FOR_AGENTMANAGER 0xB1
Provisioning::Provisioning(AgentsConfiguratorManager &agc)
  : _agentManager{ &agc } {
}

void Provisioning::begin() {
  if (!_agentManager->addRequestHandler(RequestType::GET_ID, getIdRequestHandler)) {
    Serial.println("error adding get id callback");
  }
  if (!_agentManager->addReturnTimestampCallback(setTimestamp)) {
    Serial.println("error adding returntimestamp callback");
  }
  _agentManager->begin(PROVISIONING_SERVICEID_FOR_AGENTMANAGER);
  _agentInitialized = true;
  _reqCompleted = false;
}

void Provisioning::end() {
  _agentManager->removeReturnTimestampCallback();
  _agentManager->removeRequestHandler(RequestType::GET_ID);
  _agentManager->end(PROVISIONING_SERVICEID_FOR_AGENTMANAGER);
  _agentInitialized = false;
}

bool Provisioning::poll() {
  _agentManager->poll();
  if (_reqReceived) {
    _reqReceived = false;

    if (_ts != 0) {
      //Send UHWID
      ProvisioningOutputMessage idMsg;
      idMsg.type = MessageOutputType::UHWID;
      idMsg.m.uhwid = UHWID;
      _agentManager->sendMsg(idMsg);
      //Send JWT
      ProvisioningOutputMessage jwtMsg;
      jwtMsg.type = MessageOutputType::JWT;
      jwtMsg.m.jwt = TOKEN;
      _agentManager->sendMsg(jwtMsg);
      _reqCompleted = true;
    } else {
      Serial.println("Error: timestamp not provided");
      ProvisioningOutputMessage msg = { MessageOutputType::STATUS, { MessageTypeCodes::ERROR }  };
      _agentManager->sendMsg(msg);
    }
  }
  return _reqCompleted;
}

void Provisioning::getIdRequestHandler() {
  Serial.println("Request getId received");
  _reqReceived = true;
}
void Provisioning::setTimestamp(uint64_t ts) {
  _ts = ts;
  Serial.print("received TS: ");
  Serial.println(_ts);
}