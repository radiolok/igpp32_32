# Hardware-independent core: frame format, scan sequencer, protocol, .igv.
# Builds as an ESP-IDF component and as a plain library for host tests.
set(IGPP_CORE_SRCS
    src/crc16.cpp
    src/protocol.cpp
    src/igv.cpp
    src/patterns.cpp
    src/msp_picture.cpp
)

if(ESP_PLATFORM)
    idf_component_register(SRCS ${IGPP_CORE_SRCS}
                           INCLUDE_DIRS include)
else()
    add_library(igpp_core STATIC ${IGPP_CORE_SRCS})
    target_include_directories(igpp_core PUBLIC include)
    target_compile_features(igpp_core PUBLIC cxx_std_17)
endif()
