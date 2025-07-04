/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "ANetworkConfigurator_Config.h"
#if NETWORK_CONFIGURATOR_COMPATIBLE

#include "Encoder.h"

MessageEncoder::Status StatusProvisioningMessageEncoder::encode(CborEncoder* encoder, Message *msg) {
  StatusProvisioningMessage * provisioningStatus = (StatusProvisioningMessage*) msg;
  CborEncoder array_encoder;

  if(cbor_encoder_create_array(encoder, &array_encoder, 1) != CborNoError) {
    return MessageEncoder::Status::Error;
  }

  if(cbor_encode_int(&array_encoder, provisioningStatus->status) != CborNoError) {
    return MessageEncoder::Status::Error;
  }

  if(cbor_encoder_close_container(encoder, &array_encoder) != CborNoError) {
    return MessageEncoder::Status::Error;
  }

  return MessageEncoder::Status::Complete;
}

MessageEncoder::Status ListWifiNetworksProvisioningMessageEncoder::encode(CborEncoder* encoder, Message *msg) {
  ListWifiNetworksProvisioningMessage * provisioningListWifiNetworks = (ListWifiNetworksProvisioningMessage*) msg;
  CborEncoder array_encoder;

  if(cbor_encoder_create_array(encoder,
      &array_encoder,
      2 * provisioningListWifiNetworks->numDiscoveredWiFiNetworks) != CborNoError) {
    return MessageEncoder::Status::Error;
  }

  for (int i = 0; i < provisioningListWifiNetworks->numDiscoveredWiFiNetworks; i++) {
    if(cbor_encode_text_stringz(&array_encoder, provisioningListWifiNetworks->discoveredWifiNetworks[i].SSID) != CborNoError ||
        cbor_encode_int(&array_encoder, *provisioningListWifiNetworks->discoveredWifiNetworks[i].RSSI) != CborNoError) {
      return MessageEncoder::Status::Error;
    }
  }

  if(cbor_encoder_close_container(encoder, &array_encoder) != CborNoError) {
    return MessageEncoder::Status::Error;
  }

  return MessageEncoder::Status::Complete;
}

MessageEncoder::Status UniqueHardwareIdProvisioningMessageEncoder::encode(CborEncoder* encoder, Message *msg) {
  UniqueHardwareIdProvisioningMessage * provisioningUniqueHardwareId = (UniqueHardwareIdProvisioningMessage*) msg;
  CborEncoder array_encoder;

  if(cbor_encoder_create_array(encoder, &array_encoder, 1) != CborNoError) {
    return MessageEncoder::Status::Error;
  }

  if(cbor_encode_byte_string(&array_encoder, (uint8_t *) provisioningUniqueHardwareId->uniqueHardwareId, UHWID_SIZE)) {
    return MessageEncoder::Status::Error;
  }

  if(cbor_encoder_close_container(encoder, &array_encoder) != CborNoError) {
    return MessageEncoder::Status::Error;
  }

  return MessageEncoder::Status::Complete;
}

MessageEncoder::Status JWTProvisioningMessageEncoder::encode(CborEncoder* encoder, Message *msg) {
  JWTProvisioningMessage * provisioningJWT = (JWTProvisioningMessage*) msg;
  CborEncoder array_encoder;

  if(cbor_encoder_create_array(encoder, &array_encoder, 1) != CborNoError) {
    return MessageEncoder::Status::Error;
  }

  if(cbor_encode_byte_string(&array_encoder, (uint8_t *) provisioningJWT->jwt, strlen(provisioningJWT->jwt))) {
    return MessageEncoder::Status::Error;
  }

  if(cbor_encoder_close_container(encoder, &array_encoder) != CborNoError) {
    return MessageEncoder::Status::Error;
  }

  return MessageEncoder::Status::Complete;
}

MessageEncoder::Status BLEMacAddressProvisioningMessageEncoder::encode(CborEncoder* encoder, Message *msg) {
  BLEMacAddressProvisioningMessage * provisioningBLEMacAddress = (BLEMacAddressProvisioningMessage*) msg;
  CborEncoder array_encoder;

  if(cbor_encoder_create_array(encoder, &array_encoder, 1) != CborNoError) {
    return MessageEncoder::Status::Error;
  }

  uint8_t size = 0;
  uint8_t emptyMac[] = {0, 0, 0, 0, 0, 0};
  if(memcmp(provisioningBLEMacAddress->macAddress, emptyMac, BLE_MAC_ADDRESS_SIZE) != 0) {
    size = BLE_MAC_ADDRESS_SIZE;
  }

  if(cbor_encode_byte_string(&array_encoder, provisioningBLEMacAddress->macAddress, size)) {
    return MessageEncoder::Status::Error;
  }

  if(cbor_encoder_close_container(encoder, &array_encoder) != CborNoError) {
    return MessageEncoder::Status::Error;
  }

  return MessageEncoder::Status::Complete;
}

MessageEncoder::Status ProvSketchVersionProvisioningMessageEncoder::encode(CborEncoder* encoder, Message *msg) {
  ProvSketchVersionProvisioningMessage * provisioningSketchVersion = (ProvSketchVersionProvisioningMessage*) msg;
  CborEncoder array_encoder;

  if(cbor_encoder_create_array(encoder, &array_encoder, 1) != CborNoError) {
    return MessageEncoder::Status::Error;
  }

  if(cbor_encode_text_stringz(&array_encoder, provisioningSketchVersion->provSketchVersion) != CborNoError) {
    return MessageEncoder::Status::Error;
  }

  if(cbor_encoder_close_container(encoder, &array_encoder) != CborNoError) {
    return MessageEncoder::Status::Error;
  }

  return MessageEncoder::Status::Complete;
}

MessageEncoder::Status NetConfigLibVersProvisioningMessageEncoder::encode(CborEncoder* encoder, Message *msg) {
  NetConfigLibVersionProvisioningMessage * netConfigLibVersion = (NetConfigLibVersionProvisioningMessage*) msg;
  CborEncoder array_encoder;

  if(cbor_encoder_create_array(encoder, &array_encoder, 1) != CborNoError) {
    return MessageEncoder::Status::Error;
  }

  if(cbor_encode_text_stringz(&array_encoder, netConfigLibVersion->netConfigLibVersion) != CborNoError) {
    return MessageEncoder::Status::Error;
  }

  if(cbor_encoder_close_container(encoder, &array_encoder) != CborNoError) {
    return MessageEncoder::Status::Error;
  }

  return MessageEncoder::Status::Complete;
}

MessageEncoder::Status ProvPublicKeyProvisioningMessageEncoder::encode(CborEncoder *encoder, Message *msg) {
  ProvPublicKeyProvisioningMessage * provisioningProvPublicKey = (ProvPublicKeyProvisioningMessage*) msg;
  CborEncoder array_encoder;

  if(cbor_encoder_create_array(encoder, &array_encoder, 1) != CborNoError) {
    return MessageEncoder::Status::Error;
  }

  if(cbor_encode_text_stringz(&array_encoder, provisioningProvPublicKey->provPublicKey) != CborNoError) {
    return MessageEncoder::Status::Error;
  }

  if(cbor_encoder_close_container(encoder, &array_encoder) != CborNoError) {
    return MessageEncoder::Status::Error;
  }

  return MessageEncoder::Status::Complete;
}

#endif // NETWORK_CONFIGURATOR_COMPATIBLE
