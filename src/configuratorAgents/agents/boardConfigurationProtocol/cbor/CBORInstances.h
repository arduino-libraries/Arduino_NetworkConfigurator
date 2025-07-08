#pragma once

#include "./Encoder.h"
#include "./Decoder.h"

static StatusProvisioningMessageEncoder             statusProvisioningMessageEncoder;
static ListWifiNetworksProvisioningMessageEncoder   listWifiNetworksProvisioningMessageEncoder;
static UniqueHardwareIdProvisioningMessageEncoder   uniqueHardwareIdProvisioningMessageEncoder;
static JWTProvisioningMessageEncoder                jWTProvisioningMessageEncoder;
static ProvPublicKeyProvisioningMessageEncoder      provPublicKeyProvisioningMessageEncoder;
static BLEMacAddressProvisioningMessageEncoder      bLEMacAddressProvisioningMessageEncoder;
static ProvSketchVersionProvisioningMessageEncoder  provSketchVersionProvisioningMessageEncoder;
static NetConfigLibVersProvisioningMessageEncoder   netConfigLibVersProvisioningMessageEncoder;

static TimestampProvisioningMessageDecoder      timestampProvisioningMessageDecoder;
static CommandsProvisioningMessageDecoder       commandsProvisioningMessageDecoder;
#if defined(BOARD_HAS_WIFI)
static WifiConfigProvisioningMessageDecoder     wifiConfigProvisioningMessageDecoder;
#endif
#if defined(BOARD_HAS_LORA)
static LoRaConfigProvisioningMessageDecoder     loRaConfigProvisioningMessageDecoder;
#endif
#if defined(BOARD_HAS_CATM1_NBIOT)
static CATM1ConfigProvisioningMessageDecoder    cATM1ConfigProvisioningMessageDecoder;
#endif
#if defined(BOARD_HAS_ETHERNET)
static EthernetConfigProvisioningMessageDecoder ethernetConfigProvisioningMessageDecoder;
#endif
#if defined(BOARD_HAS_CELLULAR)
static CellularConfigProvisioningMessageDecoder cellularConfigProvisioningMessageDecoder;
#endif
#if defined(BOARD_HAS_NB)
static NBIOTConfigProvisioningMessageDecoder    nbiotConfigProvisioningMessageDecoder;
#endif
#if defined(BOARD_HAS_GSM)
static GSMConfigProvisioningMessageDecoder      gsmConfigProvisioningMessageDecoder;
#endif
