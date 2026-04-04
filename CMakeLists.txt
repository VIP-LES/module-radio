# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pimoroni_pico_plus2_rp2350 CACHE STRING "Board type")

# add CMake directory in path
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

# Pull in Raspberry Pi Pico SDK
include(pico_sdk_import)

# ====================================================================================

project(template-pico-module C CXX ASM)

option(LEOS_FAKE_TELEMETRY_MODE "Transmit fake sensor/gps telemetry over SX1262 without Cyphal/CAN" OFF)
option(LEOS_ENABLE_SX1262 "Enable the SX1262 radio" ON)
option(LEOS_ENABLE_SX1268 "Enable the SX1268 radio" ON)

if(DEFINED LEOS_TETHERED_TEST_MODE)
    set(LEOS_FAKE_TELEMETRY_MODE ${LEOS_TETHERED_TEST_MODE} CACHE BOOL "Backward-compatible alias for LEOS_FAKE_TELEMETRY_MODE" FORCE)
endif()

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

set(LEOS_SX126X_PORT pico CACHE STRING "Platform port for leos_sx126x" FORCE)
include(leos-sdk)


# Add executable. Default name is the project name, version 0.1
file(GLOB SRC_FILES
    src/*.c
    src/*.cpp
)

add_executable(RADIO_MODULE_PICO ${SRC_FILES})
pico_set_program_name(RADIO_MODULE_PICO "CHANGE THIS")
pico_set_program_version(RADIO_MODULE_PICO "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(RADIO_MODULE_PICO 0)
pico_enable_stdio_usb(RADIO_MODULE_PICO 1)

# Generate project-specific Cyphal DSDL types from the checked-out LEOS
# namespace so cyphal_bridge can use real Nunavut structs.
cyphal_generate_types(
    TARGET RADIO_MODULE_PICO
    DSDL_DIR ${PROJECT_SOURCE_DIR}/external/leos-cyphal-types/leos
    DSDL_LOOKUP_DIRS ${PROJECT_SOURCE_DIR}/external/leos-sdk/external/public_regulated_data_types/uavcan
)

# Add required template libraries
target_link_libraries(RADIO_MODULE_PICO
        pico_stdlib
        leos_log
        leos_mcp251xfd
        leos_cyphal_transport_mcp251xfd
        leos_cyphal_node
        leos_sx126x
)

target_compile_definitions(RADIO_MODULE_PICO PRIVATE
    LEOS_FAKE_TELEMETRY_MODE=$<BOOL:${LEOS_FAKE_TELEMETRY_MODE}>
    LEOS_TETHERED_TEST_MODE=$<BOOL:${LEOS_FAKE_TELEMETRY_MODE}>
    LEOS_ENABLE_SX1262=$<BOOL:${LEOS_ENABLE_SX1262}>
    LEOS_ENABLE_SX1268=$<BOOL:${LEOS_ENABLE_SX1268}>
)

# Add the standard include files to the build
target_include_directories(RADIO_MODULE_PICO PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)

pico_add_extra_outputs(RADIO_MODULE_PICO)
