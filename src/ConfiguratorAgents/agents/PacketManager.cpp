/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "PacketManager.h"
#include "uCRC16Lib.h"
uint8_t PACKET_START[] = { 0x55, 0xaa };
uint8_t PACKET_END[] = { 0xaa, 0x55 };
#define PACKET_START_SIZE 2
#define PACKET_END_SIZE 2
#define PACKET_TYPE_SIZE 1
#define PACKET_LENGTH_SIZE 2
#define PACKET_CRC_SIZE 2
#define PACKET_HEADERS_OVERHEAD PACKET_START_SIZE + PACKET_TYPE_SIZE + PACKET_LENGTH_SIZE + PACKET_CRC_SIZE + PACKET_END_SIZE
#define PACKET_MINIMUM_SIZE PACKET_START_SIZE + PACKET_TYPE_SIZE + PACKET_LENGTH_SIZE

bool PacketManager::createPacket(OutputPacketBuffer &outputMsg, MessageType type, const uint8_t *data, size_t len) {
  uint16_t packetLen = len + PACKET_HEADERS_OVERHEAD;
  uint16_t payloadLen = len + PACKET_CRC_SIZE;
  uint8_t payloadLenHigh = payloadLen >> 8;
  uint8_t payloadLenLow = payloadLen & 0xff;
  uint16_t payloadCRC = uCRC16Lib::calculate((char *)data, len);
  uint8_t crcHigh = payloadCRC >> 8;
  uint8_t crcLow = payloadCRC & 0xff;

  outputMsg.allocate(packetLen);

  outputMsg += PACKET_START[0];
  outputMsg += PACKET_START[1];
  outputMsg += (uint8_t)type;
  outputMsg += payloadLenHigh;
  outputMsg += payloadLenLow;

  if (!outputMsg.copyArray(data, len)) {
    return false;
  }

  outputMsg += crcHigh;
  outputMsg += crcLow;
  outputMsg += PACKET_END[0];
  outputMsg += PACKET_END[1];

  outputMsg.startProgress();
  return true;
}

PacketManager::ReceivingState PacketManager::handleReceivedByte(ReceivedData &receivedData, uint8_t byte) {
  if (_state == ReceivingState::ERROR || _state == ReceivingState::RECEIVED) {
    _state = ReceivingState::WAITING_HEADER;
  }

  switch (_state) {
    case ReceivingState::WAITING_HEADER: _state = handle_WaitingHeader(byte); break;
    case ReceivingState::WAITING_PAYLOAD: _state = handle_WaitingPayload(byte); break;
    case ReceivingState::WAITING_END: _state = handle_WaitingEnd(byte); break;
    default: break;
  }

  if (_state == ReceivingState::RECEIVED) {
    receivedData.type = (MessageType)getPacketType();
    if (receivedData.type != MessageType::RESPONSE && receivedData.type != MessageType::DATA) {
      //Packet type not recognized
      Serial.print("Packet type not recognized: ");
      Serial.println((int)receivedData.type);
      _state = ReceivingState::WAITING_HEADER;
      return _state;
    }
    receivedData.payload = _tempInputMessagePayload;
    clearInputBuffers();
  }

  return _state;
}

void PacketManager::clear() {
  clearInputBuffers();
  _state = ReceivingState::WAITING_HEADER;
}

void PacketManager::clearInputBuffers() {
  _tempInputMessageHeader.clear();
  _tempInputMessagePayload.reset();
  _tempInputMessageEnd.clear();
}

bool PacketManager::checkBeginPacket() {
  if (_tempInputMessageHeader.len() < sizeof(PACKET_START)) {
    return false;
  }

  if (_tempInputMessageHeader[0] == PACKET_START[0] && _tempInputMessageHeader[1] == PACKET_START[1]) {
    return true;
  }

  return false;
}

bool PacketManager::checkEndPacket() {
  if (_tempInputMessageEnd.len() < sizeof(PACKET_END) + PACKET_CRC_SIZE) {
    return false;
  }
  if (_tempInputMessageEnd[2] == PACKET_END[0] && _tempInputMessageEnd[3] == PACKET_END[1]) {
    return true;
  }
  return false;
}

bool PacketManager::checkCRC() {
  if (_tempInputMessageEnd.len() < sizeof(PACKET_END) + PACKET_CRC_SIZE) {
    return false;
  }
  uint16_t receivedCRC = ((uint16_t)_tempInputMessageEnd[0] << 8 | _tempInputMessageEnd[1]);
  uint16_t computedCRC = uCRC16Lib::calculate((char *)&_tempInputMessagePayload[0], _tempInputMessagePayload.len());

  if (receivedCRC == computedCRC) {
    return true;
  }

  return false;
}

uint8_t PacketManager::getPacketType() {
  return _tempInputMessageHeader[2];
}

uint16_t PacketManager::getPacketLen() {
  if (_tempInputMessageHeader.len() < PACKET_MINIMUM_SIZE) {
    return 0;
  }

  return ((uint16_t)_tempInputMessageHeader[3] << 8 | _tempInputMessageHeader[4]);
}

PacketManager::ReceivingState PacketManager::handle_WaitingHeader(uint8_t byte) {
  ReceivingState nextState = _state;

  if (_tempInputMessageHeader.missingBytes() > 0) {
    _tempInputMessageHeader += byte;
  }

  if (_tempInputMessageHeader.receivedAll()) {
    Serial.println("Received header");
    if (!checkBeginPacket()) {
      Serial.println("Begin packet not recognized");
      clearInputBuffers();
      return ReceivingState::ERROR;
    }

    uint16_t packetLen = getPacketLen();
    uint16_t payloadLen = packetLen - PACKET_CRC_SIZE;
    _tempInputMessagePayload.allocate(payloadLen);  //The message contains only the CBOR payload
    _tempInputMessagePayload.setPayloadLen(payloadLen);
    nextState = ReceivingState::WAITING_PAYLOAD;
  }

  return nextState;
}

PacketManager::ReceivingState PacketManager::handle_WaitingPayload(uint8_t byte) {
  ReceivingState nextState = _state;

  if (_tempInputMessagePayload.missingBytes() > 0) {
    _tempInputMessagePayload += byte;
  }

  if (_tempInputMessagePayload.receivedAll()) {
    Serial.println("Received payload");
    nextState = ReceivingState::WAITING_END;
  }

  return nextState;
}

PacketManager::ReceivingState PacketManager::handle_WaitingEnd(uint8_t byte) {
  ReceivingState nextState = _state;

  if (_tempInputMessageEnd.missingBytes() > 0) {
    _tempInputMessageEnd += byte;
  }

  if (_tempInputMessageEnd.receivedAll()) {
    Serial.println("Received packet end");
    if (checkCRC() && checkEndPacket()) {
      nextState = ReceivingState::RECEIVED;
    } else {
      Serial.println("CRC or end packet not recognized");
      //Error
      clearInputBuffers();
      nextState = ReceivingState::ERROR;
    }
  }

  return nextState;
}

PacketManager Packet;