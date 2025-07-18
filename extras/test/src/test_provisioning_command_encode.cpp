/*
   Copyright (c) 2024 Arduino.  All rights reserved.
*/

/******************************************************************************
   INCLUDE
 ******************************************************************************/

 #include <catch2/catch_test_macros.hpp>

 #include <memory>
 #include <Encoder.h>
 #include <cbor/MessageEncoder.h>
 #include "../../src/configuratorAgents/agents/boardConfigurationProtocol/cbor/CBOR.h"
 #include "../../src/configuratorAgents/agents/boardConfigurationProtocol/cbor/CBORInstances.h"

 /******************************************************************************
    TEST CODE
  ******************************************************************************/

 SCENARIO("Test the encoding of command messages") {


   WHEN("Encode a message with provisioning status ")
   {
    StatusProvisioningMessage command;
    command.c.id = ProvisioningMessageId::StatusProvisioningMessageId;
    command.status = -100;
    uint8_t buffer[512];
    size_t bytes_encoded = sizeof(buffer);

    CBORMessageEncoder encoder;
    MessageEncoder::Status err = encoder.encode((Message*)&command, buffer, bytes_encoded);

    uint8_t expected_result[] = {
    0xda, 0x00, 0x01, 0x20, 0x00, 0x81, 0x38, 0x63
    };

    // Test the encoding is
    // Da 0001200     # tag(0x012000)
    //  81       # array(1)
    //  38 63 # negative(99)

    THEN("The encoding is successful") {
        REQUIRE(err == MessageEncoder::Status::Complete);
        REQUIRE(bytes_encoded == sizeof(expected_result));
        REQUIRE(memcmp(buffer, expected_result, sizeof(expected_result)) == 0);
    }
   }

   WHEN("Encode a message with provisioning wifi list ")
   {
    ListWifiNetworksProvisioningMessage command;
    command.c.id = ProvisioningMessageId::ListWifiNetworksProvisioningMessageId;
    command.numDiscoveredWiFiNetworks = 2;
    char ssid1[] = "SSID1";
    int rssi1 = -76;
    command.discoveredWifiNetworks[0].SSID = ssid1;
    command.discoveredWifiNetworks[0].RSSI = &rssi1;
    char ssid2[] = "SSID2";
    int rssi2 = -56;
    command.discoveredWifiNetworks[1].SSID = ssid2;
    command.discoveredWifiNetworks[1].RSSI = &rssi2;

    uint8_t buffer[512];
    size_t bytes_encoded = sizeof(buffer);

    CBORMessageEncoder encoder;
    MessageEncoder::Status err = encoder.encode((Message*)&command, buffer, bytes_encoded);

    uint8_t expected_result[] = {
    0xda, 0x00, 0x01, 0x20, 0x01, 0x84, 0x65, 0x53, 0x53, 0x49, 0x44, 0x31, 0x38, 0x4B, 0x65, 0x53, 0x53, 0x49, 0x44, 0x32, 0x38, 0x37
    };

    // Test the encoding is
    //DA 00012001        # tag(73729)
    //  84               # array(4)
    //     65            # text(5)
    //        5353494431 # "SSID1"
    //     38 4B         # negative(75)
    //     65            # text(5)
    //        5353494432 # "SSID2"
    //     38 37         # negative(55)
    THEN("The encoding is successful") {
        REQUIRE(err == MessageEncoder::Status::Complete);
        REQUIRE(bytes_encoded == sizeof(expected_result));
        REQUIRE(memcmp(buffer, expected_result, sizeof(expected_result)) == 0);
    }
   }

   WHEN("Encode a message with provisioning uniqueHardwareId ")
   {
    UniqueHardwareIdProvisioningMessage command;
    command.c.id = ProvisioningMessageId::UniqueHardwareIdProvisioningMessageId;
    memset(command.uniqueHardwareId, 0xCA, 32);
    uint8_t buffer[512];
    size_t bytes_encoded = sizeof(buffer);

    CBORMessageEncoder encoder;
    MessageEncoder::Status err = encoder.encode((Message*)&command, buffer, bytes_encoded);

    uint8_t expected_result[] = {
    0xda, 0x00, 0x01, 0x20, 0x10, 0x81, 0x58, 0x20,
    0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA,
    0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA,
    0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA,
    0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA,
    };

    // Test the encoding is
    //DA 00012010          # tag(73744)
    //  81                 # array(1)
    //    58 20            # bytes(32)
    //      CA.... omissis # values
    THEN("The encoding is successful") {
        REQUIRE(err == MessageEncoder::Status::Complete);
        REQUIRE(bytes_encoded == sizeof(expected_result));
        REQUIRE(memcmp(buffer, expected_result, sizeof(expected_result)) == 0);
    }
   }

     WHEN("Encode a message with provisioning  jwt ")
   {
    JWTProvisioningMessage command;
    command.c.id = ProvisioningMessageId::JWTProvisioningMessageId;
    memset(command.jwt, 0x00, 269);
    memset(command.jwt, 0xCA, 268);
    uint8_t buffer[512];
    size_t bytes_encoded = sizeof(buffer);

    CBORMessageEncoder encoder;
    MessageEncoder::Status err = encoder.encode((Message*)&command, buffer, bytes_encoded);

    uint8_t expected_result[] = {
    0xda, 0x00, 0x01, 0x20, 0x11, 0x81, 0x59, 0x01, 0x0C,
    0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA,
    0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA,
    0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA,
    0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA,
    0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA,
    0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA,
    0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA,
    0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA,
    0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA,
    0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA,
    0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA,
    0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA,
    0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA,
    0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA,
    0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA,
    0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA,
    0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA,
    0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA,
    0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA,
    0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA,
    0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA,
    0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA,
    0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA,
    0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA,
    0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA,
    0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA,
    0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA,
    0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA,
    0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA,
    0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA,
    0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA,
    0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA,
    0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA,
    0xCA, 0xCA, 0xCA, 0xCA,
    };

    // Test the encoding is
    //DA 00012011          # tag(73744)
    //  81                 # array(1)
    //    58 F6            # bytes(32)
    //      CA.... omissis # values
    THEN("The encoding is successful") {
        REQUIRE(err == MessageEncoder::Status::Complete);
        REQUIRE(bytes_encoded == sizeof(expected_result));
        REQUIRE(memcmp(buffer, expected_result, sizeof(expected_result)) == 0);
    }
   }

   WHEN("Encode a message with provisioning  public key")
   {
    ProvPublicKeyProvisioningMessage command;
    command.c.id = ProvisioningMessageId::ProvPublicKeyProvisioningMessageId;
    command.provPublicKey = "-----BEGIN PUBLIC KEY-----\nMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAE7JxCtXl5SvIrHmiasqyN4pyoXRlm44d5WXNpqmvJ\nk0tH8UpmIeHG7YPAkKLaqid95v/wLVoWeX5EbjxmlCkFtw==\n-----END PUBLIC KEY-----\n";
    uint8_t buffer[512];
    size_t bytes_encoded = sizeof(buffer);

    CBORMessageEncoder encoder;
    MessageEncoder::Status err = encoder.encode((Message*)&command, buffer, bytes_encoded);

    // Test the encoding is
    // DA 00012017                             # tag(73751)
    //    81                                   # array(1)
    //      78 B4                              # text(180)
    //           2D 2D 2D 2D 2D 4245 47 49 4E 20 50 55 42 4C 49 43204B45592D2D2D2D2D0A0A4D466B77457759484B6F5A497A6A3043415159494B6F5A497A6A3044415163445167414537
    //           4A784374586C3553764972486D69617371794E3470796F58526C6D3434643557584E70716D764A0A6B3074483855706D49654847375950416B4B4C617169643935762F774C566F5765583545626A786D6C436B4674773D3D0A0A2D2D2D2D2D454E44205055424C4943204B45592D2D2D2D2D0A

    uint8_t expected_result[] = {
        0xDA, 0x00, 0x01, 0x20, 0x17, 0x81, 0x78, 0xB2,
        0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x42, 0x45, 0x47, 0x49, 0x4E, 0x20, 0x50, 0x55, 0x42, 0x4C, 0x49,
        0x43, 0x20, 0x4B, 0x45, 0x59, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x0A, 0x4D, 0x46, 0x6B, 0x77,
        0x45, 0x77, 0x59, 0x48, 0x4B, 0x6F, 0x5A, 0x49,
        0x7A, 0x6A, 0x30, 0x43, 0x41, 0x51, 0x59, 0x49, 0x4B, 0x6F, 0x5A, 0x49,
        0x7A, 0x6A, 0x30, 0x44, 0x41, 0x51, 0x63, 0x44, 0x51, 0x67, 0x41, 0x45,
        0x37, 0x4A, 0x78, 0x43, 0x74, 0x58, 0x6C, 0x35, 0x53, 0x76, 0x49, 0x72,
        0x48, 0x6D, 0x69, 0x61, 0x73, 0x71, 0x79, 0x4E, 0x34, 0x70, 0x79, 0x6F,
        0x58, 0x52, 0x6C, 0x6D, 0x34, 0x34, 0x64, 0x35, 0x57, 0x58, 0x4E, 0x70,
        0x71, 0x6D, 0x76, 0x4A, 0x0A, 0x6B, 0x30, 0x74, 0x48, 0x38, 0x55, 0x70,
        0x6D, 0x49, 0x65, 0x48, 0x47, 0x37, 0x59, 0x50, 0x41, 0x6B, 0x4B, 0x4C,
        0x61, 0x71, 0x69, 0x64, 0x39, 0x35, 0x76, 0x2F, 0x77, 0x4C, 0x56, 0x6F,
        0x57, 0x65, 0x58, 0x35, 0x45, 0x62, 0x6A, 0x78, 0x6D, 0x6C, 0x43, 0x6B,
        0x46, 0x74, 0x77, 0x3D, 0x3D, 0x0A, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D,
        0x45, 0x4E, 0x44, 0x20, 0x50, 0x55, 0x42, 0x4C, 0x49, 0x43, 0x20, 0x4B,
        0x45, 0x59, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x0A
    };

    THEN("The encoding is successful") {
        REQUIRE(err == MessageEncoder::Status::Complete);
        REQUIRE(bytes_encoded == sizeof(expected_result));
        REQUIRE(memcmp(buffer, expected_result, sizeof(expected_result)) == 0);
    }
   }

   WHEN("Encode a message with provisioning  public key")
   {
    ProvPublicKeyProvisioningMessage command;
    command.c.id = ProvisioningMessageId::ProvPublicKeyProvisioningMessageId;
    command.provPublicKey = "-----BEGIN PUBLIC KEY-----\nMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAE7JxCtXl5SvIrHmiasqyN4pyoXRlm44d5WXNpqmvJ\nk0tH8UpmIeHG7YPAkKLaqid95v/wLVoWeX5EbjxmlCkFtw==\n-----END PUBLIC KEY-----\n";
    uint8_t buffer[50];
    size_t bytes_encoded = sizeof(buffer);

    CBORMessageEncoder encoder;
    MessageEncoder::Status err = encoder.encode((Message*)&command, buffer, bytes_encoded);

    THEN("The encoding is failing") {
        REQUIRE(err == MessageEncoder::Status::Error);
    }
   }

   WHEN("Encode a message with provisioning ble mac Address ")
   {
    BLEMacAddressProvisioningMessage command;
    command.c.id = ProvisioningMessageId::BLEMacAddressProvisioningMessageId;
    memset(command.macAddress, 0xAF, 6);
    uint8_t buffer[512];
    size_t bytes_encoded = sizeof(buffer);

    CBORMessageEncoder encoder;
    MessageEncoder::Status err = encoder.encode((Message*)&command, buffer, bytes_encoded);

    uint8_t expected_result[] = {
    0xda, 0x00, 0x01, 0x20, 0x13, 0x81, 0x46,
    0xAF, 0xAF, 0xAF, 0xAF, 0xAF, 0xAF
    };

    // Test the encoding is
    //DA 00012013       # tag(73747)
    //81                # array(1)
    //  46              # bytes(6)
    //     AFAFAFAFAFA
    THEN("The encoding is successful") {
        REQUIRE(err == MessageEncoder::Status::Complete);
        REQUIRE(bytes_encoded == sizeof(expected_result));
        REQUIRE(memcmp(buffer, expected_result, sizeof(expected_result)) == 0);
    }
   }

   WHEN("Encode a message with provisioning sketch version ")
   {
    ProvSketchVersionProvisioningMessage command;
    command.c.id = ProvisioningMessageId::ProvSketchVersionProvisioningMessageId;
    command.provSketchVersion = "1.6.0";
    uint8_t buffer[512];
    size_t bytes_encoded = sizeof(buffer);

    CBORMessageEncoder encoder;
    MessageEncoder::Status err = encoder.encode((Message*)&command, buffer, bytes_encoded);

    uint8_t expected_result[] = {
    0xda, 0x00, 0x01, 0x20, 0x15, 0x81, 0x65, 0x31, 0x2E, 0x36, 0x2E, 0x30
    };

    // Test the encoding is
    // DA 00012015       # tag(73749)
    //   81              # array(1)
    //     65            # text(5)
    //        312E362E30 # "1.6.0"
    THEN("The encoding is successful") {
        REQUIRE(err == MessageEncoder::Status::Complete);
        REQUIRE(bytes_encoded == sizeof(expected_result));
        REQUIRE(memcmp(buffer, expected_result, sizeof(expected_result)) == 0);
    }
   }

   WHEN("Encode a message with provisioning Network Configurator lib version ")
   {
    NetConfigLibVersionProvisioningMessage command;
    command.c.id = ProvisioningMessageId::NetConfigLibVersProvisioningMessageId;
    command.netConfigLibVersion = "1.6.0";
    uint8_t buffer[512];
    size_t bytes_encoded = sizeof(buffer);

    CBORMessageEncoder encoder;
    MessageEncoder::Status err = encoder.encode((Message*)&command, buffer, bytes_encoded);

    uint8_t expected_result[] = {
    0xda, 0x00, 0x01, 0x20, 0x16, 0x81, 0x65, 0x31, 0x2E, 0x36, 0x2E, 0x30
    };

    // Test the encoding is
    // DA 00012016       # tag(73750)
    //   81              # array(1)
    //     65            # text(5)
    //        312E362E30 # "1.6.0"
    THEN("The encoding is successful") {
        REQUIRE(err == MessageEncoder::Status::Complete);
        REQUIRE(bytes_encoded == sizeof(expected_result));
        REQUIRE(memcmp(buffer, expected_result, sizeof(expected_result)) == 0);
    }
   }
 }
