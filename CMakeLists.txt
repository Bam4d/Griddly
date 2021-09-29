cmake_minimum_required(VERSION 3.10.0)
project(Griddly VERSION 1.2.8)

set(BIN_OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${CMAKE_BUILD_TYPE}/bin)

if (MSVC)
    message("Compiling with MSVC")

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    foreach (OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${BIN_OUTPUT_DIR})
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${BIN_OUTPUT_DIR})
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${BIN_OUTPUT_DIR})
    endforeach (OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES)
elseif (MINGW)
    message("Compiling with Mingw.")
    add_compile_options("-Wa,-mbig-obj")
endif ()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIR})

# use position independent code
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

# Require (at least) it
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Don't use e.g. GNU extension (like -std=gnu++11) for portability
set(CMAKE_CXX_EXTENSIONS OFF)
# Relevant cmake files are in this folder
set(CMAKE_CONFIG_FOLDER ${CMAKE_SOURCE_DIR}/cmake)
# the main library name
set(GRIDDLY_LIB_NAME libgriddly)
# the griddly test target name
set(GRIDDLY_TEST_BIN_NAME ${GRIDDLY_LIB_NAME}_Test)
# project main directory for all c++ related files
set(GRIDDLY_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
# project main directory for all c++ related files
set(GRIDDLY_PYBINDING_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bindings)
# project test directory
set(GRIDDLY_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)
# main test sources folder
set(GRIDDLY_TEST_SRC_DIR ${GRIDDLY_TEST_DIR}/src)
# the dependency folder specifying all required libs for conan
set(GRIDDLY_DEPS_DIR deps)
# The dependencies to load by Conan are in this file
set(CONANFILE conanfile.txt)

option(ENABLE_TESTING "Enable Test Builds" ON)

include(${CMAKE_CONFIG_FOLDER}/settings/Conan.cmake)
run_conan()
include(${PROJECT_BINARY_DIR}/conanbuildinfo.cmake)
include(${PROJECT_BINARY_DIR}/conan_paths.cmake)

# find the dependencies from conan
# pybind11
set(PYBIND11_FINDPYTHON TRUE)
find_package(pybind11 REQUIRED)
#glm
find_package(glm REQUIRED)
# Yaml-Cpp
find_package(yaml-cpp REQUIRED)
# stb
find_package(stb REQUIRED)
# Vulkan
find_package(Vulkan REQUIRED)

include(${CMAKE_CONFIG_FOLDER}/targets/griddly.cmake)
include(${CMAKE_CONFIG_FOLDER}/targets/pygriddly.cmake)

if (ENABLE_TESTING)
    message("\nBUILDING TESTS...\n")
    # Testing stuff
    find_package(GTest REQUIRED)
    include(CTest)
    enable_testing()
    include(${CMAKE_CONFIG_FOLDER}/targets/test.cmake)
endif ()
