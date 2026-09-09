cmake_minimum_required(VERSION 3.15)

if(NOT CMAKE_GENERATOR_PLATFORM STREQUAL "Win32" AND NOT CMAKE_VS_PLATFORM_NAME STREQUAL "Win32")
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR
            "This project must be configured for a 32-bit (Win32) target, "
            "e.g. `cmake -S . -B build -A Win32`. "
            "EuroScope only loads 32-bit plugin DLLs.")
    endif()
endif()

project(OFPlugin CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EUROSCOPE_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/euroscope-sdk")
set(NLOHMANN_JSON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/nlohmann-json")

add_library(OFPlugin SHARED
    src/OFPlugIn.h
    src/OFPlugIn.cpp
    src/FrequencyStore.h
    src/FrequencyStore.cpp
    src/TransceiverFetcher.h
    src/TransceiverFetcher.cpp
)

target_include_directories(OFPlugin PRIVATE
    "${EUROSCOPE_SDK_DIR}/include"
    "${NLOHMANN_JSON_DIR}/include"
)

target_link_libraries(OFPlugin PRIVATE
    "${EUROSCOPE_SDK_DIR}/lib/EuroScopePlugInDll.lib"
    winhttp
)

target_compile_definitions(OFPlugin PRIVATE _MBCS)

target_compile_options(OFPlugin PRIVATE /Zc:wchar_t-)

set_target_properties(OFPlugin PROPERTIES
    OUTPUT_NAME "OFPlugin"
    SUFFIX ".dll"
)
