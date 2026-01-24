cmake_minimum_required(VERSION 3.16)

# Project name and language
project(CureCraft LANGUAGES CXX)

# ---- Compiler Cache Setup (CRITICAL for fast incremental builds) ----
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set(CMAKE_CXX_COMPILER_LAUNCHER ccache)
    message(STATUS "Using ccache for faster incremental builds")
else()
    message(WARNING "ccache not found - incremental builds will be slow. Install with: sudo apt-get install ccache")
endif()

# Use modern C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

# Optimization flags for Raspberry Pi
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Enable LTO (Link Time Optimization) for smaller, faster binaries
include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported)
if(ipo_supported)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

# ---- Qt Configuration for GUI ----
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets PrintSupport)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets PrintSupport)

# ---- Core Project Source files ----
set(CORE_SOURCES
    src/main.cpp
    src/math.cpp
)

# ---- GUI Source files ----
set(GUI_SOURCES
    src/gui/mainwindow.cpp
    src/gui/qcustomplot.cpp
    src/gui/mainwindow.ui
)

# ---- Header files ----
set(HEADERS
    include/math.h
    include/mainwindow.h
    include/qcustomplot.h
)

# ---- Create QCustomPlot as a static library (cached separately) ----
# IMPORTANT: This 35k+ line file takes ~8 minutes to compile on Pi 400
# By making it a separate library, it only rebuilds when qcustomplot.cpp changes
add_library(qcustomplot STATIC EXCLUDE_FROM_ALL src/gui/qcustomplot.cpp)
target_include_directories(qcustomplot PUBLIC ${PROJECT_SOURCE_DIR}/include)

# Optimize qcustomplot compilation specifically
target_compile_options(qcustomplot PRIVATE 
    -O2                    # Moderate optimization
    -fno-exceptions        # QCustomPlot doesn't use exceptions
    -fno-rtti             # No runtime type info needed
    -pipe                 # Use pipes instead of temp files
)

target_link_libraries(qcustomplot PUBLIC
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::PrintSupport
)

# Mark qcustomplot library outputs for caching
set_target_properties(qcustomplot PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# ---- Create main CureCraft executable with integrated GUI ----
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(curecraft
        MANUAL_FINALIZATION
        ${CORE_SOURCES}
        ${GUI_SOURCES}
        ${HEADERS}
    )
else()
    add_executable(curecraft
        ${CORE_SOURCES}
        ${GUI_SOURCES}
        ${HEADERS}
    )
endif()

# Compiler optimizations
target_compile_options(curecraft PRIVATE
    -O2                    # Balanced optimization for embedded
    -march=native          # Use Pi 400 CPU features
    -fno-exceptions        # Reduce binary size
    -fno-rtti             # No runtime type info
    -ffunction-sections    # Enable dead code elimination
    -fdata-sections       # Enable data section elimination
    -pipe                 # Reduce disk I/O during compilation
)

# Linker optimizations
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_link_options(curecraft PRIVATE -Wl,--gc-sections -Wl,-s)
endif()

# Link libraries
target_link_libraries(curecraft PRIVATE 
    qcustomplot
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::PrintSupport
)

# ---- Include directories ----
target_include_directories(curecraft
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)
