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

add_executable(pico_sx1262_minimal main.c sx126x_hal_pico.c)
pico_set_program_name(pico_sx1262_minimal "pico_sx1262_minimal")
pico_set_program_version(pico_sx1262_minimal "0.1")
target_link_libraries(pico_sx1262_minimal PRIVATE
    pico_stdlib hardware_spi sx126x_driver::sx126x_driver)
pico_enable_stdio_uart(pico_sx1262_minimal 0)
pico_enable_stdio_usb(pico_sx1262_minimal 1)
pico_add_extra_outputs(pico_sx1262_minimal)

add_executable(pico_sx1262_tx_test tx_test.c sx126x_hal_pico.c)
pico_set_program_name(pico_sx1262_tx_test "pico_sx1262_tx_test")
pico_set_program_version(pico_sx1262_tx_test "0.1")
target_link_libraries(pico_sx1262_tx_test PRIVATE
    pico_stdlib hardware_spi sx126x_driver::sx126x_driver)
pico_enable_stdio_uart(pico_sx1262_tx_test 0)
pico_enable_stdio_usb(pico_sx1262_tx_test 1)
pico_add_extra_outputs(pico_sx1262_tx_test)

add_executable(pico_sx1262_rx_test rx_test.c sx126x_hal_pico.c)
pico_set_program_name(pico_sx1262_rx_test "pico_sx1262_rx_test")
pico_set_program_version(pico_sx1262_rx_test "0.1")
target_link_libraries(pico_sx1262_rx_test PRIVATE
    pico_stdlib hardware_spi sx126x_driver::sx126x_driver)
pico_enable_stdio_uart(pico_sx1262_rx_test 0)
pico_enable_stdio_usb(pico_sx1262_rx_test 1)
pico_add_extra_outputs(pico_sx1262_rx_test)
