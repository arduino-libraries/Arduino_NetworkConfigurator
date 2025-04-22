/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "ANetworkConfigurator_Config.h"
#if NETWORK_CONFIGURATOR_COMPATIBLE

#include "BoardConfigurationProtocol.h"
#include "Arduino_DebugUtils.h"
#include "CBORAdapter.h"
#include "cbor/CBOR.h"

#define PACKET_VALIDITY_MS 30000

/******************************************************************************
 * PUBLIC MEMBER FUNCTIONS
 ******************************************************************************/
bool BoardConfigurationProtocol::getMsg(ProvisioningInputMessage &msg) {
  if (_inputMessagesList.size() == 0) {
    return false;
  }
  InputPacketBuffer *buf = &_inputMessagesList.front();
  ProvisioningMessageDown cborMsg;

  bool decodeRes = CBORAdapter::getMsgFromCBOR(buf->get_ptr(), buf->len(), &cborMsg);
  _inputMessagesList.pop_front();

  if (!decodeRes) {
    DEBUG_DEBUG("BoardConfigurationProtocol::%s Invalid message", __FUNCTION__);
    sendStatus(StatusMessage::INVALID_PARAMS);
    return false;
  }

  if (cborMsg.c.id == ProvisioningMessageId::TimestampProvisioningMessageId) {
    msg.type = MessageInputType::TIMESTAMP;
    msg.m.timestamp = cborMsg.provisioningTimestamp.timestamp;
  } else if (cborMsg.c.id == ProvisioningMessageId::CommandsProvisioningMessageId) {
    msg.type = MessageInputType::COMMANDS;
    msg.m.cmd = (RemoteCommands)cborMsg.provisioningCommands.cmd;
  } else {
    msg.type = MessageInputType::NETWORK_SETTINGS;
    msg.m.netSetting.type = NetworkAdapter::NONE;
    memcpy(&msg.m.netSetting, &cborMsg.provisioningNetworkConfig.networkSetting, sizeof(models::NetworkSetting));
  }
  return true;
}

bool BoardConfigurationProtocol::sendMsg(ProvisioningOutputMessage &msg) {
  bool res = false;
  switch (msg.type) {
    case MessageOutputType::STATUS:
      res = sendStatus(msg.m.status);
      break;
    case MessageOutputType::NETWORK_OPTIONS:
      res = sendNetworkOptions(msg.m.netOptions);
      break;
    case MessageOutputType::UHWID:
      res = sendUhwid(msg.m.uhwid);
      break;
    case MessageOutputType::JWT:
      res = sendJwt(msg.m.jwt, strlen(msg.m.jwt));
      break;
    case MessageOutputType::BLE_MAC_ADDRESS:
      res = sendBleMacAddress(msg.m.BLEMacAddress, BLE_MAC_ADDRESS_SIZE);
      break;
    case MessageOutputType::WIFI_FW_VERSION:
      res = sendVersion(msg.m.wifiFwVersion, msg.type);
      break;
    case MessageOutputType::PROV_SKETCH_VERSION:
      res = sendVersion(msg.m.provSketchVersion, msg.type);
      break;
    case MessageOutputType::NETCONFIG_LIB_VERSION:
      res = sendVersion(msg.m.netConfigLibVersion, msg.type);
      break;
    case MessageOutputType::PROV_PUBLIC_KEY:
      res = sendProvPublicKey(msg.m.provPublicKey, strlen(msg.m.provPublicKey));
      break;
    default:
      break;
  }

  return res;
}

bool BoardConfigurationProtocol::msgAvailable() {
  return _inputMessagesList.size() > 0;
}

/******************************************************************************
 * PROTECTED MEMBER FUNCTIONS
 ******************************************************************************/
BoardConfigurationProtocol::TransmissionResult BoardConfigurationProtocol::sendAndReceive() {
  TransmissionResult transmissionRes = TransmissionResult::NOT_COMPLETED;
  if (!isPeerConnected()) {
    return TransmissionResult::PEER_NOT_AVAILABLE;
  }

  if (_outputMessagesList.size() > 0) {
    checkOutputPacketValidity();
    transmitStream();
  }

  if (!received()) {
    return transmissionRes;
  }

  int receivedDataLen = available();

  for (int i = 0; i < receivedDataLen; i++) {
    PacketManager::ReceivingState res;
    uint8_t val = read();

    res = PacketManager::PacketReceiver::getInstance().handleReceivedByte(_packet, val);
    if (res == PacketManager::ReceivingState::ERROR) {
      DEBUG_DEBUG("BoardConfigurationProtocol::%s Malformed packet", __FUNCTION__);
      sendNak();
      clearInputBuffer();
      transmissionRes = TransmissionResult::INVALID_DATA;
      break;
    } else if (res == PacketManager::ReceivingState::RECEIVED) {
      switch (_packet.Type) {
        case PacketManager::MessageType::DATA:
          {
            #if BCP_DEBUG_PACKET == 1
            printPacket("payload", _packet.Payload.get_ptr(), _packet.Payload.len());
            #endif
            _inputMessagesList.push_back(_packet.Payload);
            //Consider all sent data as received
            _outputMessagesList.clear();
            transmissionRes = TransmissionResult::DATA_RECEIVED;
          }
          break;
        case PacketManager::MessageType::TRANSMISSION_CONTROL:
          {
            if (_packet.Payload.len() == 1 && _packet.Payload[0] == (uint8_t)PacketManager::TransmissionControlMessage::NACK) {
              for (std::list<OutputPacketBuffer>::iterator packet = _outputMessagesList.begin(); packet != _outputMessagesList.end(); ++packet) {
                packet->startProgress();
              }
            } else if (_packet.Payload.len() == 1 && _packet.Payload[0] == (uint8_t)PacketManager::TransmissionControlMessage::DISCONNECT) {
              handleDisconnectRequest();
            }
          }
          break;
        default:
          break;
      }
      PacketManager::PacketReceiver::getInstance().clear(_packet);
    }
  }

  return transmissionRes;
}

