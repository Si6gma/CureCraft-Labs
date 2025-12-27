cmake_minimum_required(VERSION 3.20)

project(CureCraftLabs VERSION 0.1.0 LANGUAGES CXX)

# Create the executable target
add_executable(curecraft)

# C++ standard (target-scoped)
target_compile_features(curecraft PRIVATE cxx_std_20)

# Includes (your headers live here)
target_include_directories(curecraft PRIVATE
  "${PROJECT_SOURCE_DIR}/include"
)

# Warnings (good defaults for GCC/Clang on Pi)
target_compile_options(curecraft PRIVATE
  -Wall -Wextra -Wpedantic
)

# Add src/ (where we list sources)
add_subdirectory(src)
