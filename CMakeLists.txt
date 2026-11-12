# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if(EXISTS ${picoVscode})
    include(${picoVscode})
endif()
set(PICO_BOARD pimoroni_pico_plus2_rp2350 CACHE STRING "Board type")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(pico_sdk_import)

project(pico_sx1262_minimal C CXX ASM)
pico_sdk_init()

set(SX126X_DRIVER_PATH "${CMAKE_CURRENT_LIST_DIR}/../sx126x_driver"
    CACHE PATH "Unmodified Semtech sx126x_driver checkout")
add_subdirectory("${SX126X_DRIVER_PATH}" "${CMAKE_CURRENT_BINARY_DIR}/sx126x_driver")

set(LEOS_PROTOBUF_PATH "${CMAKE_CURRENT_LIST_DIR}/../leos-protobufs"
    CACHE PATH "LEOS protobuf schema and generated nanopb codec checkout")
set(LEOS_PROTOBUF_GENERATED "${LEOS_PROTOBUF_PATH}/generated/c")
set(NANOPB_PATH "${LEOS_PROTOBUF_PATH}/vendor/nanopb")

add_library(leos_protobuf_codec STATIC
    ${NANOPB_PATH}/pb_common.c
    ${NANOPB_PATH}/pb_decode.c
    ${NANOPB_PATH}/pb_encode.c
    ${LEOS_PROTOBUF_GENERATED}/beacon.pb.c
    ${LEOS_PROTOBUF_GENERATED}/command.pb.c
    ${LEOS_PROTOBUF_GENERATED}/efm.pb.c
    ${LEOS_PROTOBUF_GENERATED}/geiger.pb.c
    ${LEOS_PROTOBUF_GENERATED}/gps.pb.c
    ${LEOS_PROTOBUF_GENERATED}/packet.pb.c
    ${LEOS_PROTOBUF_GENERATED}/purpleboard.pb.c
    ${LEOS_PROTOBUF_GENERATED}/telemetry.pb.c)
target_include_directories(leos_protobuf_codec PUBLIC
    ${NANOPB_PATH}
    ${LEOS_PROTOBUF_GENERATED})

add_executable(pico_sx1262_tx_test tx_test.c sx126x_hal_pico.c)
pico_set_program_name(pico_sx1262_tx_test "pico_sx1262_tx_test")
pico_set_program_version(pico_sx1262_tx_test "0.1")
target_link_libraries(pico_sx1262_tx_test PRIVATE
    pico_stdlib hardware_spi sx126x_driver::sx126x_driver)
pico_enable_stdio_uart(pico_sx1262_tx_test 0)
pico_enable_stdio_usb(pico_sx1262_tx_test 1)
pico_add_extra_outputs(pico_sx1262_tx_test)

add_executable(pico_sx1262_tx_protobuf_test
    tx_protobuf_test.c sx126x_hal_pico.c)
pico_set_program_name(pico_sx1262_tx_protobuf_test "pico_sx1262_tx_protobuf_test")
pico_set_program_version(pico_sx1262_tx_protobuf_test "0.1")
target_link_libraries(pico_sx1262_tx_protobuf_test PRIVATE
    pico_stdlib hardware_spi sx126x_driver::sx126x_driver leos_protobuf_codec)
pico_enable_stdio_uart(pico_sx1262_tx_protobuf_test 0)
pico_enable_stdio_usb(pico_sx1262_tx_protobuf_test 1)
pico_add_extra_outputs(pico_sx1262_tx_protobuf_test)

add_executable(pico_sx1262_rx_test rx_test.c sx126x_hal_pico.c)
pico_set_program_name(pico_sx1262_rx_test "pico_sx1262_rx_test")
pico_set_program_version(pico_sx1262_rx_test "0.1")
target_link_libraries(pico_sx1262_rx_test PRIVATE
    pico_stdlib hardware_spi sx126x_driver::sx126x_driver)
pico_enable_stdio_uart(pico_sx1262_rx_test 0)
pico_enable_stdio_usb(pico_sx1262_rx_test 1)
pico_add_extra_outputs(pico_sx1262_rx_test)

# ---------------------------------------------------------------------------
# Cyphal -> LoRa bridge (receives on CAN, forwards leos.Packet over SX126x)
# ---------------------------------------------------------------------------
set(LEOS_SDK_PATH "${CMAKE_CURRENT_LIST_DIR}/../leos-sdk" CACHE PATH "VIP-LES/leos-sdk checkout (with submodules)")
set(LEOS_CYPHAL_TYPES_PATH "${CMAKE_CURRENT_LIST_DIR}/../leos-cyphal-types" CACHE PATH "VIP-LES/leos-cyphal-types checkout")
set(LEOS_SDK_ROOT "${LEOS_SDK_PATH}")
list(APPEND CMAKE_MODULE_PATH "${LEOS_SDK_PATH}/cmake")
add_subdirectory("${LEOS_SDK_PATH}" "${CMAKE_CURRENT_BINARY_DIR}/leos-sdk")

add_executable(pico_sx1262_cyphal_bridge
    src/main.c src/module_setup.c src/cyphal_bridge.c
    src/radio_protocol.c src/radio.c src/radio_command.c src/board_can.c
    sx126x_hal_pico.c)
target_include_directories(pico_sx1262_cyphal_bridge PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(pico_sx1262_cyphal_bridge PRIVATE
    pico_stdlib hardware_spi sx126x_driver::sx126x_driver leos_protobuf_codec
    leos_cyphal_node leos_cyphal_transport_mcp251xfd leos_mcp251xfd leos_log)
cyphal_generate_types(
    TARGET pico_sx1262_cyphal_bridge
    DSDL_DIR ${LEOS_CYPHAL_TYPES_PATH}/leos
    DSDL_LOOKUP_DIRS ${LEOS_SDK_PATH}/external/public_regulated_data_types/uavcan)
pico_enable_stdio_uart(pico_sx1262_cyphal_bridge 0)
pico_enable_stdio_usb(pico_sx1262_cyphal_bridge 1)
pico_add_extra_outputs(pico_sx1262_cyphal_bridge)