bool BoardConfigurationProtocol::sendNak() {
  uint8_t data = 0x03;
  return sendData(PacketManager::MessageType::TRANSMISSION_CONTROL, &data, sizeof(data));
}

bool BoardConfigurationProtocol::sendData(PacketManager::MessageType type, const uint8_t *data, size_t len) {
  OutputPacketBuffer outputMsg;
  outputMsg.setValidityTs(millis() + PACKET_VALIDITY_MS);

  if (!PacketManager::createPacket(outputMsg, type, data, len)) {
    return false;
  }

  #if BCP_DEBUG_PACKET == 1
  printPacket("output message", outputMsg.get_ptr(), outputMsg.len());
  #endif

  _outputMessagesList.push_back(outputMsg);

  TransmissionResult res = TransmissionResult::NOT_COMPLETED;
  do {
    res = transmitStream();
    if (res == TransmissionResult::PEER_NOT_AVAILABLE) {
      break;
    }
  } while (res == TransmissionResult::NOT_COMPLETED);

  return true;
}

void BoardConfigurationProtocol::clear() {
  PacketManager::PacketReceiver::getInstance().clear(_packet);
  _outputMessagesList.clear();
  _inputMessagesList.clear();
}

void BoardConfigurationProtocol::checkOutputPacketValidity() {
  if (_outputMessagesList.size() == 0) {
    return;
  }
  _outputMessagesList.remove_if([](OutputPacketBuffer &packet) {
    if (packet.getValidityTs() != 0 && packet.getValidityTs() < millis()) {
      return true;
    }
    return false;
  });
}

/******************************************************************************
 * PRIVATE MEMBER FUNCTIONS
 ******************************************************************************/
bool BoardConfigurationProtocol::sendStatus(StatusMessage msg) {
  bool res = false;
  size_t len = CBOR_DATA_STATUS_LEN;
  uint8_t data[len];
  res = CBORAdapter::statusToCBOR(msg, data, &len);
  if (!res) {
    return res;
  }

  res = sendData(PacketManager::MessageType::DATA, data, len);
  if (!res) {
    DEBUG_WARNING("BoardConfigurationProtocol::%s failed to send status: %d ", __FUNCTION__, (int)msg);
  }
  return res;
}

size_t BoardConfigurationProtocol::calculateTotalOptionsLength(const NetworkOptions *netOptions) {
  size_t length = CBOR_DATA_HEADER_LEN;

  if (netOptions->type == NetworkOptionsClass::WIFI) {
    for (uint8_t i = 0; i < netOptions->option.wifi.numDiscoveredWiFiNetworks; i++) {
      length += 4;  //for RSSI and text identifier
      length += netOptions->option.wifi.discoveredWifiNetworks[i].SSIDsize;
    }
  }

  return length;
}

bool BoardConfigurationProtocol::sendNetworkOptions(const NetworkOptions *netOptions) {
  bool res = false;

  size_t len = calculateTotalOptionsLength(netOptions);
  uint8_t data[len];

  if (!CBORAdapter::networkOptionsToCBOR(netOptions, data, &len)) {
    return res;
  }

  res = sendData(PacketManager::MessageType::DATA, data, len);
  if (!res) {
    DEBUG_WARNING("BoardConfigurationProtocol::%s failed to send network options", __FUNCTION__);
  }

  return res;
}

bool BoardConfigurationProtocol::sendUhwid(const byte *uhwid) {
  bool res = false;

  size_t cborDataLen = CBOR_DATA_UHWID_LEN;
  uint8_t data[cborDataLen];

  res = CBORAdapter::uhwidToCBOR(uhwid, data, &cborDataLen);

  if (!res) {
    return res;
  }

  res = sendData(PacketManager::MessageType::DATA, data, cborDataLen);
  if (!res) {
    DEBUG_WARNING("BoardConfigurationProtocol::%s failed to send uhwid", __FUNCTION__);
    return res;
  }

  return res;
}

