# ============================================================================
# CureCraft Patient Monitor - CMake Build Configuration
# ============================================================================
cmake_minimum_required(VERSION 3.16)
project(CureCraft VERSION 1.0 LANGUAGES CXX)

# ============================================================================
# Build Options
# ============================================================================
option(BUILD_QT_GUI "Build Qt-based GUI" ON)
option(BUILD_WEB_SERVER "Build HTML-based web server" ON)

# ============================================================================
# Compiler Cache Setup (CRITICAL for fast incremental builds)
# ============================================================================
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set(CMAKE_CXX_COMPILER_LAUNCHER ccache)
    message(STATUS "✓ Using ccache for faster incremental builds")
else()
    message(WARNING "⚠ ccache not found - install with: sudo apt-get install ccache")
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
# Qt Configuration (Optional)
# ============================================================================
if(BUILD_QT_GUI)
    set(CMAKE_AUTOUIC ON)  # Auto-process .ui files
    set(CMAKE_AUTOMOC ON)  # Auto-process Qt meta-objects
    set(CMAKE_AUTORCC ON)  # Auto-process Qt resources

    find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets PrintSupport)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets PrintSupport)

    # Try to find OpenGL support (Qt6 uses OpenGLWidgets)
    if(${QT_VERSION_MAJOR} EQUAL 6)
        find_package(Qt6 COMPONENTS OpenGLWidgets QUIET)
        if(Qt6OpenGLWidgets_FOUND)
            message(STATUS "✓ Qt6 OpenGL support found")
            set(OPENGL_AVAILABLE TRUE)
        else()
            message(WARNING "⚠ Qt6 OpenGL not available - performance will be limited")
            set(OPENGL_AVAILABLE FALSE)
        endif()
    else()
        find_package(Qt5 COMPONENTS OpenGL QUIET)
        if(Qt5OpenGL_FOUND)
            set(OPENGL_AVAILABLE TRUE)
        else()
            set(OPENGL_AVAILABLE FALSE)
        endif()
    endif()

    message(STATUS "✓ Using Qt${QT_VERSION_MAJOR}")
endif()

# ============================================================================
# Source Files (Auto-discovered)
# ============================================================================
if(BUILD_QT_GUI)
    # Core application sources
    file(GLOB CORE_SOURCES 
        "${PROJECT_SOURCE_DIR}/src/*.cpp"
    )

    # GUI sources (excluding qcustomplot which is built separately)
    file(GLOB GUI_SOURCES 
        "${PROJECT_SOURCE_DIR}/src/gui/mainwindow.cpp"
    )

    # UI files
    file(GLOB UI_FILES
        "${PROJECT_SOURCE_DIR}/src/gui/*.ui"
    )

    # Headers
    file(GLOB HEADERS
        "${PROJECT_SOURCE_DIR}/include/*.h"
    )
endif()

# ============================================================================
# QCustomPlot Library (35k+ lines, cached separately)
# ============================================================================
if(BUILD_QT_GUI)
    # Build as static library to avoid recompiling on every change
    add_library(qcustomplot STATIC EXCLUDE_FROM_ALL 
        "${PROJECT_SOURCE_DIR}/src/gui/qcustomplot.cpp"
    )

    target_include_directories(qcustomplot PUBLIC 
        "${PROJECT_SOURCE_DIR}/include"
    )

    # Enable OpenGL support if available
    if(OPENGL_AVAILABLE)
        target_compile_definitions(qcustomplot PUBLIC QCUSTOMPLOT_USE_OPENGL)
        message(STATUS "✓ QCustomPlot OpenGL enabled")
    endif()

    # Optimized compilation flags for QCustomPlot
    target_compile_options(qcustomplot PRIVATE 
        -O3                    # Maximum optimization (safe for this library)  
        -fno-exceptions        # QCustomPlot doesn't use exceptions
        -pipe                  # Use pipes instead of temp files
        -ffast-math           # Fast math operations
        # NOTE: RTTI required for dynamic_cast
    )

    target_link_libraries(qcustomplot PUBLIC
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::PrintSupport
    )

    # Link OpenGL if available
    if(OPENGL_AVAILABLE)
        if(${QT_VERSION_MAJOR} EQUAL 6)
            target_link_libraries(qcustomplot PUBLIC Qt6::OpenGLWidgets)
        else()
            target_link_libraries(qcustomplot PUBLIC Qt5::OpenGL)
        endif()
    endif()

    # Cache compiled library
    set_target_properties(qcustomplot PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    )
