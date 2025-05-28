##########################################################################

cmake_minimum_required(VERSION 2.8)

##########################################################################

project(testNetworkConfigurator)

Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.4.0
)

FetchContent_Declare(
  cloudutils
  GIT_REPOSITORY      https://github.com/arduino-libraries/Arduino_CloudUtils.git
  GIT_TAG             main
  CONFIGURE_COMMAND   ""
  BUILD_COMMAND       ""
)

FetchContent_Declare(
  connectionhandler
  GIT_REPOSITORY      https://github.com/arduino-libraries/Arduino_ConnectionHandler.git
  GIT_TAG             master
  CONFIGURE_COMMAND   ""
  BUILD_COMMAND       ""
)

FetchContent_MakeAvailable(Catch2)

FetchContent_GetProperties(cloudutils)
if(NOT cloudutils_POPULATED)
  FetchContent_Populate(cloudutils)
endif()

FetchContent_GetProperties(connectionhandler)
if(NOT connectionhandler_POPULATED)
  FetchContent_Populate(connectionhandler)
endif()

##########################################################################

include_directories(include)
include_directories(../../src)
include_directories(../../src/configuratorAgents)
include_directories(../../src/configuratorAgents/agents/boardConfigurationProtocol/cbor)

# add_library(cloudutils STATIC IMPORTED GLOBAL)
add_library(cloudutils INTERFACE)

target_include_directories(
  cloudutils INTERFACE
  ${cloudutils_SOURCE_DIR}/src/
)

target_include_directories(
  cloudutils INTERFACE
  ${cloudutils_SOURCE_DIR}/src/cbor
)

target_include_directories(
  cloudutils INTERFACE
  ${cloudutils_SOURCE_DIR}/src/interfaces
)

add_library(connectionhandler INTERFACE)

target_include_directories(
    connectionhandler INTERFACE
  ${connectionhandler_SOURCE_DIR}/src
)

target_include_directories(
    connectionhandler INTERFACE
  ${connectionhandler_SOURCE_DIR}/src/settings
)


##########################################################################

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

##########################################################################

set(TEST_TARGET ${CMAKE_PROJECT_NAME})

##########################################################################

set(TEST_SRCS
  src/test_provisioning_command_decode.cpp
  src/test_provisioning_command_encode.cpp
)

set(TEST_DUT_SRCS
  ../../src/configuratorAgents/agents/boardConfigurationProtocol/cbor/Decoder.cpp
  ../../src/configuratorAgents/agents/boardConfigurationProtocol/cbor/Encoder.cpp

  ${cloudutils_SOURCE_DIR}/src/cbor/tinycbor/src/cborencoder.c
  ${cloudutils_SOURCE_DIR}/src/cbor/tinycbor/src/cborencoder_close_container_checked.c
  ${cloudutils_SOURCE_DIR}/src/cbor/tinycbor/src/cborerrorstrings.c
  ${cloudutils_SOURCE_DIR}/src/cbor/tinycbor/src/cborparser.c
  ${cloudutils_SOURCE_DIR}/src/cbor/tinycbor/src/cborparser_dup_string.c
  ${cloudutils_SOURCE_DIR}/src/cbor/tinycbor/src/cborpretty.c
  ${cloudutils_SOURCE_DIR}/src/cbor/tinycbor/src/cborpretty_stdio.c
  ${cloudutils_SOURCE_DIR}/src/cbor/tinycbor/src/cbortojson.c
  ${cloudutils_SOURCE_DIR}/src/cbor/tinycbor/src/cborvalidation.c
  ${cloudutils_SOURCE_DIR}/src/cbor/tinycbor/src/open_memstream.c
  ${cloudutils_SOURCE_DIR}/src/cbor/MessageDecoder.cpp
  ${cloudutils_SOURCE_DIR}/src/cbor/MessageEncoder.cpp
)
##########################################################################

set(TEST_TARGET_SRCS
  src/Arduino.cpp
  ${TEST_SRCS}
  ${TEST_UTIL_SRCS}
  ${TEST_DUT_SRCS}
)

##########################################################################

add_compile_definitions(CI_TEST BOARD_HAS_LORA BOARD_HAS_CATM1_NBIOT BOARD_HAS_WIFI BOARD_HAS_ETHERNET BOARD_HAS_CELLULAR BOARD_HAS_NB BOARD_HAS_GSM)
add_compile_options(-Wextra -Werror)
add_compile_options(-Wno-cast-function-type)

set(CMAKE_C_FLAGS   ${CMAKE_C_FLAGS}   "--coverage")
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "--coverage -Wno-deprecated-copy -Wno-missing-field-initializers")

##########################################################################

add_executable(
  ${TEST_TARGET}
  ${TEST_TARGET_SRCS}
)

target_link_libraries( ${TEST_TARGET} connectionhandler)
target_link_libraries( ${TEST_TARGET} cloudutils)
target_link_libraries( ${TEST_TARGET} Catch2WithMain )

##########################################################################

