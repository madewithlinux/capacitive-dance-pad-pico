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
add_library(common_stuff INTERFACE)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

include_directories(src/)
target_sources(common_stuff INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/src/main.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/config_defines.h
    ${CMAKE_CURRENT_LIST_DIR}/src/config_values.hpp
    ${CMAKE_CURRENT_LIST_DIR}/src/custom_logging.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/custom_logging.hpp
    ${CMAKE_CURRENT_LIST_DIR}/src/multicore_ipc.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/multicore_ipc.h
    ${CMAKE_CURRENT_LIST_DIR}/src/reset_interface.c
    ${CMAKE_CURRENT_LIST_DIR}/src/running_stats.hpp
    ${CMAKE_CURRENT_LIST_DIR}/src/serial_config_console.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/serial_config_console.hpp
    ${CMAKE_CURRENT_LIST_DIR}/src/teleplot_task.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/teleplot_task.hpp
    ${CMAKE_CURRENT_LIST_DIR}/src/touch_hid_tasks.hpp
    ${CMAKE_CURRENT_LIST_DIR}/src/touch_hid_tasks.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/touch_sensor_config.hpp
    ${CMAKE_CURRENT_LIST_DIR}/src/touch_sensor_thread.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/touch_sensor_thread.hpp
    ${CMAKE_CURRENT_LIST_DIR}/src/tusb_config.h
    ${CMAKE_CURRENT_LIST_DIR}/src/usb_descriptors.c
    ${CMAKE_CURRENT_LIST_DIR}/src/usb_descriptors.h
)

# Generate PIO header
pico_generate_pio_header(common_stuff ${CMAKE_CURRENT_LIST_DIR}/touch.pio)

# Add the standard library to the build
# In addition to pico_stdlib required for common PicoSDK functionality, add dependency on tinyusb_device
# for TinyUSB device support and tinyusb_board for the additional board support library used by the example
target_link_libraries(common_stuff INTERFACE
    pico_stdlib
    pico_multicore
    pico_unique_id
    pico_stdio_usb
    hardware_pio
    hardware_timer
    tinyusb_device
    tinyusb_board)

# Add the standard include files to the build, and Make sure TinyUSB can find tusb_config.h
target_include_directories(common_stuff INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts or any other standard includes, if required
  ${CMAKE_CURRENT_LIST_DIR}/src
)

add_compile_options(-Wall
    -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
    -Wno-unused-function # we have some for the docs that aren't called
)
if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wno-maybe-uninitialized)
endif()

#########################################
### individual targets for different configurations

add_executable(sockpad_itg8)
target_link_libraries(sockpad_itg8 common_stuff)
target_compile_definitions(sockpad_itg8 PRIVATE
    TOUCH_SENSOR_CONFIG=TOUCH_SENSOR_CONFIG_ITG8
)
pico_add_extra_outputs(sockpad_itg8)
pico_set_binary_type(sockpad_itg8 copy_to_ram)

add_executable(sockpad_itg8_parallel)
target_link_libraries(sockpad_itg8_parallel common_stuff)
target_compile_definitions(sockpad_itg8_parallel PRIVATE
    TOUCH_SENSOR_CONFIG=TOUCH_SENSOR_CONFIG_ITG8
    TOUCH_POLLING_TYPE=TOUCH_POLLING_PARALLEL
)
pico_add_extra_outputs(sockpad_itg8_parallel)
pico_set_binary_type(sockpad_itg8_parallel copy_to_ram)


add_executable(sockpad_pump)
target_link_libraries(sockpad_pump common_stuff)
target_compile_definitions(sockpad_pump PRIVATE
    TOUCH_SENSOR_CONFIG=TOUCH_SENSOR_CONFIG_PUMP
)
pico_add_extra_outputs(sockpad_pump)
pico_set_binary_type(sockpad_pump copy_to_ram)

add_executable(sockpad_pump_p1)
target_link_libraries(sockpad_pump_p1 common_stuff)
target_compile_definitions(sockpad_pump_p1 PRIVATE
    TOUCH_SENSOR_CONFIG=TOUCH_SENSOR_CONFIG_PUMP
    PLAYER_NUMBER=1
)
pico_add_extra_outputs(sockpad_pump_p1)
pico_set_binary_type(sockpad_pump_p1 copy_to_ram)

add_executable(sockpad_pump_p2)
target_link_libraries(sockpad_pump_p2 common_stuff)
target_compile_definitions(sockpad_pump_p2 PRIVATE
    TOUCH_SENSOR_CONFIG=TOUCH_SENSOR_CONFIG_PUMP
    PLAYER_NUMBER=2
)
pico_add_extra_outputs(sockpad_pump_p2)
pico_set_binary_type(sockpad_pump_p2 copy_to_ram)








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
