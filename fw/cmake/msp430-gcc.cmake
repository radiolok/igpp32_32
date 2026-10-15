# CMake toolchain file for TI MSP430-GCC (msp430-elf-gcc).
#
# Usage:
#   cmake -S fw/igpp32-32 -B build \
#         -DCMAKE_TOOLCHAIN_FILE=fw/cmake/msp430-gcc.cmake \
#         -DMSP430_GCC_ROOT=/opt/msp430-gcc \
#         -DMSP430_SUPPORT_DIR=/opt/msp430-gcc-support-files/include
#
# MSP430_GCC_ROOT     - toolchain install dir (contains bin/msp430-elf-gcc).
#                       If empty, the compiler is searched in PATH.
# MSP430_SUPPORT_DIR  - "include" dir of msp430-gcc-support-files
#                       (device headers msp430f5529.h and linker scripts *.ld).
#                       If empty, <MSP430_GCC_ROOT>/include is used (full TI installer layout).
# Both can also be passed through environment variables of the same name.

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR msp430)

if(NOT MSP430_GCC_ROOT AND DEFINED ENV{MSP430_GCC_ROOT})
    set(MSP430_GCC_ROOT "$ENV{MSP430_GCC_ROOT}")
endif()
if(NOT MSP430_SUPPORT_DIR AND DEFINED ENV{MSP430_SUPPORT_DIR})
    set(MSP430_SUPPORT_DIR "$ENV{MSP430_SUPPORT_DIR}")
endif()
if(NOT MSP430_SUPPORT_DIR AND MSP430_GCC_ROOT)
    set(MSP430_SUPPORT_DIR "${MSP430_GCC_ROOT}/include")
endif()

set(MSP430_GCC_ROOT "${MSP430_GCC_ROOT}" CACHE PATH "MSP430-GCC install directory")
set(MSP430_SUPPORT_DIR "${MSP430_SUPPORT_DIR}" CACHE PATH "MSP430-GCC support files include directory")

# Make the variables visible inside try_compile projects.
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES MSP430_GCC_ROOT MSP430_SUPPORT_DIR)

if(MSP430_GCC_ROOT)
    set(_msp430_bin "${MSP430_GCC_ROOT}/bin/")
else()
    set(_msp430_bin "")
endif()

set(CMAKE_C_COMPILER   "${_msp430_bin}msp430-elf-gcc")
set(CMAKE_CXX_COMPILER "${_msp430_bin}msp430-elf-g++")
set(CMAKE_ASM_COMPILER "${_msp430_bin}msp430-elf-gcc")
set(CMAKE_OBJCOPY      "${_msp430_bin}msp430-elf-objcopy" CACHE FILEPATH "objcopy")
set(CMAKE_SIZE         "${_msp430_bin}msp430-elf-size"    CACHE FILEPATH "size")

# Compiler checks must not try to link a full executable without -mmcu.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

if(MSP430_SUPPORT_DIR)
    set(CMAKE_C_FLAGS_INIT "-I${MSP430_SUPPORT_DIR}")
    set(CMAKE_EXE_LINKER_FLAGS_INIT "-L${MSP430_SUPPORT_DIR}")
endif()

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
