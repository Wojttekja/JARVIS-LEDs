cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico)  # first-gen Pico (RP2040, no WiFi)

# Use the VS Code extension's picotool (built with USB support, so `picotool load -f` works)
# instead of letting the SDK fetch and build one without USB.
set(PICOTOOL_EXT_DIR $ENV{HOME}/.pico-sdk/picotool/2.3.1/picotool)
if(EXISTS ${PICOTOOL_EXT_DIR}/picotoolConfig.cmake)
    set(picotool_DIR ${PICOTOOL_EXT_DIR})
endif()

include(pico_sdk_import.cmake)

project(pico_leds C CXX ASM)
set(CMAKE_C_STANDARD 11)

pico_sdk_init()

add_subdirectory(common)

# Step-by-step test programs
add_subdirectory(steps/01_blink)
add_subdirectory(steps/02_serial)
add_subdirectory(steps/03_nrf_probe)
add_subdirectory(steps/04_ping_pong)
add_subdirectory(steps/05_inputs)
add_subdirectory(steps/07_strip_test)

# Final firmware
add_subdirectory(remote)
add_subdirectory(receiver)
