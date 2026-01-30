# ============================================================================
# CureCraft Patient Monitor - CMake Build Configuration
# ============================================================================
cmake_minimum_required(VERSION 3.16)
project(CureCraft VERSION 2.0 LANGUAGES CXX)

# ============================================================================
# Compiler Cache Setup (CRITICAL for fast incremental builds)
# ============================================================================
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set(CMAKE_CXX_COMPILER_LAUNCHER ccache)
    message(STATUS "✓ Using ccache for faster incremental builds")
else()
    message(STATUS "⚠ ccache not found - builds will be slower")
endif()

# ============================================================================
# C++ Standard and Optimization Settings
# ============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

# Default to Release build
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Enable Link-Time Optimization (LTO) for smaller, faster binaries
include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported)
if(ipo_supported)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    message(STATUS "✓ Link-Time Optimization (LTO) enabled")
endif()

# ============================================================================
# Main Application (Web Server + Hardware Backend)
# ============================================================================

# Find pthread for threading support
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Find mosquitto MQTT library
find_package(PkgConfig REQUIRED)
pkg_check_modules(MOSQUITTO REQUIRED libmosquitto)

# Source files
set(SOURCES
        "${PROJECT_SOURCE_DIR}/src/main.cpp"
        "${PROJECT_SOURCE_DIR}/src/server/webserver.cpp"
        "${PROJECT_SOURCE_DIR}/src/server/auth.cpp"
        "${PROJECT_SOURCE_DIR}/src/core/signal_generator.cpp"
        "${PROJECT_SOURCE_DIR}/src/core/MQTTDriver.cpp"
        "${PROJECT_SOURCE_DIR}/src/core/SensorDataStore.cpp"
        "${PROJECT_SOURCE_DIR}/src/hardware/i2c_driver.cpp"
        "${PROJECT_SOURCE_DIR}/src/hardware/sensor_manager.cpp"
)

# Create main executable
add_executable(curecraft ${SOURCES})

# Compiler optimizations and warnings
target_compile_options(curecraft PRIVATE
        -O3
        -march=native
        -ffunction-sections
        -fdata-sections
        -pipe
        -ffast-math
        -Wall
        -Wextra
        -Wpedantic
)

# Linker optimizations (Linux-specific flags, skip on macOS)
if(UNIX AND NOT APPLE)
    target_link_options(curecraft PRIVATE
            -Wl,--gc-sections
            -Wl,-s
    )
endif()

# Include directories
target_include_directories(curecraft PRIVATE
        "${PROJECT_SOURCE_DIR}/include"
        ${MOSQUITTO_INCLUDE_DIRS}
)

# Link directories (for mosquitto)
target_link_directories(curecraft PRIVATE
        ${MOSQUITTO_LIBRARY_DIRS}
)

# Link libraries
target_link_libraries(curecraft PRIVATE
        Threads::Threads
        ${MOSQUITTO_LIBRARIES}
)

# Install web assets
install(DIRECTORY "${PROJECT_SOURCE_DIR}/web/"
        DESTINATION "share/curecraft/web"
        FILES_MATCHING
        PATTERN "*.html"
        PATTERN "*.css"
        PATTERN "*.js")

# ============================================================================
# Build Summary
# ============================================================================
message(STATUS "")
message(STATUS "========================================")
message(STATUS "CureCraft Build Configuration")
message(STATUS "========================================")
message(STATUS "Platform:       Web-Based Architecture + MQTT")
message(STATUS "Build Type:     ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler:       ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "C++ Standard:   C++${CMAKE_CXX_STANDARD}")
message(STATUS "LTO Enabled:    ${ipo_supported}")
message(STATUS "ccache:         ${CCACHE_FOUND}")
message(STATUS "========================================")
message(STATUS "")
