cmake_minimum_required(VERSION 3.10.0)
project(Griddy VERSION 0.1.0)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
# Require (at least) it
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Don't use e.g. GNU extension (like -std=gnu++11) for portability
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)
enable_testing()

file(GLOB_RECURSE Griddy_SOURCES "src/*.cpp")

set (Griddy_INCLUDE_DIRS "")
foreach (_headerFile ${Griddy_HEADERS})
    get_filename_component(_dir ${_headerFile} PATH)
    list (APPEND Griddy_INCLUDE_DIRS ${_dir})
endforeach()
list(REMOVE_DUPLICATES Griddy_INCLUDE_DIRS)

set(LIBS_DIR "libs/")

include_directories(${LIBS_DIR})

add_executable (GriddyExe ${Griddy_SOURCES})
#target_link_libraries(GriddyExe PRIVATE spdlog::spdlog_header_only)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
