/*
   Copyright (c) 2024 Arduino.  All rights reserved.
*/

/******************************************************************************
   INCLUDE
 ******************************************************************************/

 #include <catch2/catch_test_macros.hpp>
 #include <string.h>

 #include <memory>

 #include <Decoder.h>
 #include <cbor/MessageDecoder.h>
 #include "../../src/configuratorAgents/agents/boardConfigurationProtocol/cbor/CBOR.h"
 #include "../../src/configuratorAgents/agents/boardConfigurationProtocol/cbor/CBORInstances.h"
 #include <IPAddress.h>

 /******************************************************************************
    TEST CODE
  ******************************************************************************/

 SCENARIO("Test the decoding of command messages") {
   /****************************************************************************/

   WHEN("Decode a provisioning timestamp message")
   {
    ProvisioningMessageDown command;

     uint8_t const payload[] = {0xda, 0x00, 0x01, 0x20, 0x02, 0x81, 0x1B, 0x00, 0x00,
     0x00, 0x00, 0x67, 0x06, 0x6F, 0xE4};
     /*
     DA 00012002             # tag(73730)
       81                    # array(1)
         1B 0000000067066FE4 # unsigned(1728475108)
     */
     uint64_t ts = 1728475108;
     size_t payload_length = sizeof(payload) / sizeof(uint8_t);
     CBORMessageDecoder decoder;
     MessageDecoder::Status err =  decoder.decode((Message*)&command, payload, payload_length);

     THEN("The decode is successful") {
       REQUIRE(err == MessageDecoder::Status::Complete);
       REQUIRE(command.c.id == ProvisioningMessageId::TimestampProvisioningMessageId);
       REQUIRE(command.provisioningTimestamp.timestamp == ts);
     }
   }

   /****************************************************************************/

   WHEN("Decode a provisioning command message")
   {
    ProvisioningMessageDown command;
     /*
       DA 00012003 # tag(73731)
         81       # array(1)
           18 64 # unsigned(100)
     */
     uint8_t const payload[] = {0xda, 0x00, 0x01, 0x20, 0x03, 0x81, 0x18, 0x64};
     uint8_t commandId = 100;
     size_t payload_length = sizeof(payload) / sizeof(uint8_t);
     CBORMessageDecoder decoder;
     MessageDecoder::Status err =  decoder.decode((Message*)&command, payload, payload_length);

     THEN("The decode is successful") {
       REQUIRE(err == MessageDecoder::Status::Complete);
       REQUIRE(command.c.id == ProvisioningMessageId::CommandsProvisioningMessageId);
       REQUIRE(command.provisioningCommands.cmd == commandId);
     }
   }

   /****************************************************************************/

   WHEN("Decode a provisioning wifi configuration message")
   {
    ProvisioningMessageDown command;
     /*
       DA 00012004                      # tag(73732)
         82                             # array(2)
           65                           # text(5)
             5353494431                 # "SSID1"
           6D                           # text(13)
             50415353574F52445353494431 # "PASSWORDSSID1"
     */
     uint8_t const payload[] = {0xda, 0x00, 0x01, 0x20, 0x04, 0x82, 0x65, 0x53,
                                0x53, 0x49, 0x44, 0x31, 0x6d, 0x50, 0x41, 0x53,
                                0x53, 0x57, 0x4f, 0x52, 0x44, 0x53, 0x53, 0x49,
                                0x44, 0x31};
     char ssid[] = "SSID1";
     char password[] = "PASSWORDSSID1";
     size_t payload_length = sizeof(payload) / sizeof(uint8_t);
     CBORMessageDecoder decoder;
     MessageDecoder::Status err =  decoder.decode((Message*)&command, payload, payload_length);
     THEN("The decode is successful") {
       REQUIRE(err == MessageDecoder::Status::Complete);
       REQUIRE(command.c.id == ProvisioningMessageId::WifiConfigProvisioningMessageId);
       REQUIRE(command.provisioningNetworkConfig.networkSetting.type == NetworkAdapter::WIFI);
       REQUIRE(strcmp(command.provisioningNetworkConfig.networkSetting.wifi.ssid, ssid) == 0);
       REQUIRE(strcmp(command.provisioningNetworkConfig.networkSetting.wifi.pwd, password) == 0);
     }
   }

   /****************************************************************************/

   WHEN("Decode a provisioning LoRaWan configuration message")
   {
    ProvisioningMessageDown command;
     /*
       DA 00012005             # tag(73733)
    85                   # array(5)
       67                # text(7)
          41505045554931 # "APPEUI1"
       66                # text(6)
          4150504B4559   # "APPKEY"
       05                # unsigned(5)
       65                # text(5)
          3031313130     # "01110"
       61                # text(1)
          41             # "A"
     */
     uint8_t const payload[] = {0xda, 0x00, 0x01, 0x20, 0x05, 0x85, 0x67, 0x41,
                                0x50, 0x50, 0x45, 0x55, 0x49, 0x31, 0x66, 0x41,
                                0x50, 0x50, 0x4B, 0x45, 0x59, 0x05, 0x65, 0x30,
                                0x31, 0x31, 0x31, 0x30, 0x61, 0x41};
     char appeui[] = "APPEUI1";
     char appkey[] = "APPKEY";
     char channelMask[] = "01110";
     char deviceType = 'A';
     uint8_t band = 5;
     size_t payload_length = sizeof(payload) / sizeof(uint8_t);
     CBORMessageDecoder decoder;
     MessageDecoder::Status err =  decoder.decode((Message*)&command, payload, payload_length);

     THEN("The decode is successful") {
       REQUIRE(err == MessageDecoder::Status::Complete);
       REQUIRE(command.c.id == ProvisioningMessageId::LoRaConfigProvisioningMessageId);
       REQUIRE(command.provisioningNetworkConfig.networkSetting.type == NetworkAdapter::LORA);
       REQUIRE(strcmp(command.provisioningNetworkConfig.networkSetting.lora.appeui, appeui) == 0);
       REQUIRE(strcmp(command.provisioningNetworkConfig.networkSetting.lora.appkey, appkey) == 0);
       REQUIRE(command.provisioningNetworkConfig.networkSetting.lora.band == band);
       REQUIRE(strcmp(command.provisioningNetworkConfig.networkSetting.lora.channelMask, channelMask) == 0);
       REQUIRE(command.provisioningNetworkConfig.networkSetting.lora.deviceClass == deviceType);
     }
   }

  /****************************************************************************/

  WHEN("Decode a provisioning LoRaWan configuration message with defaults")
  {
  ProvisioningMessageDown command;
    /*
    DA 00012005           # tag(73733)
      85                  # array(5)
        67                # text(7)
          41505045554931  # "APPEUI1"
        66                # text(6)
          4150504B4559    # "APPKEY"
        20                # negative(0)
        60                # text(0)
                          # ""
        60                # text(0)
                          # ""
    */
    uint8_t const payload[] = {0xda, 0x00, 0x01, 0x20, 0x05, 0x85, 0x67, 0x41,
                              0x50, 0x50, 0x45, 0x55, 0x49, 0x31, 0x66, 0x41,
                              0x50, 0x50, 0x4B, 0x45, 0x59, 0x20, 0x60, 0x60};
    char appeui[] = "APPEUI1";
    char appkey[] = "APPKEY";
    char deviceType = models::settingsDefault(NetworkAdapter::LORA).lora.deviceClass;
    uint8_t band = models::settingsDefault(NetworkAdapter::LORA).lora.band;
    size_t payload_length = sizeof(payload) / sizeof(uint8_t);
    CBORMessageDecoder decoder;
    MessageDecoder::Status err =  decoder.decode((Message*)&command, payload, payload_length);

    THEN("The decode is successful") {
      REQUIRE(err == MessageDecoder::Status::Complete);
      REQUIRE(command.c.id == ProvisioningMessageId::LoRaConfigProvisioningMessageId);
      REQUIRE(command.provisioningNetworkConfig.networkSetting.type == NetworkAdapter::LORA);
      REQUIRE(strcmp(command.provisioningNetworkConfig.networkSetting.lora.appeui, appeui) == 0);
      REQUIRE(strcmp(command.provisioningNetworkConfig.networkSetting.lora.appkey, appkey) == 0);
      REQUIRE(command.provisioningNetworkConfig.networkSetting.lora.band == band);
      REQUIRE(strcmp(command.provisioningNetworkConfig.networkSetting.lora.channelMask, models::settingsDefault(NetworkAdapter::LORA).lora.channelMask) == 0);
      REQUIRE(command.provisioningNetworkConfig.networkSetting.lora.deviceClass == deviceType);
    }
  }

   /****************************************************************************/

   WHEN("Decode a provisioning GSM complete configuration message")
   {
    ProvisioningMessageDown command;
     /*
       DA 00012006                        # tag(73734)
         84                               # array(4)
           68                             # text(8)
             3132333435363738             # "12345678"
           6E                             # text(14)
             61706E2E61726475696E6F2E6363 # "apn.arduino.cc"
           68                             # text(8)
             5445535455534552             # "TESTUSER"
           6C                             # text(12)
             5445535450415353574F5244     # "TESTPASSWORD"
     */
     uint8_t const payload[] = {0xda, 0x00, 0x01, 0x20, 0x06, 0x84, 0x68, 0x31,
                                0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x6e,
                                0x61, 0x70, 0x6e, 0x2e, 0x61, 0x72, 0x64, 0x75,
                                0x69, 0x6e, 0x6f, 0x2e, 0x63, 0x63, 0x68, 0x54,
                                0x45, 0x53, 0x54, 0x55, 0x53, 0x45, 0x52, 0x6c,
                                0x54, 0x45, 0x53, 0x54, 0x50, 0x41, 0x53, 0x53,
                                0x57, 0x4f, 0x52, 0x44};
     char pin[] = "12345678";
     char apn[] = "apn.arduino.cc";
     char username[] = "TESTUSER";
     char password[] = "TESTPASSWORD";

     size_t payload_length = sizeof(payload) / sizeof(uint8_t);
     CBORMessageDecoder decoder;
     MessageDecoder::Status err =  decoder.decode((Message*)&command, payload, payload_length);

     THEN("The decode is successful") {
       REQUIRE(err == MessageDecoder::Status::Complete);
       REQUIRE(command.c.id == ProvisioningMessageId::GSMConfigProvisioningMessageId);
       REQUIRE(command.provisioningNetworkConfig.networkSetting.type == NetworkAdapter::GSM);
       REQUIRE(strcmp(command.provisioningNetworkConfig.networkSetting.gsm.pin, pin) == 0);
       REQUIRE(strcmp(command.provisioningNetworkConfig.networkSetting.gsm.apn, apn) == 0);
       REQUIRE(strcmp(command.provisioningNetworkConfig.networkSetting.gsm.login, username) == 0);
       REQUIRE(strcmp(command.provisioningNetworkConfig.networkSetting.gsm.pass, password) == 0);

     }
   }

   /****************************************************************************/

   WHEN("Decode a provisioning GSM configuration message with empty PIN")
   {
    ProvisioningMessageDown command;
     /*
       DA 00012006                          # tag(73734)
         84                                 # array(4)
             60                             # text(0)
                                            # ""
             6E                             # text(14)
               61706E2E61726475696E6F2E6363 # "apn.arduino.cc"
             68                             # text(8)
               5445535455534552             # "TESTUSER"
             6C                             # text(12)
               5445535450415353574F5244     # "TESTPASSWORD"
     */
     uint8_t const payload[] = {0xda, 0x00, 0x01, 0x20, 0x06, 0x84, 0x60, 0x6e, 0x61,
                                0x70, 0x6e, 0x2e, 0x61, 0x72, 0x64, 0x75, 0x69, 0x6e,
                                0x6f, 0x2e, 0x63, 0x63, 0x68, 0x54, 0x45, 0x53, 0x54,
                                0x55, 0x53, 0x45, 0x52, 0x6c, 0x54, 0x45, 0x53, 0x54,
                                0x50, 0x41, 0x53, 0x53, 0x57, 0x4f, 0x52, 0x44};
     char pin[] = "";
     char apn[] = "apn.arduino.cc";
     char username[] = "TESTUSER";
     char password[] = "TESTPASSWORD";

     size_t payload_length = sizeof(payload) / sizeof(uint8_t);
     CBORMessageDecoder decoder;
     MessageDecoder::Status err =  decoder.decode((Message*)&command, payload, payload_length);

     THEN("The decode is successful") {
      REQUIRE(err == MessageDecoder::Status::Complete);
      REQUIRE(command.c.id == ProvisioningMessageId::GSMConfigProvisioningMessageId);
      REQUIRE(command.provisioningNetworkConfig.networkSetting.type == NetworkAdapter::GSM);
      REQUIRE(strcmp(command.provisioningNetworkConfig.networkSetting.gsm.pin, pin) == 0);
      REQUIRE(strcmp(command.provisioningNetworkConfig.networkSetting.gsm.apn, apn) == 0);
      REQUIRE(strcmp(command.provisioningNetworkConfig.networkSetting.gsm.login, username) == 0);
      REQUIRE(strcmp(command.provisioningNetworkConfig.networkSetting.gsm.pass, password) == 0);

     }
   }

   /****************************************************************************/

   WHEN("Decode a provisioning GSM configuration message with PIN, Login and Password empty")
   {
    ProvisioningMessageDown command;
     /*
       DA 00012006                          # tag(73734)
         84                                 # array(4)
             60                             # text(0)
                                            # ""
             6E                             # text(14)
               61706E2E61726475696E6F2E6363 # "apn.arduino.cc"
             60                             # text(0)
                                            # ""
             60                             # text(0)
                                            # ""
     */
     uint8_t const payload[] = {0xda, 0x00, 0x01, 0x20, 0x06, 0x84, 0x60, 0x6e, 0x61,
                                0x70, 0x6e, 0x2e, 0x61, 0x72, 0x64, 0x75, 0x69, 0x6e,
                                0x6f, 0x2e, 0x63, 0x63, 0x60, 0x60};
     char pin[] = "";
     char apn[] = "apn.arduino.cc";
     char username[] = "";
     char password[] = "";

     size_t payload_length = sizeof(payload) / sizeof(uint8_t);
     CBORMessageDecoder decoder;
     MessageDecoder::Status err =  decoder.decode((Message*)&command, payload, payload_length);

     THEN("The decode is successful") {
      REQUIRE(err == MessageDecoder::Status::Complete);
      REQUIRE(command.c.id == ProvisioningMessageId::GSMConfigProvisioningMessageId);
      REQUIRE(command.provisioningNetworkConfig.networkSetting.type == NetworkAdapter::GSM);
      REQUIRE(strcmp(command.provisioningNetworkConfig.networkSetting.gsm.pin, pin) == 0);
      REQUIRE(strcmp(command.provisioningNetworkConfig.networkSetting.gsm.apn, apn) == 0);
      REQUIRE(strcmp(command.provisioningNetworkConfig.networkSetting.gsm.login, username) == 0);
      REQUIRE(strcmp(command.provisioningNetworkConfig.networkSetting.gsm.pass, password) == 0);
     }
   }

   /****************************************************************************/

   WHEN("Decode a provisioning NB-IoT complete configuration message")
   {
    ProvisioningMessageDown command;
     /*
       DA 00012006                        # tag(73734)
         84                               # array(4)
           68                             # text(8)
             3132333435363738             # "12345678"
           6E                             # text(14)
             61706E2E61726475696E6F2E6363 # "apn.arduino.cc"
           68                             # text(8)
             5445535455534552             # "TESTUSER"
           6C                             # text(12)
             5445535450415353574F5244     # "TESTPASSWORD"
     */
     uint8_t const payload[] = {0xda, 0x00, 0x01, 0x20, 0x07, 0x84, 0x68, 0x31,
                                0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x6e,
                                0x61, 0x70, 0x6e, 0x2e, 0x61, 0x72, 0x64, 0x75,
                                0x69, 0x6e, 0x6f, 0x2e, 0x63, 0x63, 0x68, 0x54,
                                0x45, 0x53, 0x54, 0x55, 0x53, 0x45, 0x52, 0x6c,
                                0x54, 0x45, 0x53, 0x54, 0x50, 0x41, 0x53, 0x53,
                                0x57, 0x4f, 0x52, 0x44};
     char pin[] = "12345678";
     char apn[] = "apn.arduino.cc";
     char username[] = "TESTUSER";
     char password[] = "TESTPASSWORD";

     size_t payload_length = sizeof(payload) / sizeof(uint8_t);
     CBORMessageDecoder decoder;
     MessageDecoder::Status err =  decoder.decode((Message*)&command, payload, payload_length);

     THEN("The decode is successful") {
       REQUIRE(err == MessageDecoder::Status::Complete);
       REQUIRE(command.c.id == ProvisioningMessageId::NBIOTConfigProvisioningMessageId);
       REQUIRE(command.provisioningNetworkConfig.networkSetting.type == NetworkAdapter::NB);
       REQUIRE(strcmp(command.provisioningNetworkConfig.networkSetting.nb.pin, pin) == 0);
       REQUIRE(strcmp(command.provisioningNetworkConfig.networkSetting.nb.apn, apn) == 0);
       REQUIRE(strcmp(command.provisioningNetworkConfig.networkSetting.nb.login, username) == 0);
       REQUIRE(strcmp(command.provisioningNetworkConfig.networkSetting.nb.pass, password) == 0);
     }
   }

   /****************************************************************************/

   WHEN("Decode a provisioning CATM1 complete configuration message")
   {
    ProvisioningMessageDown command;
     /*
     DA 00012008                          # tag(73736)
       85                                 # array(5)
           68                             # text(8)
             3132333435363738             # "12345678"
           84                             # array(4)
             01                           # unsigned(1)
             02                           # unsigned(2)
             1A 00080000                  # unsigned(524288)
             1A 08000000                  # unsigned(134217728)
           6E                             # text(14)
             61706E2E61726475696E6F2E6363 # "apn.arduino.cc"
           68                             # text(8)
             5445535455534552             # "TESTUSER"
           6C                             # text(12)
             5445535450415353574F5244     # "TESTPASSWORD"
     */
    uint8_t const payload[] = {0xda, 0x00, 0x01, 0x20, 0x08, 0x85, 0x68, 0x31,
                              0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x84,
                              0x01, 0x02, 0x1A, 0x00, 0x08, 0x00, 0x00, 0x1A,
                              0x08, 0x00, 0x00, 0x00, 0x6E, 0x61, 0x70, 0x6E,
                              0x2E, 0x61, 0x72, 0x64, 0x75, 0x69, 0x6E, 0x6F,
                              0x2E, 0x63, 0x63, 0x68, 0x54, 0x45, 0x53, 0x54,
                              0x55, 0x53, 0x45, 0x52, 0x6C, 0x54, 0x45, 0x53,
                              0x54, 0x50, 0x41, 0x53, 0x53, 0x57, 0x4F, 0x52,
                              0x44};
    char pin[] = "12345678";
    char apn[] = "apn.arduino.cc";
    char username[] = "TESTUSER";
    char password[] = "TESTPASSWORD";
    uint32_t band = 0x8080003; //{0x01, 0x02, 0x80000, 0x8000000};
    uint8_t rat = models::settingsDefault(NetworkAdapter::CATM1).catm1.rat;

    size_t payload_length = sizeof(payload) / sizeof(uint8_t);
    CBORMessageDecoder decoder;
    MessageDecoder::Status err =  decoder.decode((Message*)&command, payload, payload_length);

    THEN("The decode is successful") {
      REQUIRE(err == MessageDecoder::Status::Complete);
      REQUIRE(command.c.id == ProvisioningMessageId::CATM1ConfigProvisioningMessageId);
      REQUIRE(command.provisioningNetworkConfig.networkSetting.type == NetworkAdapter::CATM1);
      REQUIRE(command.provisioningNetworkConfig.networkSetting.catm1.rat == rat);
      REQUIRE(strcmp(command.provisioningNetworkConfig.networkSetting.catm1.pin, pin) == 0);
      REQUIRE(strcmp(command.provisioningNetworkConfig.networkSetting.catm1.apn, apn) == 0);
      REQUIRE(strcmp(command.provisioningNetworkConfig.networkSetting.catm1.login, username) == 0);
      REQUIRE(strcmp(command.provisioningNetworkConfig.networkSetting.catm1.pass, password) == 0);
      REQUIRE(command.provisioningNetworkConfig.networkSetting.catm1.band == band);
    }
   }

   /****************************************************************************/

   WHEN("Decode a provisioning CATM1 configuration message with only one band")
   {
    ProvisioningMessageDown command;
    /*
    DA 00012008                           # tag(73736)
      85                                  # array(5)
          68                              # text(8)
            3132333435363738              # "12345678"
          81                              # array(1)
            01                            # unsigned(1)
          6E                              # text(14)
            61706E2E61726475696E6F2E6363  # "apn.arduino.cc"
          68                              # text(8)
            5445535455534552              # "TESTUSER"
          6C                              # text(12)
            5445535450415353574F5244      # "TESTPASSWORD"
    */
    uint8_t const payload[] = {0xda, 0x00, 0x01, 0x20, 0x08, 0x85, 0x68, 0x31,
                              0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x81,
                              0x01, 0x6E, 0x61, 0x70, 0x6E, 0x2E, 0x61, 0x72,
                              0x64, 0x75, 0x69, 0x6E, 0x6F, 0x2E, 0x63, 0x63,
                              0x68, 0x54, 0x45, 0x53, 0x54, 0x55, 0x53, 0x45,
                              0x52, 0x6C, 0x54, 0x45, 0x53, 0x54, 0x50, 0x41,
                              0x53, 0x53, 0x57, 0x4F, 0x52, 0x44};
    char pin[] = "12345678";
    char apn[] = "apn.arduino.cc";
    char username[] = "TESTUSER";
    char password[] = "TESTPASSWORD";
    uint32_t band = 0x01;
    uint8_t rat = models::settingsDefault(NetworkAdapter::CATM1).catm1.rat;

    size_t payload_length = sizeof(payload) / sizeof(uint8_t);
    CBORMessageDecoder decoder;
    MessageDecoder::Status err =  decoder.decode((Message*)&command, payload, payload_length);

    THEN("The decode is successful") {
      REQUIRE(err == MessageDecoder::Status::Complete);
      REQUIRE(command.c.id == ProvisioningMessageId::CATM1ConfigProvisioningMessageId);
      REQUIRE(command.provisioningNetworkConfig.networkSetting.type == NetworkAdapter::CATM1);
      REQUIRE(command.provisioningNetworkConfig.networkSetting.catm1.rat == rat);
      REQUIRE(strcmp(command.provisioningNetworkConfig.networkSetting.catm1.pin, pin) == 0);
      REQUIRE(strcmp(command.provisioningNetworkConfig.networkSetting.catm1.apn, apn) == 0);
      REQUIRE(strcmp(command.provisioningNetworkConfig.networkSetting.catm1.login, username) == 0);
      REQUIRE(strcmp(command.provisioningNetworkConfig.networkSetting.catm1.pass, password) == 0);
      REQUIRE(command.provisioningNetworkConfig.networkSetting.catm1.band == band);
    }
   }

   /****************************************************************************/

   WHEN("Decode a provisioning CATM1 configuration message without band, username, password")
   {
    ProvisioningMessageDown command;
     /*
     DA 00012008                           # tag(73736)
       85                                  # array(5)
           68                              # text(8)
             3132333435363738              # "12345678"
           80                              # array(0)
           6E                              # text(14)
             61706E2E61726475696E6F2E6363  # "apn.arduino.cc"
           60                              # text(0)
                                           # ""
           60                              # text(0)
                                           # ""
     */
    uint8_t const payload[] = {0xda, 0x00, 0x01, 0x20, 0x08, 0x85, 0x68, 0x31,
                              0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x80,
                              0x6E, 0x61, 0x70, 0x6E, 0x2E, 0x61, 0x72, 0x64,
                              0x75, 0x69, 0x6E, 0x6F, 0x2E, 0x63, 0x63, 0x60,
                              0x60};
    char pin[] = "12345678";
    char apn[] = "apn.arduino.cc";
    char username[] = "";
    char password[] = "";
    uint32_t band = models::settingsDefault(NetworkAdapter::CATM1).catm1.band;
    uint8_t rat = models::settingsDefault(NetworkAdapter::CATM1).catm1.rat;
    size_t payload_length = sizeof(payload) / sizeof(uint8_t);
    CBORMessageDecoder decoder;
    MessageDecoder::Status err =  decoder.decode((Message*)&command, payload, payload_length);

    THEN("The decode is successful") {
      REQUIRE(err == MessageDecoder::Status::Complete);
      REQUIRE(command.c.id == ProvisioningMessageId::CATM1ConfigProvisioningMessageId);
      REQUIRE(command.provisioningNetworkConfig.networkSetting.type == NetworkAdapter::CATM1);
      REQUIRE(command.provisioningNetworkConfig.networkSetting.catm1.rat == rat);
      REQUIRE(strcmp(command.provisioningNetworkConfig.networkSetting.catm1.pin, pin) == 0);
      REQUIRE(strcmp(command.provisioningNetworkConfig.networkSetting.catm1.apn, apn) == 0);
      REQUIRE(strcmp(command.provisioningNetworkConfig.networkSetting.catm1.login, username) == 0);
      REQUIRE(strcmp(command.provisioningNetworkConfig.networkSetting.catm1.pass, password) == 0);
      REQUIRE(command.provisioningNetworkConfig.networkSetting.catm1.band == band);
    }
   }
   /****************************************************************************/
   WHEN("Decode a provisioning CATM1 configuration message with more bands than allowed")
   {
    ProvisioningMessageDown command;
     /*
     DA 00012008                           # tag(73736)
       85                                  # array(5)
           68                              # text(8)
             3132333435363738              # "12345678"
           85                              # array(5)
             01                            # unsigned(1)
             02                            # unsigned(2)
             1A 00080000                   # unsigned(524288)
             1A 08000000                   # unsigned(134217728)
             1A 08000000                   # unsigned(134217728)
           6E                              # text(14)
             61706E2E61726475696E6F2E6363  # "apn.arduino.cc"
           68                              # text(8)
             5445535455534552              # "TESTUSER"
           6C                              # text(12)
             5445535450415353574F5244      # "TESTPASSWORD"
     */
     uint8_t const payload[] = {0xda, 0x00, 0x01, 0x20, 0x08, 0x85, 0x68, 0x31,
                                0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x85,
                                0x01, 0x02, 0x1A, 0x00, 0x08, 0x00, 0x00, 0x1A,
                                0x08, 0x00, 0x00, 0x00, 0x1A, 0x08, 0x00, 0x00,
                                0x00, 0x6E, 0x61, 0x70, 0x6E, 0x2E, 0x61, 0x72,
                                0x64, 0x75, 0x69, 0x6E, 0x6F, 0x2E, 0x63, 0x63,
                                0x68, 0x54, 0x45, 0x53, 0x54, 0x55, 0x53, 0x45,
                                0x52, 0x6C, 0x54, 0x45, 0x53, 0x54, 0x50, 0x41,
                                0x53, 0x53, 0x57, 0x4F, 0x52, 0x44};

     size_t payload_length = sizeof(payload) / sizeof(uint8_t);
     CBORMessageDecoder decoder;
     MessageDecoder::Status err =  decoder.decode((Message*)&command, payload, payload_length);

     THEN("The decode is error") {
       REQUIRE(err == MessageDecoder::Status::Error);
     }
   }

   /****************************************************************************/

   WHEN("Decode a provisioning Ethernet complete configuration message with IPv4")
   {
    ProvisioningMessageDown command;
     /*
     DA 00012009       # tag(73737)
       86              # array(6)
           58 04       # bytes(4)
             C0A80002  # "\xC0\xA8\u0000\u0002"
           58 04       # bytes(4)
             08080808  # "\b\b\b\b"
           58 04       # bytes(4)
             C0A80101  # "\xC0\xA8\u0001\u0001"
           58 04       # bytes(4)
             FFFFFF00  # "\xFF\xFF\xFF\u0000"
           0F          # unsigned(15)
           18 C8       # unsigned(200)
     */
     uint8_t const payload[] = {0xda, 0x00, 0x01, 0x20, 0x09, 0x86, 0x58, 0x04,
                                0xC0, 0xA8, 0x00, 0x02, 0x58, 0x04, 0x08, 0x08,
                                0x08, 0x08, 0x58, 0x04, 0xC0, 0xA8, 0x01, 0x01,
                                0x58, 0x04, 0xFF, 0xFF, 0xFF, 0x00, 0x0F, 0x18,
                                0xC8};

     uint8_t ip[] = {192, 168, 0, 2};
     uint8_t dns[] = {8, 8, 8, 8};
     uint8_t gateway[] = {192, 168, 1, 1};
     uint8_t netmask[] = {255, 255, 255, 0};
     uint32_t timeout = 15;
     uint32_t responseTimeout = 200;

     size_t payload_length = sizeof(payload) / sizeof(uint8_t);
     CBORMessageDecoder decoder;
     MessageDecoder::Status err =  decoder.decode((Message*)&command, payload, payload_length);

     THEN("The decode is successful") {
       REQUIRE(err == MessageDecoder::Status::Complete);
       REQUIRE(command.c.id == ProvisioningMessageId::EthernetConfigProvisioningMessageId);
       REQUIRE(command.provisioningNetworkConfig.networkSetting.type == NetworkAdapter::ETHERNET);
       REQUIRE(command.provisioningNetworkConfig.networkSetting.eth.ip.type == IPType::IPv4);
       REQUIRE(memcmp(command.provisioningNetworkConfig.networkSetting.eth.ip.bytes, ip, sizeof(ip)) == 0);
       REQUIRE(command.provisioningNetworkConfig.networkSetting.eth.dns.type == IPType::IPv4);
       REQUIRE(memcmp(command.provisioningNetworkConfig.networkSetting.eth.dns.bytes, dns, sizeof(dns)) == 0);
       REQUIRE(command.provisioningNetworkConfig.networkSetting.eth.gateway.type == IPType::IPv4);
       REQUIRE(memcmp(command.provisioningNetworkConfig.networkSetting.eth.gateway.bytes, gateway, sizeof(gateway)) == 0);
       REQUIRE(command.provisioningNetworkConfig.networkSetting.eth.netmask.type == IPType::IPv4);
       REQUIRE(memcmp(command.provisioningNetworkConfig.networkSetting.eth.netmask.bytes, netmask, sizeof(netmask)) == 0);
       REQUIRE(command.provisioningNetworkConfig.networkSetting.eth.timeout == timeout);
       REQUIRE(command.provisioningNetworkConfig.networkSetting.eth.response_timeout == responseTimeout);
     }
   }

   /****************************************************************************/

   WHEN("Decode a provisioning Ethernet complete configuration message with IPv6")
   {
    ProvisioningMessageDown command;
     /*
     DA 00012009                              # tag(73737)
       86                                     # array(6)
           58 10                              # bytes(16)
             1A4FA7A9928F7B1CEC3B1ECD88580D1E # "\u001AO\xA7\xA9\x92\x8F{\u001C\xEC;\u001E͈X\r\u001E"
           58 10                              # bytes(16)
             21F63B22996F5B7225D9E024F036B5D2 # "!\xF6;\"\x99o[r%\xD9\xE0$\xF06\xB5\xD2"
           58 10                              # bytes(16)
             2EC227F1F19A0C11471B84AF9610B017 # ".\xC2'\xF1\xF1\x9A\f\u0011G\e\x84\xAF\x96\u0010\xB0\u0017"
           58 10                              # bytes(16)
             FFFFFFFFFFFFFFFF0000000000000000 # "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000"
           0F                                 # unsigned(15)
           18 C8                              # unsigned(200)
     */
     uint8_t const payload[] = {0xda, 0x00, 0x01, 0x20, 0x09, 0x86, 0x58, 0x10,
                                0x1A, 0x4F, 0xA7, 0xA9, 0x92, 0x8F, 0x7B, 0x1C,
                                0xEC, 0x3B, 0x1E, 0xCD, 0x88, 0x58, 0x0D, 0x1E,
                                0x58, 0x10, 0x21, 0xF6, 0x3B, 0x22, 0x99, 0x6F,
                                0x5B, 0x72, 0x25, 0xD9, 0xE0, 0x24, 0xF0, 0x36,
                                0xB5, 0xD2, 0x58, 0x10, 0x2E, 0xC2, 0x27, 0xF1,
                                0xF1, 0x9A, 0x0C, 0x11, 0x47, 0x1B, 0x84, 0xAF,
                                0x96, 0x10, 0xB0, 0x17, 0x58, 0x10, 0xFF, 0xFF,
                                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x18,
                                0xC8};


     uint8_t ip[] = {0x1a, 0x4f, 0xa7, 0xa9, 0x92, 0x8f, 0x7b, 0x1c, 0xec, 0x3b, 0x1e, 0xcd, 0x88, 0x58, 0x0d, 0x1e};
     uint8_t dns[] = {0x21, 0xf6, 0x3b, 0x22, 0x99, 0x6f, 0x5b, 0x72, 0x25, 0xd9, 0xe0, 0x24, 0xf0, 0x36, 0xb5, 0xd2};
     uint8_t gateway[] = {0x2e, 0xc2, 0x27, 0xf1, 0xf1, 0x9a, 0x0c, 0x11, 0x47, 0x1b, 0x84, 0xaf, 0x96, 0x10, 0xb0, 0x17};
     uint8_t netmask[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
     uint32_t timeout = 15;
     uint32_t responseTimeout = 200;

     size_t payload_length = sizeof(payload) / sizeof(uint8_t);
     CBORMessageDecoder decoder;
     MessageDecoder::Status err =  decoder.decode((Message*)&command, payload, payload_length);

     THEN("The decode is successful") {
      REQUIRE(err == MessageDecoder::Status::Complete);
      REQUIRE(command.c.id == ProvisioningMessageId::EthernetConfigProvisioningMessageId);
      REQUIRE(command.provisioningNetworkConfig.networkSetting.type == NetworkAdapter::ETHERNET);
      REQUIRE(command.provisioningNetworkConfig.networkSetting.eth.ip.type == IPType::IPv6);
      REQUIRE(memcmp(command.provisioningNetworkConfig.networkSetting.eth.ip.bytes, ip, sizeof(ip)) == 0);
      REQUIRE(command.provisioningNetworkConfig.networkSetting.eth.dns.type == IPType::IPv6);
      REQUIRE(memcmp(command.provisioningNetworkConfig.networkSetting.eth.dns.bytes, dns, sizeof(dns)) == 0);
      REQUIRE(command.provisioningNetworkConfig.networkSetting.eth.gateway.type == IPType::IPv6);
      REQUIRE(memcmp(command.provisioningNetworkConfig.networkSetting.eth.gateway.bytes, gateway, sizeof(gateway)) == 0);
      REQUIRE(command.provisioningNetworkConfig.networkSetting.eth.netmask.type == IPType::IPv6);
      REQUIRE(memcmp(command.provisioningNetworkConfig.networkSetting.eth.netmask.bytes, netmask, sizeof(netmask)) == 0);
      REQUIRE(command.provisioningNetworkConfig.networkSetting.eth.timeout == timeout);
      REQUIRE(command.provisioningNetworkConfig.networkSetting.eth.response_timeout == responseTimeout);
     }
   }

/****************************************************************************/

WHEN("Decode a provisioning Ethernet with defaults and DHCP")
{
 ProvisioningMessageDown command;
  /*
    DA 00012009 # tag(73737)
       86       # array(6)
          40    # bytes(0)
                # ""
          40    # bytes(0)
                # ""
          40    # bytes(0)
                # ""
          40    # bytes(0)
                # ""
          20    # negative(0)
          20    # negative(0)
  */
  uint8_t const payload[] = {0xda, 0x00, 0x01, 0x20, 0x09, 0x86, 0x40, 0x40, 0x40, 0x40, 0x20, 0x20};

  uint8_t ip[] = {0, 0, 0, 0};
  uint8_t dns[] = {0, 0, 0, 0};
  uint8_t gateway[] = {0, 0, 0, 0};
  uint8_t netmask[] = {0, 0, 0, 0};
  uint32_t timeout = models::settingsDefault(NetworkAdapter::ETHERNET).eth.timeout;
  uint32_t responseTimeout = models::settingsDefault(NetworkAdapter::ETHERNET).eth.response_timeout;

  size_t payload_length = sizeof(payload) / sizeof(uint8_t);
  CBORMessageDecoder decoder;
  MessageDecoder::Status err =  decoder.decode((Message*)&command, payload, payload_length);

  THEN("The decode is successful") {
    REQUIRE(err == MessageDecoder::Status::Complete);
    REQUIRE(command.c.id == ProvisioningMessageId::EthernetConfigProvisioningMessageId);
    REQUIRE(command.provisioningNetworkConfig.networkSetting.type == NetworkAdapter::ETHERNET);
    REQUIRE(command.provisioningNetworkConfig.networkSetting.eth.ip.type == IPType::IPv4);
    REQUIRE(memcmp(command.provisioningNetworkConfig.networkSetting.eth.ip.bytes, ip, sizeof(ip)) == 0);
    REQUIRE(command.provisioningNetworkConfig.networkSetting.eth.dns.type == IPType::IPv4);
    REQUIRE(memcmp(command.provisioningNetworkConfig.networkSetting.eth.dns.bytes, dns, sizeof(dns)) == 0);
    REQUIRE(command.provisioningNetworkConfig.networkSetting.eth.gateway.type == IPType::IPv4);
    REQUIRE(memcmp(command.provisioningNetworkConfig.networkSetting.eth.gateway.bytes, gateway, sizeof(gateway)) == 0);
    REQUIRE(command.provisioningNetworkConfig.networkSetting.eth.netmask.type == IPType::IPv4);
    REQUIRE(memcmp(command.provisioningNetworkConfig.networkSetting.eth.netmask.bytes, netmask, sizeof(netmask)) == 0);
    REQUIRE(command.provisioningNetworkConfig.networkSetting.eth.timeout == timeout);
    REQUIRE(command.provisioningNetworkConfig.networkSetting.eth.response_timeout == responseTimeout);
  }
}

   /****************************************************************************/

    WHEN("Decode a provisioning Cellular complete configuration message")
   {
    ProvisioningMessageDown command;
     /*
       DA 00012012                        # tag(73746)
         84                               # array(4)
           68                             # text(8)
             3132333435363738             # "12345678"
           6E                             # text(14)
             61706E2E61726475696E6F2E6363 # "apn.arduino.cc"
           68                             # text(8)
             5445535455534552             # "TESTUSER"
           6C                             # text(12)
             5445535450415353574F5244     # "TESTPASSWORD"
     */
     uint8_t const payload[] = {0xda, 0x00, 0x01, 0x20, 0x12, 0x84, 0x68, 0x31,
                                0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x6e,
                                0x61, 0x70, 0x6e, 0x2e, 0x61, 0x72, 0x64, 0x75,
                                0x69, 0x6e, 0x6f, 0x2e, 0x63, 0x63, 0x68, 0x54,
                                0x45, 0x53, 0x54, 0x55, 0x53, 0x45, 0x52, 0x6c,
                                0x54, 0x45, 0x53, 0x54, 0x50, 0x41, 0x53, 0x53,
                                0x57, 0x4f, 0x52, 0x44};
     char pin[] = "12345678";
     char apn[] = "apn.arduino.cc";
     char username[] = "TESTUSER";
     char password[] = "TESTPASSWORD";

     size_t payload_length = sizeof(payload) / sizeof(uint8_t);
     CBORMessageDecoder decoder;
     MessageDecoder::Status err =  decoder.decode((Message*)&command, payload, payload_length);

     THEN("The decode is successful") {
      REQUIRE(err == MessageDecoder::Status::Complete);
      REQUIRE(command.c.id == ProvisioningMessageId::CellularConfigProvisioningMessageId);
      REQUIRE(command.provisioningNetworkConfig.networkSetting.type == NetworkAdapter::CELL);
      REQUIRE(strcmp(command.provisioningNetworkConfig.networkSetting.cell.pin, pin) == 0);
      REQUIRE(strcmp(command.provisioningNetworkConfig.networkSetting.cell.apn, apn) == 0);
      REQUIRE(strcmp(command.provisioningNetworkConfig.networkSetting.cell.login, username) == 0);
      REQUIRE(strcmp(command.provisioningNetworkConfig.networkSetting.cell.pass, password) == 0);
     }
   }

 }
