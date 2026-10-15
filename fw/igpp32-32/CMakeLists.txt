cmake_minimum_required(VERSION 3.16)

# Toolchain: fw/cmake/msp430-gcc.cmake (pass with -DCMAKE_TOOLCHAIN_FILE=...).
project(igpp32-32 C)

set(IGPP_MCU "msp430f5529" CACHE STRING
    "Target MCU: msp430f5529 (LaunchPad) or msp430f5510 (native board)")
option(IGPP_GAME_OF_LIFE "Build with Game of Life demo (GAME_OF_LIFE)" OFF)

set(RNG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../msp430-rng)
if(NOT EXISTS ${RNG_DIR}/rand.h)
    message(FATAL_ERROR
        "fw/msp430-rng submodule is missing. Run:\n"
        "  git submodule update --init fw/msp430-rng")
endif()

file(GLOB DRIVERLIB_SOURCES CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/driverlib/MSP430F5xx_6xx/*.c)

add_executable(igpp32-32
    main.c
    Igpp.c
    uart.c
    hal.c
    game_of_life.c
    rng.c
    ${DRIVERLIB_SOURCES}
)

set_target_properties(igpp32-32 PROPERTIES SUFFIX ".elf")

# Vendor TI driverlib: keep its warnings out of our build output.
set_source_files_properties(${DRIVERLIB_SOURCES} PROPERTIES COMPILE_OPTIONS "-w")

target_include_directories(igpp32-32 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/driverlib/MSP430F5xx_6xx
    ${RNG_DIR}
)

target_compile_definitions(igpp32-32 PRIVATE
    $<$<BOOL:${IGPP_GAME_OF_LIFE}>:GAME_OF_LIFE>
)

target_compile_options(igpp32-32 PRIVATE
    -mmcu=${IGPP_MCU}
    -std=gnu99
    -Os
    -g
    -Wall
    -ffunction-sections
    -fdata-sections
)

target_link_options(igpp32-32 PRIVATE
    -mmcu=${IGPP_MCU}
    -Wl,--gc-sections
    -Wl,-Map=$<TARGET_FILE_DIR:igpp32-32>/igpp32-32.map
)

add_custom_command(TARGET igpp32-32 POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:igpp32-32>
            $<TARGET_FILE_DIR:igpp32-32>/igpp32-32.hex
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:igpp32-32>
    COMMENT "Generating igpp32-32.hex"
    VERBATIM
)
