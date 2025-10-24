cmake_minimum_required(VERSION 3.20)
project(matching_engine_cpp)

# Export compile_commands.json for IDE IntelliSense (VS Code, clangd)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Use legacy FindBoost module behavior for Homebrew-installed Boost
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 OLD)
endif()
# Prefer legacy FindBoost module instead of BoostConfig.cmake
set(Boost_NO_BOOST_CMAKE ON)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use vcpkg toolchain file only if VCPKG_ROOT is defined and toolchain not specified
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{VCPKG_ROOT} AND NOT "$ENV{VCPKG_ROOT}" STREQUAL "")
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
endif()

# On macOS, ensure we have a valid SDK path and configure libc++ explicitly for CommandLineTools environments
if(APPLE)
    if(NOT CMAKE_OSX_SYSROOT)
        execute_process(
            COMMAND xcrun --show-sdk-path
            OUTPUT_VARIABLE MACOS_SDK_PATH
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        if(MACOS_SDK_PATH)
            set(CMAKE_OSX_SYSROOT "${MACOS_SDK_PATH}" CACHE STRING "macOS SDK path" FORCE)
        endif()
    endif()
endif()

# Homebrew hints so CMake can locate package config files installed by brew
if(APPLE)
    list(PREPEND CMAKE_PREFIX_PATH
        /opt/homebrew
        /opt/homebrew/opt/boost
        /opt/homebrew/opt/nlohmann-json
        /opt/homebrew/opt/prometheus-cpp
        /opt/homebrew/opt/googletest
        /usr/local
        /usr/local/opt/boost
        /usr/local/opt/nlohmann-json
        /usr/local/opt/prometheus-cpp
        /usr/local/opt/googletest
    )
endif()

# Find required packages
# Try to locate boost_system library, but fall back to header-only mode if not found
find_library(BOOST_SYSTEM_LIB
    NAMES boost_system boost_system-mt libboost_system
    HINTS
        /opt/homebrew/opt/boost/lib
        /opt/homebrew/lib
        /usr/local/opt/boost/lib
        /usr/local/lib)
if(NOT BOOST_SYSTEM_LIB)
    set(BOOST_SYSTEM_HEADER_ONLY ON)
    message(STATUS "boost_system not found as a library; using header-only Boost.System")
endif()

find_package(nlohmann_json CONFIG REQUIRED)
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)
# Note: Using Boost.Beast/Asio headers; no separate websocketpp/asio packages required
find_package(prometheus-cpp CONFIG REQUIRED)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    # libc++ headers for CommandLineTools SDK (needed when using explicit sysroot)
    $<$<BOOL:${APPLE}>:${CMAKE_OSX_SYSROOT}/usr/include/c++/v1>
    /opt/homebrew/include
    /usr/local/include
)

# Source files (exclude src/main.cpp from library sources)
file(GLOB_RECURSE SOURCES
    "src/engine/*.cpp"
    "src/network/*.cpp"
    "src/utils/*.cpp"
)

# Exclude deprecated/unused session.cpp to avoid duplicate WS handling and build issues
list(REMOVE_ITEM SOURCES "${CMAKE_SOURCE_DIR}/src/network/session.cpp")

# Main executable
add_executable(matching_engine src/main.cpp ${SOURCES})
if(APPLE)
    target_compile_options(matching_engine PRIVATE -stdlib=libc++)
endif()
target_link_libraries(matching_engine
    PRIVATE
    nlohmann_json::nlohmann_json
    Threads::Threads
    prometheus-cpp::core
    prometheus-cpp::pull
)
if(BOOST_SYSTEM_LIB)
    target_link_libraries(matching_engine PRIVATE ${BOOST_SYSTEM_LIB})
else()
    target_compile_definitions(matching_engine PRIVATE BOOST_ERROR_CODE_HEADER_ONLY=1)
endif()

# Unit tests
enable_testing()

# Core tests
add_executable(test_matching_engine
    tests/unit/test_matching_engine.cpp
    ${SOURCES}
)
if(APPLE)
    target_compile_options(test_matching_engine PRIVATE -stdlib=libc++)
endif()
target_link_libraries(test_matching_engine
    PRIVATE
    GTest::gtest
    GTest::gtest_main
    nlohmann_json::nlohmann_json
    Threads::Threads
    prometheus-cpp::core
    prometheus-cpp::pull
)
if(BOOST_SYSTEM_LIB)
    target_link_libraries(test_matching_engine PRIVATE ${BOOST_SYSTEM_LIB})
else()
    target_compile_definitions(test_matching_engine PRIVATE BOOST_ERROR_CODE_HEADER_ONLY=1)
endif()

# Advanced order tests (disabled until advanced order types are implemented)
# add_executable(test_advanced_orders
#     tests/unit/test_advanced_orders.cpp
#     ${SOURCES}
# )
# target_link_libraries(test_advanced_orders
#     PRIVATE
#     GTest::GTest
#     GTest::Main
#     ${Boost_LIBRARIES}
#     nlohmann_json::nlohmann_json
#     Threads::Threads
# )

# Benchmarks
add_executable(performance_benchmark
    benchmarks/performance_benchmark.cpp
    ${SOURCES}
)
if(APPLE)
    target_compile_options(performance_benchmark PRIVATE -stdlib=libc++)
endif()
target_link_libraries(performance_benchmark
    PRIVATE
    nlohmann_json::nlohmann_json
    Threads::Threads
    prometheus-cpp::core
    prometheus-cpp::pull
)
if(BOOST_SYSTEM_LIB)
    target_link_libraries(performance_benchmark PRIVATE ${BOOST_SYSTEM_LIB})
else()
    target_compile_definitions(performance_benchmark PRIVATE BOOST_ERROR_CODE_HEADER_ONLY=1)
endif()

# Add test targets
add_test(NAME matching_engine_tests COMMAND test_matching_engine)

# Install targets
install(TARGETS matching_engine
    RUNTIME DESTINATION bin
)

# Install headers
install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)