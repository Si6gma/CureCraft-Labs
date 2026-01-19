cmake_minimum_required(VERSION 3.16)

# Project name and language
project(CureCraft LANGUAGES CXX)

# Use modern C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

# ---- Create QCustomPlot as a static library ----
add_library(qcustomplot STATIC src/gui/qcustomplot.cpp)
target_include_directories(qcustomplot PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(qcustomplot PUBLIC
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::PrintSupport
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
