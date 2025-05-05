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

   WHEN("Encode a message with provisioning wifi fw version ")
   {
    WiFiFWVersionProvisioningMessage command;
    command.c.id = ProvisioningMessageId::WiFiFWVersionProvisioningMessageId;
    command.wifiFwVersion = "1.6.0";
    uint8_t buffer[512];
    size_t bytes_encoded = sizeof(buffer);

    CBORMessageEncoder encoder;
    MessageEncoder::Status err = encoder.encode((Message*)&command, buffer, bytes_encoded);

    uint8_t expected_result[] = {
    0xda, 0x00, 0x01, 0x20, 0x14, 0x81, 0x65, 0x31, 0x2E, 0x36, 0x2E, 0x30
    };

    // Test the encoding is
    //DA 00012014     # tag(73748)
    // 81             # array(1)
    //   65           # text(5)
    //     312E362E30 # "1.6.0"
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
    printf("res %d\n", (int)err);
    THEN("The encoding is successful") {
        REQUIRE(err == MessageEncoder::Status::Complete);
        REQUIRE(bytes_encoded == sizeof(expected_result));
        REQUIRE(memcmp(buffer, expected_result, sizeof(expected_result)) == 0);
    }
   }
 }
