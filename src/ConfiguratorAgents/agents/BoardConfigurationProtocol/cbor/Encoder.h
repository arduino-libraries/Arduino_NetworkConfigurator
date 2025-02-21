/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once
#include <Arduino.h>

#include "ProvisioningMessage.h"
#include <Arduino_CBOR.h>

// TODO better naming
class StatusProvisioningMessageEncoder: public CBORMessageEncoderInterface {
public:
  StatusProvisioningMessageEncoder()
  : CBORMessageEncoderInterface(CBORStatusProvisioningMessage, StatusProvisioningMessageId) {}
protected:
  Encoder::Status encode(CborEncoder* encoder, Message *msg) override;
};

class ListWifiNetworksProvisioningMessageEncoder: public CBORMessageEncoderInterface {
public:
  ListWifiNetworksProvisioningMessageEncoder()
  : CBORMessageEncoderInterface(CBORListWifiNetworksProvisioningMessage, ListWifiNetworksProvisioningMessageId) {}
protected:
  Encoder::Status encode(CborEncoder* encoder, Message *msg) override;
};

class UniqueHardwareIdProvisioningMessageEncoder: public CBORMessageEncoderInterface {
public:
  UniqueHardwareIdProvisioningMessageEncoder()
  : CBORMessageEncoderInterface(CBORUniqueHardwareIdProvisioningMessage, UniqueHardwareIdProvisioningMessageId) {}
protected:
  Encoder::Status encode(CborEncoder* encoder, Message *msg) override;
};

class JWTProvisioningMessageEncoder: public CBORMessageEncoderInterface {
public:
  JWTProvisioningMessageEncoder()
  : CBORMessageEncoderInterface(CBORJWTProvisioningMessage, JWTProvisioningMessageId) {}
protected:
  Encoder::Status encode(CborEncoder* encoder, Message *msg) override;
};

class BLEMacAddressProvisioningMessageEncoder: public CBORMessageEncoderInterface {
public:
  BLEMacAddressProvisioningMessageEncoder()
  : CBORMessageEncoderInterface(CBORBLEMacAddressProvisioningMessage, BLEMacAddressProvisioningMessageId) {}
protected:
  Encoder::Status encode(CborEncoder* encoder, Message *msg) override;
};

class WiFiFWVersionProvisioningMessageEncoder: public CBORMessageEncoderInterface {
public:
  WiFiFWVersionProvisioningMessageEncoder()
  : CBORMessageEncoderInterface(CBORWiFiFWVersionProvisioningMessage, WiFiFWVersionProvisioningMessageId) {}
protected:
  Encoder::Status encode(CborEncoder* encoder, Message *msg) override;
};
