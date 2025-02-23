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
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Set any variables required for importing libraries
SET(FREERTOS_KERNEL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/components/FreeRTOS-Kernel)

include(pico_sdk_import.cmake)
include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

project(memory_game C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1
add_executable(memory_game
        components/led_matrix/neopixel_pio.c
        #components/FreeRTOS/semaforo.c
        #components/joystick/joystick_timer.c
        #components/joystick/joystick.c
        #components/joystick/joystick_led.c
        #components/oled_ssd1306/test_ssd1306_module.c
        #components/oled_ssd1306/ssd1306_i2c.c
        )

        # Generate PIO header
pico_generate_pio_header(neopixel_pio ${CMAKE_CURRENT_LIST_DIR}/components/led_matrix/ws2818b.pio)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(memory_game 0)
pico_enable_stdio_usb(memory_game 1)

# Add the standard include files to the build
target_include_directories(memory_game PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
)

# pull in common dependencies and additional i2c hardware support
target_link_libraries(memory_game 
                    pico_stdlib hardware_i2c 
                    hardware_adc 
                    hardware_pwm 
                    hardware_timer 
                    FreeRTOS-Kernel-Heap4
                    hardware_pio
                    hardware_clocks)

# create map/bin/hex file etc.
pico_add_extra_outputs(memory_game)
