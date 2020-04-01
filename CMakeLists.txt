cmake_minimum_required(VERSION 3.10.0)
project(GriddyRTS VERSION 0.1.0)

set(BINARY ${CMAKE_PROJECT_NAME})

find_package(Vulkan REQUIRED FATAL_ERROR)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
# Require (at least) it
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Don't use e.g. GNU extension (like -std=gnu++11) for portability
set(CMAKE_CXX_EXTENSIONS OFF)

set(VULKAN_SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Griddy/Core/Observers/Vulkan/resources/shaders)

file(GLOB_RECURSE GRIDDY_SOURCES "src/*.cpp")

set (GRIDDY_INCLUDE_DIRS "")
foreach (_headerFile ${GRIDDY_HEADERS})
    get_filename_component(_dir ${_headerFile} PATH)
    list (APPEND GRIDDY_INCLUDE_DIRS ${_dir})
endforeach()
list(REMOVE_DUPLICATES GRIDDY_INCLUDE_DIRS)

# Compile shaders and copy them into resources directory in build output
execute_process ( COMMAND ${VULKAN_SHADER_DIR}/compile_shaders.sh  
                  WORKING_DIRECTORY ${VULKAN_SHADER_DIR} ) 

set(LIBS_DIR "libs/")

include_directories(${LIBS_DIR})

add_library(${BINARY} STATIC ${GRIDDY_SOURCES})
target_link_libraries(${BINARY} Vulkan::Vulkan "stdc++fs")

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

# Testing stuff
include(CTest)
enable_testing()

if(BUILD_TESTING)
    add_subdirectory (tests)
endif()
