cmake_minimum_required(VERSION 3.10.0)
project(Griddly VERSION 1.2.8)

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
# Relevant cmake files are in this folder
set(CMAKE_CONFIG_FOLDER ${CMAKE_SOURCE_DIR}/cmake)
# the dependency folder specifying all required libs for conan
set(DEPENDENCY_DIR deps)
# Relevant cmake files are in this folder
set(CONANFILE conanfile.txt)

include(${CMAKE_CONFIG_FOLDER}/Conan.cmake)
run_conan()
include(${PROJECT_BINARY_DIR}/conanbuildinfo.cmake)
include(${PROJECT_BINARY_DIR}/conan_paths.cmake)

# find the downloaded/installed packages
find_package(glm REQUIRED)

# pybind11
set(PYBIND11_FINDPYTHON TRUE)
find_package(pybind11 REQUIRED)

# Yaml-Cpp
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "disable yaml tests")
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "disable yaml tools")
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "disable yaml contrib")
set(YAML_BUILD_SHARED_LIBS OFF CACHE BOOL "disable shared libs")
find_package(yaml-cpp REQUIRED)

# stb
find_package(stb REQUIRED)
if(TARGET CONAN_PKG::stb)
    message("STB IS DEFINITELY HERE")
    message("${stb_INCLUDE_DIRS}")
    get_target_property(stb_dirs_hey CONAN_PKG::stb INTERFACE_INCLUDE_DIRECTORIES )
    message("stb dirs: ${stb_dirs_hey}")
    endif()

# Vulkan
find_package(Vulkan REQUIRED)
#find_package(VulkanHeaders REQUIRED FATAL_ERROR)
#find_package(VulkanLoader REQUIRED FATAL_ERROR)
#find_package(vulkan-validationlayers REQUIRED FATAL_ERROR)
#find_package(vulkan-memory-allocator REQUIRED FATAL_ERROR)

set(VULKAN_SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Griddly/Core/Observers/Vulkan/resources/shaders)

file(GLOB_RECURSE GRIDDLY_SOURCES "src/*.cpp")

set (GRIDDLY_INCLUDE_DIRS "")
foreach (_headerFile ${GRIDDLY_HEADERS})
    get_filename_component(_dir ${_headerFile} PATH)
    list (APPEND GRIDDLY_INCLUDE_DIRS ${_dir})
endforeach()
list(REMOVE_DUPLICATES GRIDDLY_INCLUDE_DIRS)

message(STATUS "Compiling shaders...")

# Compile shaders and copy them into resources directory in build output

if(MSVC)
    execute_process ( COMMAND ${VULKAN_SHADER_DIR}/compile_shaders.bat  
                    WORKING_DIRECTORY ${VULKAN_SHADER_DIR} 
                    RESULT_VARIABLE rv)
else()
    execute_process ( COMMAND bash ${VULKAN_SHADER_DIR}/compile_shaders.sh  
                    WORKING_DIRECTORY ${VULKAN_SHADER_DIR} 
                    RESULT_VARIABLE rv)
endif()

add_library(${BINARY} STATIC ${GRIDDLY_SOURCES})
target_link_libraries(${BINARY}
        PUBLIC
#        CONAN_PKG::vulkan_Vulkan
        CONAN_PKG::vulkan-headers
        CONAN_PKG::vulkan-loader
        CONAN_PKG::vulkan-validationlayers
        CONAN_PKG::vulkan-memory-allocator
        CONAN_PKG::yaml-cpp
        CONAN_PKG::glm
        CONAN_PKG::spdlog
        CONAN_PKG::stb
        )
message(${BINARY})
get_target_property(griddly_dirs_hey ${BINARY} INCLUDE_DIRECTORIES )
message("griddly dirs: ${griddly_dirs_hey}")
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
target_link_libraries(${PYTHON_MODULE} PRIVATE
        ${BINARY}
        )

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

# Testing stuff
include(CTest)
enable_testing()

if(BUILD_TESTING)
    add_subdirectory (tests)
endif()
