# Grab all .cpp files under src/ recursively
file(GLOB_RECURSE CONFIGURE_DEPENDS SRC_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

if(NOT SRC_FILES)
  message(FATAL_ERROR "No .cpp files found under src/")
endif()

target_sources(curecraft PRIVATE ${SRC_FILES})
