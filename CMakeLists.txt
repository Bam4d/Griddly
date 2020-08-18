cmake_minimum_required(VERSION 3.10.0)
project(Griddly VERSION 0.1.1)

set(BINARY ${CMAKE_PROJECT_NAME})

set(BIN_OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${CMAKE_BUILD_TYPE}/bin)

if(MSVC)
message("Compiling with MSVC")

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
	set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${BIN_OUTPUT_DIR} )
	set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${BIN_OUTPUT_DIR} )
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${BIN_OUTPUT_DIR} )
    endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )
elseif(MINGW)
    message("Compiling with Mingw.")
    add_compile_options("-Wa,-mbig-obj")
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIR})

# use position independent code
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
# Require (at least) it
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Don't use e.g. GNU extension (like -std=gnu++11) for portability
set(CMAKE_CXX_EXTENSIONS OFF)

# GLM
add_subdirectory("libs/glm")

# PyBind
add_subdirectory("libs/pybind11")

# Yaml-Cpp
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "disable yaml tests")
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "disable yaml tools")
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "disable yaml contrib")
set(YAML_BUILD_SHARED_LIBS OFF CACHE BOOL "disable shared libs")
add_subdirectory("libs/yaml-cpp")


find_package(Vulkan REQUIRED FATAL_ERROR)
set(VULKAN_SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Griddly/Core/Observers/Vulkan/resources/shaders)

file(GLOB_RECURSE GRIDDLY_SOURCES "src/*.cpp")

set (GRIDDLY_INCLUDE_DIRS "")
foreach (_headerFile ${GRIDDLY_HEADERS})
    get_filename_component(_dir ${_headerFile} PATH)
    list (APPEND GRIDDLY_INCLUDE_DIRS ${_dir})
endforeach()
list(REMOVE_DUPLICATES GRIDDLY_INCLUDE_DIRS)

# Compile shaders and copy them into resources directory in build output
execute_process ( COMMAND bash ${VULKAN_SHADER_DIR}/compile_shaders.sh  
                  WORKING_DIRECTORY ${VULKAN_SHADER_DIR} 
                  RESULT_VARIABLE rv)

# Add the spdlog libraries to the build
set(SPDLOG_DIR "libs/spdlog")
include_directories(${SPDLOG_DIR})

# Add the stb libraries to the build
set(STB_DIR "libs/stb")
include_directories(${STB_DIR})

add_library(${BINARY} STATIC ${GRIDDLY_SOURCES})
target_link_libraries(${BINARY} PRIVATE Vulkan::Vulkan yaml-cpp glm)

# Add the pybind11 module
set(PYTHON_MODULE python_griddly)
pybind11_add_module(${PYTHON_MODULE} bindings/python.cpp)

# Want the python lib to be output in the same directory as the other dll/so
if(MSVC)
    foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
        string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
        set_target_properties(${PYTHON_MODULE} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${BIN_OUTPUT_DIR})
    endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )
endif()

set_target_properties(${PYTHON_MODULE} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIR})
target_link_libraries(${PYTHON_MODULE} PRIVATE ${BINARY} Vulkan::Vulkan yaml-cpp glm)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

# Testing stuff
include(CTest)
enable_testing()

if(BUILD_TESTING)
    add_subdirectory (tests)
endif()
