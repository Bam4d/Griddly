cmake_minimum_required(VERSION 3.10.0)
project(GriddyRTS VERSION 0.1.0)

set(BINARY ${CMAKE_PROJECT_NAME})

find_package(Vulkan REQUIRED FATAL_ERROR)
find_package(glm REQUIRED FATAL_ERROR)

add_subdirectory("libs/pybind11")

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

# Add the spdlog libraries to the build
set(SPDLOG_DIR "libs/spdlog")
include_directories(${SPDLOG_DIR})

# Add the stb libraries to the build
set(STB_DIR "libs/stb")
include_directories(${STB_DIR})

add_library(${BINARY} SHARED ${GRIDDY_SOURCES})
target_link_libraries(${BINARY} PRIVATE Vulkan::Vulkan "stdc++fs")

# Add the pybind11 module
set(PYTHON_MODULE griddy)
pybind11_add_module(${PYTHON_MODULE} bindings/python.cpp)
target_link_libraries(${PYTHON_MODULE} PRIVATE ${BINARY} Vulkan::Vulkan "stdc++fs")

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

# Testing stuff
include(CTest)
enable_testing()

if(BUILD_TESTING)
    add_subdirectory (tests)
endif()
