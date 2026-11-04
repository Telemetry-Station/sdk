# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.16)
project(telemetry_station_sdk C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(TS_ENABLE_GNSS "Build optional bounded UBX NAV-PVT receiver" OFF)
option(TS_ENABLE_MT "Support receiving messages" ON)
option(TS_QUERY_IMEI_ON_READY "Request the IMEI as soon as the modem is ready" ON)
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
    src/ts_names.c src/ts_gnss.c)
target_include_directories(telemetry_station_sdk PUBLIC include PRIVATE src)

if(TS_ENABLE_MT)
    set(TS_MT_VALUE 1)
else()
    set(TS_MT_VALUE 0)
endif()
target_compile_definitions(telemetry_station_sdk PUBLIC
    TS_ENABLE_GNSS=$<BOOL:${TS_ENABLE_GNSS}>
    TS_ENABLE_MT=${TS_MT_VALUE}
    TS_QUERY_IMEI_ON_READY=$<BOOL:${TS_QUERY_IMEI_ON_READY}>
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

    # Drives every branch of src/ (see tests/branch_waivers.json for the rest).
    add_executable(test_branches tests/test_branches.c)
    target_link_libraries(test_branches ts_sim_modem telemetry_station_sdk)
    target_include_directories(test_branches PRIVATE src)
    if(NOT MSVC)
        target_compile_options(test_branches PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wstrict-prototypes
            -Wmissing-prototypes -Wcast-qual -Wundef)
        if(TS_WERROR)
            target_compile_options(test_branches PRIVATE -Werror)
        endif()
    endif()
    add_test(NAME branches COMMAND test_branches)

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

add_library(ts_example_payload STATIC examples/common/app_payload.c examples/common/generated/telemetry_payload.c)
target_include_directories(ts_example_payload PUBLIC examples/common examples/common/generated include)
target_link_libraries(ts_example_payload PUBLIC telemetry_station_sdk)
if(NOT CMAKE_CROSSCOMPILING)
    add_executable(test_payload examples/common/generated/test_payload.c)
    target_link_libraries(test_payload ts_example_payload)
    add_test(NAME optimizer_golden COMMAND test_payload)
    add_executable(test_gnss tests/test_gnss.c)
    target_link_libraries(test_gnss telemetry_station_sdk)
    add_test(NAME gnss COMMAND test_gnss)
endif()

# Bench checkout: drives one modem cycle and prints a PASS/CHECK report.
add_library(ts_bear_checkout STATIC examples/common/bear_checkout.c)
target_include_directories(ts_bear_checkout PUBLIC examples/common include)
target_link_libraries(ts_bear_checkout PUBLIC telemetry_station_sdk)
if(NOT MSVC)
    target_compile_options(ts_bear_checkout PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wstrict-prototypes
        -Wmissing-prototypes -Wcast-qual -Wundef)
    if(TS_WERROR)
        target_compile_options(ts_bear_checkout PRIVATE -Werror)
    endif()
endif()
if(NOT CMAKE_CROSSCOMPILING)
    add_executable(test_checkout tests/test_checkout.c)
    target_link_libraries(test_checkout ts_bear_checkout ts_sim_modem)
    target_include_directories(test_checkout PRIVATE src)
    add_test(NAME checkout COMMAND test_checkout)

    # Runs the MSP430F169 port, console and example firmware on the host
    # against the register stub and a board model, calling the interrupt
    # handlers directly. Once without a topic (activation) and once with one.
    # <topic>_<console>: first flash with/without a topic, and a headless build.
    foreach(variant 0_1 1234_1 1234_0)
        string(REPLACE "_" ";" parts ${variant})
        list(GET parts 0 topic)
        list(GET parts 1 console)
        set(name test_f169_firmware_${topic})
        if(NOT console)
            set(name test_f169_firmware_headless)
        endif()
        add_executable(${name} tests/test_f169_port.c
            ports/msp430f169/ts_f169_port.c ports/msp430f169/ts_f169_console.c)
        target_include_directories(${name} PRIVATE tests/stubs ports/msp430f169)
        target_compile_definitions(${name} PRIVATE
            TS_ENABLE_CONSOLE=${console} TS_TOPIC_ID=${topic}u)
        target_link_libraries(${name} ts_bear_checkout ts_example_payload ts_sim_modem)
        string(REPLACE "test_" "" test_name ${name})
        add_test(NAME ${test_name} COMMAND ${name})
        list(APPEND TS_F169_TESTS ${name})
    endforeach()
endif()

if(NOT CMAKE_CROSSCOMPILING)
    add_executable(test_lifecycle tests/test_lifecycle.c)
    target_link_libraries(test_lifecycle ts_example_payload ts_sim_modem)
    foreach(case delayed_boot incomplete_reply interrupted_command unknown_delivery shutdown_order power_cycles payload_topic decoder_vector invalid_supply unexpected_reset restart_booted_high boot_timeout_sequence)
        add_test(NAME lifecycle_${case} COMMAND test_lifecycle ${case})
    endforeach()
endif()

if(NOT CMAKE_CROSSCOMPILING)
    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(Python3_Interpreter_FOUND)
        add_test(NAME configuration_and_release_records COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_tools.py)
    endif()
    find_program(TS_NODE node)
    if(TS_NODE)
        add_test(NAME ccs_snapshot_mock COMMAND ${TS_NODE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_snapshot.js)
    endif()
endif()
# Assertions are the test oracle even when callers choose a Release build.
foreach(test_target test_sdk test_faults test_branches test_payload test_gnss test_lifecycle test_checkout ${TS_F169_TESTS})
    if(TARGET ${test_target})
        if(MSVC)
            target_compile_options(${test_target} PRIVATE /UNDEBUG)
        else()
            target_compile_options(${test_target} PRIVATE -UNDEBUG)
        endif()
    endif()
endforeach()