endif()

# ============================================================================
# Main Executable (Qt GUI)
# ============================================================================
if(BUILD_QT_GUI)
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(curecraft
        MANUAL_FINALIZATION
        ${CORE_SOURCES}
        ${GUI_SOURCES}
        ${UI_FILES}
        ${HEADERS}
    )
else()
    add_executable(curecraft
        ${CORE_SOURCES}
        ${GUI_SOURCES}
        ${UI_FILES}
        ${HEADERS}
    )
endif()

# ============================================================================
# Compiler Optimizations for Main Executable
# ============================================================================
target_compile_options(curecraft PRIVATE
    -O3                    # Maximum optimization for application code
    -march=native          # Use Raspberry Pi 400 CPU features
    -fno-exceptions        # Reduce binary size
    -ffunction-sections    # Enable dead code elimination
    -fdata-sections        # Enable dead data elimination
    -pipe                  # Reduce disk I/O during compilation
    -ffast-math           # Fast floating point (safe for this app)
    # NOTE: RTTI kept enabled for QCustomPlot compatibility
)

# ============================================================================
# Linker Optimizations
# ============================================================================
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_link_options(curecraft PRIVATE 
        -Wl,--gc-sections   # Remove unused sections
        -Wl,-s              # Strip symbols (smaller binary)
    )
endif()

# ============================================================================
# Link Libraries
# ============================================================================
target_link_libraries(curecraft PRIVATE 
    qcustomplot
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::PrintSupport
    Qt${QT_VERSION_MAJOR}::OpenGL
)
endif()

# ============================================================================
# Include Directories
# ============================================================================
if(BUILD_QT_GUI)
    target_include_directories(curecraft PRIVATE
        "${PROJECT_SOURCE_DIR}/include"
    )
endif()

# ============================================================================
# Web Server Build (HTML UI)
# ============================================================================
if(BUILD_WEB_SERVER)
    # Find pthread for threading support
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)

    # Source files for web server
    set(WEB_SERVER_SOURCES
        "${PROJECT_SOURCE_DIR}/src/server/main_web.cpp"
        "${PROJECT_SOURCE_DIR}/src/server/webserver.cpp"
        "${PROJECT_SOURCE_DIR}/src/signal_generator.cpp"
    )

    # Create web server executable
    add_executable(curecraft-web ${WEB_SERVER_SOURCES})

    # Compiler optimizations
    target_compile_options(curecraft-web PRIVATE
        -O3
        -march=native
        -ffunction-sections
        -fdata-sections
        -pipe
        -ffast-math
    )

    # Linker optimizations (Linux-specific flags, skip on macOS)
    if(UNIX AND NOT APPLE)
        target_link_options(curecraft-web PRIVATE
            -Wl,--gc-sections
            -Wl,-s
        )
    endif()

    # Include directories
    target_include_directories(curecraft-web PRIVATE
        "${PROJECT_SOURCE_DIR}/include"
    )

    # Link pthread
    target_link_libraries(curecraft-web PRIVATE Threads::Threads)

    # Install web assets
    install(DIRECTORY "${PROJECT_SOURCE_DIR}/web/"
            DESTINATION "share/curecraft/web"
            FILES_MATCHING
            PATTERN "*.html"
            PATTERN "*.css"
            PATTERN "*.js")

    message(STATUS "✓ Web server build enabled")
endif()

# ============================================================================
# Build Summary
# ============================================================================
message(STATUS "")
message(STATUS "========================================")
message(STATUS "CureCraft Build Configuration")
message(STATUS "========================================")
if(BUILD_QT_GUI)
    message(STATUS "Qt GUI:         Enabled (Qt${QT_VERSION_MAJOR})")
else()
    message(STATUS "Qt GUI:         Disabled")
endif()
if(BUILD_WEB_SERVER)
    message(STATUS "Web Server:     Enabled")
else()
    message(STATUS "Web Server:     Disabled")
endif()
message(STATUS "Build Type:     ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler:       ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "C++ Standard:   C++${CMAKE_CXX_STANDARD}")
message(STATUS "LTO Enabled:    ${ipo_supported}")
message(STATUS "ccache:         ${CCACHE_FOUND}")
message(STATUS "========================================")
message(STATUS "")
