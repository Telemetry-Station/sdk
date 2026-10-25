# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.16)
project(telemetry_station_sdk C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(TS_ENABLE_MT "Support receiving messages" ON)
set(TS_MAX_PAYLOAD 64 CACHE STRING "Largest payload in bytes")
set(TS_LINE_CAP 256 CACHE STRING "Modem line buffer size")
option(TS_WERROR "Treat compiler warnings as errors" OFF)
option(TS_SANITIZE "Build host tests with AddressSanitizer and UBSan" OFF)
option(TS_COVERAGE "Instrument the driver for coverage (see tests/coverage.sh)" OFF)

# Directory-wide flags must come before any target is created.
if(TS_SANITIZE AND NOT MSVC)
    add_compile_options(-fsanitize=address,undefined -fno-sanitize-recover=all)
    add_link_options(-fsanitize=address,undefined)
endif()
if(TS_COVERAGE AND NOT MSVC)
    add_compile_options(--coverage -O0 -g)
    add_link_options(--coverage)
endif()

add_library(telemetry_station_sdk STATIC
    src/ts_bear9704.c src/ts_cmd.c src/ts_setup.c src/ts_mo.c src/ts_mt.c
    src/ts_jspr.c src/ts_crc.c src/ts_base64.c src/ts_activation.c
    src/ts_names.c)
target_include_directories(telemetry_station_sdk PUBLIC include PRIVATE src)

if(TS_ENABLE_MT)
    set(TS_MT_VALUE 1)
else()
    set(TS_MT_VALUE 0)
endif()
target_compile_definitions(telemetry_station_sdk PUBLIC
    TS_ENABLE_MT=${TS_MT_VALUE}
    TS_MAX_PAYLOAD=${TS_MAX_PAYLOAD}
    TS_LINE_CAP=${TS_LINE_CAP})

if(NOT MSVC)
    target_compile_options(telemetry_station_sdk PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wstrict-prototypes
        -Wmissing-prototypes -Wcast-qual -Wundef)
    if(TS_WERROR)
        target_compile_options(telemetry_station_sdk PRIVATE -Werror)
    endif()
endif()

# The platform template must always compile, so it cannot drift from the API.
add_library(ts_port_template OBJECT ports/template/ts_port_template.c)
target_include_directories(ts_port_template PUBLIC include)

# The MSP430 port and example are compiled against a host stub of <msp430.h>.
# This catches syntax, type and warning problems without the TI toolchain. It
# does not run or link the code: build for the real target with your toolchain.
add_library(ts_msp430f169_syntax_check OBJECT
    ports/msp430f169/ts_f169_port.c
    ports/msp430f169/ts_f169_console.c
    examples/msp430f169/main.c)
target_include_directories(ts_msp430f169_syntax_check PRIVATE
    tests/stubs include ports/msp430f169 examples/common)
if(NOT MSVC)
    # Interrupt handlers must be externally visible for the vector table.
    target_compile_options(ts_msp430f169_syntax_check PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wstrict-prototypes
        -Wcast-qual -Wundef)
    if(TS_WERROR)
        target_compile_options(ts_msp430f169_syntax_check PRIVATE -Werror)
    endif()
endif()

# Host-only builds: the unit tests and a bench program for a real modem.
if(NOT CMAKE_CROSSCOMPILING)
    enable_testing()
    add_library(ts_sim_modem STATIC tests/sim_modem.c)
    target_link_libraries(ts_sim_modem telemetry_station_sdk)
    target_include_directories(ts_sim_modem PUBLIC tests)

    add_executable(test_sdk tests/test_sdk.c)
    target_link_libraries(test_sdk ts_sim_modem telemetry_station_sdk)
    target_include_directories(test_sdk PRIVATE src)
    add_test(NAME sdk COMMAND test_sdk)

    add_executable(test_faults tests/test_faults.c)
    target_link_libraries(test_faults ts_sim_modem telemetry_station_sdk)
    target_include_directories(test_faults PRIVATE src)
    add_test(NAME faults COMMAND test_faults)

    # No-surprises check: the driver core must not use stdio formatting or
    # dynamic memory. (ts_activation.c formats text for a console and is exempt.)
    if(UNIX)
        add_test(NAME no_stdio_no_heap
            COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/tests/check_core_symbols.sh
                    $<TARGET_FILE:telemetry_station_sdk>)
    endif()

    add_executable(simulated_demo examples/host/simulated_demo.c)
    target_link_libraries(simulated_demo ts_sim_modem telemetry_station_sdk)
    add_test(NAME simulated_demo COMMAND simulated_demo)

    if(UNIX)
        add_executable(ts_posix_modem
            examples/host/posix_modem.c ports/posix/ts_posix.c)
        target_include_directories(ts_posix_modem PRIVATE ports/posix)
        target_link_libraries(ts_posix_modem telemetry_station_sdk)
    endif()
endif()
