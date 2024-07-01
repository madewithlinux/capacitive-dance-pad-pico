# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NEVER EDIT THE NEXT LINES for Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
   set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(PICO_SDK_PATH ${USERHOME}/.pico-sdk/sdk/1.5.1)
set(PICO_TOOLCHAIN_PATH ${USERHOME}/.pico-sdk/toolchain/13_2_Rel1)
if(WIN32)
    set(pico-sdk-tools_DIR ${USERHOME}/.pico-sdk/tools/1.5.1)
    include(${pico-sdk-tools_DIR}/pico-sdk-tools-config.cmake)
    include(${pico-sdk-tools_DIR}/pico-sdk-tools-config-version.cmake)
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.4.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.4.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

project(capacitive-dance-pad-pico C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(capacitive-dance-pad-pico
  capacitive-dance-pad-pico.cpp
  touch_sensor_thread.cpp
  custom_logging.cpp
  multicore_ipc.cpp
  usb_descriptors.c
  reset_interface.c
  tusb_config.h
  )

pico_set_program_name(capacitive-dance-pad-pico "capacitive-dance-pad-pico")
pico_set_program_version(capacitive-dance-pad-pico "0.1")

# Generate PIO header
pico_generate_pio_header(capacitive-dance-pad-pico ${CMAKE_CURRENT_LIST_DIR}/blink.pio)
pico_generate_pio_header(capacitive-dance-pad-pico ${CMAKE_CURRENT_LIST_DIR}/touch.pio)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(capacitive-dance-pad-pico 0)
pico_enable_stdio_usb(capacitive-dance-pad-pico 1)

# TODO: this doesn't work
# target_compile_definitions(capacitive-dance-pad-pico INTERFACE
#   PICO_STDIO_USB_ENABLE_RESET_VIA_BAUD_RATE=1
#   PICO_STDIO_USB_RESET_BOOTSEL_ACTIVITY_LED=25
#   )

# Make sure TinyUSB can find tusb_config.h
target_include_directories(capacitive-dance-pad-pico PUBLIC
        ${CMAKE_CURRENT_LIST_DIR})

# Add the standard library to the build
# In addition to pico_stdlib required for common PicoSDK functionality, add dependency on tinyusb_device
# for TinyUSB device support and tinyusb_board for the additional board support library used by the example
target_link_libraries(capacitive-dance-pad-pico
        pico_stdlib
        pico_multicore
        pico_unique_id
        tinyusb_device
        tinyusb_board)

# Add the standard include files to the build
target_include_directories(capacitive-dance-pad-pico PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts or any other standard includes, if required
)

# Add any user requested libraries
target_link_libraries(capacitive-dance-pad-pico
        hardware_pio
        hardware_timer
        )

pico_add_extra_outputs(capacitive-dance-pad-pico)

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        )
if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wno-maybe-uninitialized)
endif()


# turns out fmt doesn't work well, seems to crash the microcontroller (maybe due to running out of memory?)
# ############ fmt
# include(FetchContent)

# FetchContent_Declare(
#   fmt
#   GIT_REPOSITORY https://github.com/fmtlib/fmt
#   GIT_TAG        e69e5f977d458f2650bb346dadf2ad30c5320281) # 10.2.1
# FetchContent_MakeAvailable(fmt)

# # target_link_libraries(capacitive-dance-pad-pico fmt::fmt)
# target_link_libraries(capacitive-dance-pad-pico fmt::fmt-header-only)
