cmake_minimum_required(VERSION 3.16)

# Project name and language
project(CureCraft LANGUAGES CXX)

# Use modern C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Source files ----
set(SOURCES
    src/main.cpp
    src/math.cpp
)

# ---- Header files ----
# (Not strictly required by CMake, but listed for clarity)
set(HEADERS
    include/math.h
)

# ---- Create executable ----
add_executable(curecraft
    ${SOURCES}
    ${HEADERS}
)

# ---- Include directories ----
target_include_directories(curecraft
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)
