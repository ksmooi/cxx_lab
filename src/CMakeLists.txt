add_subdirectory(apps)
add_subdirectory(demo)
add_subdirectory(tests)

# Include the thread module directory so that its headers can be used
include_directories(${CMAKE_SOURCE_DIR}/src/modules)

