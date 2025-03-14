/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once
#include "Arduino.h"
#include <memory>

class PacketBuffer {
public:
  PacketBuffer(){};
  PacketBuffer(const PacketBuffer &obj)
    : _bytesTransferred(obj._bytesTransferred),
      _bytesToTransfer(obj._bytesToTransfer),
      _validityTs(obj._validityTs) {
    allocate(obj._size);
    memcpy(_buffer.get(), obj._buffer.get(), obj._size);
  };
  virtual ~PacketBuffer() = default;
  void setValidityTs(uint32_t ts) {
    _validityTs = ts;
  };
  uint32_t getValidityTs() {
    return _validityTs;
  };
  //Return the pointer to the internal buffer at specified index. Do not use this method to modify the buffer, only for reading
  const uint8_t *get_ptrAt(size_t idx){
    return idx < _size ? &get_ptr()[idx] : get_ptr();
  }
  //Return the pointer to the internal buffer. Do not use this method to modify the buffer, only for reading
  const uint8_t *get_ptr(){
    return _buffer.get();
  }
  //Operator overloaded for reading and updating the value a single byte of the buffer
  uint8_t &operator[](size_t idx) {
    return idx < _size ? _buffer[idx] : _buffer[0];
  };
  virtual PacketBuffer &operator+=(uint8_t newChar) = 0;
  //Append srcBuf to the internal buffer
  virtual bool copyArray(const uint8_t *srcBuf, size_t len) = 0;
  //Copy the srcBuf at the indicated position of the internal buffer
  virtual bool copyArray(int positionFrom, const uint8_t *srcBuf, size_t len) = 0;

  PacketBuffer &operator=(const PacketBuffer &msg) {
    _bytesTransferred = msg._bytesTransferred;
    _bytesToTransfer = msg._bytesToTransfer;
    _validityTs = msg._validityTs;
    allocate(msg._size);
    memcpy(_buffer.get(), msg._buffer.get(), msg._size);
  }

  void allocate(size_t n) {
    _buffer = std::unique_ptr<uint8_t[]>(new uint8_t[n]);
    _size = n;
  }

  void reset() {
    if (_buffer) {
      _buffer.reset();
    }
    _size = 0;
    _bytesTransferred = 0;
    _bytesToTransfer = 0;
    _validityTs = 0;
  }

protected:
  bool hasPendingBytes() {
    return _bytesToTransfer - _bytesTransferred > 0;
  };
  uint32_t transferredBytes() {
    return _bytesTransferred;
  };
  uint32_t pendingBytes() {
    return _bytesToTransfer - _bytesTransferred;
  };
  uint32_t bytesToTransfer() {
    return _bytesToTransfer;
  };
  void setBytesTransferred(int n) {
    _bytesTransferred = n;
  };
  void setBytesToTransfer(int n) {
    _bytesToTransfer = n;
  };
  size_t size() {
    return _size;
  };
  bool setBytes(int position, uint8_t *newChar, size_t len) {
    if (position + len <= _size) {
      memcpy(&_buffer[position], newChar, len);
      return true;
    }
    return false;
  }

private:
  std::unique_ptr<uint8_t[]> _buffer;
  size_t _size = 0;
  uint32_t _bytesTransferred = 0;
  uint32_t _bytesToTransfer = 0;
  uint32_t _validityTs = 0;
};

class OutputPacketBuffer : public PacketBuffer {
public:
  OutputPacketBuffer(){};
  uint32_t len() {
    return bytesToTransfer();
  };
  uint32_t bytesSent() {
    return transferredBytes();
  };
  uint32_t bytesToSend() {
    return pendingBytes();
  };
  bool hasBytesToSend() {
    return hasPendingBytes();
  };
  void bytesSent(int n) {
    setBytesTransferred(n);
  };
  void incrementBytesSent(int n) {
    setBytesTransferred(transferredBytes() + n);
  };
  void startProgress() {
    setBytesTransferred(0);
  };
  void clear() {
    setBytesTransferred(0);
  };

  OutputPacketBuffer &operator+=(uint8_t newChar) {
    copyArray(bytesToTransfer(), &newChar, sizeof(newChar));
  }

  bool copyArray(const uint8_t *srcBuf, size_t len) {
    copyArray(bytesToTransfer(), srcBuf, len);
  }

  bool copyArray(int positionFrom, const uint8_t *srcBuf, size_t len) override {
    int nextOccupation = bytesToTransfer() + len;
    bool success = false;
    if (nextOccupation <= size()) {
      setBytes(positionFrom, const_cast<uint8_t *>(srcBuf), len);
      setBytesToTransfer(nextOccupation);
      success = true;
    }

    return success;
  }
};

class InputPacketBuffer : public PacketBuffer {
public:
  InputPacketBuffer(){};
  InputPacketBuffer(int n) {
    allocate(n);
    setPayloadLen(n);
  };
  bool receivedAll() {
    return !hasPendingBytes() && bytesToTransfer() > 0;
  };
  uint32_t missingBytes() {
    return pendingBytes();
  };
  uint32_t len() {
    return transferredBytes();
  };
  void setPayloadLen(uint32_t len) {
    setBytesToTransfer(len);
  };
  void clear() {
    setBytesTransferred(0);
  };
  InputPacketBuffer &operator+=(uint8_t newChar) {
    copyArray(transferredBytes(), &newChar, sizeof(newChar));
  }

  bool copyArray(const uint8_t *srcBuf, size_t len) {
    copyArray(transferredBytes(), srcBuf, len);
  }

  bool copyArray(int positionFrom, const uint8_t *srcBuf, size_t len) override {
    int nextOccupation = transferredBytes() + len;
    bool success = false;
    if (nextOccupation <= size()) {
      setBytes(positionFrom, const_cast<uint8_t *>(srcBuf), len);
      setBytesTransferred(nextOccupation);
      success = true;
    }

    return success;
  }
};
