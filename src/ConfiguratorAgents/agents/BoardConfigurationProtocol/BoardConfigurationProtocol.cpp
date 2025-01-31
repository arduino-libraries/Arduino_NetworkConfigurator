/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/
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
  size_t len = buf->len();
  uint8_t data[len];
  memset(data, 0x00, len);
  memcpy(data, &(*buf)[0], len);
  _inputMessagesList.pop_front();

  ProvisioningCommandDown cborMsg;
  if (!CBORAdapter::getMsgFromCBOR(data, len, &cborMsg)) {
    DEBUG_DEBUG("BoardConfigurationProtocol::%s Invalid message", __FUNCTION__);
    sendStatus(StatusMessage::INVALID_PARAMS);
    return false;
  }

  if (cborMsg.c.id == CommandId::ProvisioningTimestamp) {
    uint64_t ts;
    if (!CBORAdapter::extractTimestamp(&cborMsg, &ts)) {
      DEBUG_DEBUG("BoardConfigurationProtocol::%s Invalid timestamp", __FUNCTION__);
      sendStatus(StatusMessage::INVALID_PARAMS);
      return false;
    }

    msg.type = MessageInputType::TIMESTAMP;
    msg.m.timestamp = ts;

  } else if (cborMsg.c.id == CommandId::ProvisioningCommands) {
    RemoteCommands cmd;
    if (!CBORAdapter::extractCommand(&cborMsg, &cmd)) {
      DEBUG_DEBUG("BoardConfigurationProtocol::%s Invalid command", __FUNCTION__);
      sendStatus(StatusMessage::INVALID_PARAMS);
      return false;
    }

    msg.type = MessageInputType::COMMANDS;
    msg.m.cmd = cmd;
  } else {
    msg.type = MessageInputType::NETWORK_SETTINGS;
    if (!CBORAdapter::extractNetworkSetting(&cborMsg, &msg.m.netSetting)) {
      DEBUG_DEBUG("BoardConfigurationProtocol::%s Invalid network Setting", __FUNCTION__);
      sendStatus(StatusMessage::INVALID_PARAMS);
      return false;
    }
  }

  return true;
}

bool BoardConfigurationProtocol::sendNewMsg(ProvisioningOutputMessage &msg) {
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

  if (hasReceivedBytes()) {
    int receivedDataLen = receivedBytes();

    PacketManager::ReceivingState res;
    PacketManager::ReceivedData receivedData;

    for (int i = 0; i < receivedDataLen; i++) {
      uint8_t val = readByte();
      res = Packet.handleReceivedByte(receivedData, val);
      if (res == PacketManager::ReceivingState::ERROR) {
        DEBUG_DEBUG("BoardConfigurationProtocol::%s Error receiving packet", __FUNCTION__);
        sendNak();
        transmissionRes = TransmissionResult::INVALID_DATA;
        break;
      } else if (res == PacketManager::ReceivingState::RECEIVED) {
        switch (receivedData.type) {
          case PacketManager::MessageType::DATA:
            {
              DEBUG_DEBUG("BoardConfigurationProtocol::%s Received data packet", __FUNCTION__);
              printPacket("payload", &receivedData.payload[0], receivedData.payload.len());
              _inputMessagesList.push_back(receivedData.payload);
              //Consider all sent data as received
              _outputMessagesList.clear();
              transmissionRes = TransmissionResult::DATA_RECEIVED;
            }
            break;
          case PacketManager::MessageType::TRANSMISSION_CONTROL:
            {
              if (receivedData.payload.len() == 1 && receivedData.payload[0] == 0x03) {

                DEBUG_DEBUG("BoardConfigurationProtocol::%s Received NACK packet", __FUNCTION__);
                for (std::list<OutputPacketBuffer>::iterator packet = _outputMessagesList.begin(); packet != _outputMessagesList.end(); ++packet) {
                  packet->startProgress();
                }
              } else if (receivedData.payload.len() == 1 && receivedData.payload[0] == 0x02) {
                DEBUG_DEBUG("BoardConfigurationProtocol::%s Received disconnect request", __FUNCTION__);
                handleDisconnectRequest();
              }
            }
            break;
          default:
            break;
        }
      }
    }
  }

  if (_outputMessagesList.size() > 0) {
    checkOutputPacketValidity();
    transmitStream();
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
    DEBUG_WARNING("BoardConfigurationProtocol::%s Failed to create packet", __FUNCTION__);
    return false;
  }

  printPacket("output message", &outputMsg[0], outputMsg.len());

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
  Packet.clear();
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
    DEBUG_ERROR("BoardConfigurationProtocol::%s failed encode status: %d ", __FUNCTION__, (int)msg);
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
    DEBUG_ERROR("BoardConfigurationProtocol::%s failed to convert uhwid to CBOR", __FUNCTION__);
    return res;
  }

  res = sendData(PacketManager::MessageType::DATA, data, cborDataLen);
  if (!res) {
    DEBUG_ERROR("BoardConfigurationProtocol::%s failed to send uhwid", __FUNCTION__);
    return res;
  }

  return res;
}

bool BoardConfigurationProtocol::sendJwt(const char *jwt, size_t len) {
  bool res = false;
  if (len > MAX_JWT_SIZE) {
    DEBUG_ERROR("BoardConfigurationProtocol::%s JWT too long", __FUNCTION__);
    return res;
  }

  size_t cborDataLen = CBOR_DATA_JWT_LEN;
  uint8_t data[cborDataLen];

  res = CBORAdapter::jwtToCBOR(jwt, data, &cborDataLen);
  if (!res) {
    DEBUG_ERROR("BoardConfigurationProtocol::%s failed to convert JWT to CBOR", __FUNCTION__);
    return res;
  }

  res = sendData(PacketManager::MessageType::DATA, data, cborDataLen);

  if (!res) {
    DEBUG_ERROR("BoardConfigurationProtocol::%s failed to send JWT", __FUNCTION__);
    return res;
  }

  return res;
}

bool BoardConfigurationProtocol::sendBleMacAddress(const uint8_t *mac, size_t len) {
  bool res = false;
  if (len != BLE_MAC_ADDRESS_SIZE) {
    DEBUG_ERROR("BoardConfigurationProtocol::%s Invalid BLE MAC address", __FUNCTION__);
    return res;
  }

  size_t cborDataLen = CBOR_DATA_BLE_MAC_LEN;
  uint8_t data[cborDataLen];

  res = CBORAdapter::BLEMacAddressToCBOR(mac, data, &cborDataLen);
  if (!res) {
    DEBUG_ERROR("BoardConfigurationProtocol::%s failed to convert BLE MAC address to CBOR", __FUNCTION__);
    return res;
  }

  res = sendData(PacketManager::MessageType::DATA, data, cborDataLen);
  if (!res) {
    DEBUG_ERROR("BoardConfigurationProtocol::%s failed to send BLE MAC address", __FUNCTION__);
    return res;
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
      packet->incrementBytesSent(writeBytes(&(*packet)[packet->bytesSent()], packet->bytesToSend()));
      DEBUG_DEBUG("BoardConfigurationProtocol::%s  transferred: %d of %d", __FUNCTION__, packet->bytesSent(), packet->len());
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