bool BoardConfigurationProtocol::sendJwt(const char *jwt, size_t len) {
  bool res = false;
  if (len > MAX_JWT_SIZE) {
    return res;
  }

  size_t cborDataLen = CBOR_DATA_JWT_LEN;
  uint8_t data[cborDataLen];

  res = CBORAdapter::jwtToCBOR(jwt, data, &cborDataLen);
  if (!res) {
    return res;
  }

  res = sendData(PacketManager::MessageType::DATA, data, cborDataLen);

  if (!res) {
    DEBUG_WARNING("BoardConfigurationProtocol::%s failed to send JWT", __FUNCTION__);
    return res;
  }

  return res;
}

bool BoardConfigurationProtocol::sendProvPublicKey(const char *provPublicKey, size_t len) {

  size_t cborDataLen = CBOR_MIN_PROV_PUBIC_KEY_LEN + len;
  uint8_t data[cborDataLen];

  if (!CBORAdapter::provPublicKeyToCBOR(provPublicKey, data, &cborDataLen)) {
    return false;
  }

  if (!sendData(PacketManager::MessageType::DATA, data, cborDataLen)) {
    DEBUG_WARNING("BoardConfigurationProtocol::%s failed to send JWT", __FUNCTION__);
    return false;
  }

  return true;
}

bool BoardConfigurationProtocol::sendBleMacAddress(const uint8_t *mac, size_t len) {
  bool res = false;
  if (len != BLE_MAC_ADDRESS_SIZE) {
    return res;
  }

  size_t cborDataLen = CBOR_DATA_BLE_MAC_LEN;
  uint8_t data[cborDataLen];

  res = CBORAdapter::BLEMacAddressToCBOR(mac, data, &cborDataLen);
  if (!res) {
    return res;
  }

  res = sendData(PacketManager::MessageType::DATA, data, cborDataLen);
  if (!res) {
    DEBUG_WARNING("BoardConfigurationProtocol::%s failed to send BLE MAC address", __FUNCTION__);
    return res;
  }
  return res;
}

bool BoardConfigurationProtocol::sendVersion(const char *version, MessageOutputType type) {
  bool res = false;

  size_t cborDataLen = CBOR_MIN_WIFI_FW_VERSION_LEN + strlen(version);
  uint8_t data[cborDataLen];

  switch (type)
  {
  case MessageOutputType::WIFI_FW_VERSION:       res = CBORAdapter::wifiFWVersionToCBOR      (version, data, &cborDataLen); break;
  case MessageOutputType::PROV_SKETCH_VERSION:   res = CBORAdapter::provSketchVersionToCBOR  (version, data, &cborDataLen); break;
  case MessageOutputType::NETCONFIG_LIB_VERSION: res = CBORAdapter::netConfigLibVersionToCBOR(version, data, &cborDataLen); break;
  default:                                                                                                           return false;
  }

  if (!res) {
    return res;
  }

  res = sendData(PacketManager::MessageType::DATA, data, cborDataLen);
  if (!res) {
    DEBUG_WARNING("BoardConfigurationProtocol::%s failed to send version of type %d", __FUNCTION__, (int)type);
  }

  return res;
}

BoardConfigurationProtocol::TransmissionResult BoardConfigurationProtocol::transmitStream() {
  if (!isPeerConnected()) {
    return TransmissionResult::PEER_NOT_AVAILABLE;
  }

  if (_outputMessagesList.size() == 0) {
    return TransmissionResult::COMPLETED;
  }

  TransmissionResult res = TransmissionResult::COMPLETED;

  for (std::list<OutputPacketBuffer>::iterator packet = _outputMessagesList.begin(); packet != _outputMessagesList.end(); ++packet) {
    if (packet->hasBytesToSend()) {
      res = TransmissionResult::NOT_COMPLETED;
      packet->incrementBytesSent(write(packet->get_ptrAt(packet->bytesSent()), packet->bytesToSend()));
      #if BCP_DEBUG_PACKET == 1
      DEBUG_DEBUG("BoardConfigurationProtocol::%s  transferred: %d of %d", __FUNCTION__, packet->bytesSent(), packet->len());
      #endif
      break;
    }
  }

  return res;
}

void BoardConfigurationProtocol::printPacket(const char *label, const uint8_t *data, size_t len) {
  if (Debug.getDebugLevel() == DBG_VERBOSE) {
    DEBUG_VERBOSE("Print %s data:", label);
    Debug.newlineOff();
    for (size_t i = 0; i < len; i++) {
      DEBUG_VERBOSE("%02x ", data[i]);
      if ((i + 1) % 10 == 0) {
        DEBUG_VERBOSE("\n");
      }
    }
    DEBUG_VERBOSE("\n");
  }
  Debug.newlineOn();
}

#endif // NETWORK_CONFIGURATOR_COMPATIBLE
