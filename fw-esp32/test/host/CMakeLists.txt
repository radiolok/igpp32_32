# Host tests for the hardware-independent core.
#   cmake -S fw-esp32/test/host -B build-host && cmake --build build-host && ctest --test-dir build-host
cmake_minimum_required(VERSION 3.16)
project(igpp_host_tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../components/igpp_core igpp_core)
target_compile_options(igpp_core PRIVATE -Wall -Wextra -Werror)

add_executable(igpp_tests
    main.cpp
    test_frame.cpp
    test_scanner.cpp
    test_protocol.cpp
    test_igv.cpp
    test_patterns.cpp
)
target_link_libraries(igpp_tests PRIVATE igpp_core)
target_compile_options(igpp_tests PRIVATE -Wall -Wextra -Werror -fsanitize=address,undefined)
target_link_options(igpp_tests PRIVATE -fsanitize=address,undefined)

enable_testing()
add_test(NAME igpp_tests COMMAND igpp_tests)
