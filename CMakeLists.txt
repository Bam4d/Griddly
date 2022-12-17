cmake_minimum_required(VERSION 3.18.0)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum OS X deployment version")
project(Griddly VERSION 1.6.3)

string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWERCASE)

set(BIN_OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${CMAKE_BUILD_TYPE}/bin)

# Use C++17
set(PROJ_CXX_STD_FEATURE cxx_std_17)
set(PROJ_CXX_STANDARD C++17)

# Require (at least) it
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Don't use e.g. GNU extension (like -std=gnu++11) for portability
set(CMAKE_CXX_EXTENSIONS OFF)

# Relevant cmake files are in this folder
set(CMAKE_CONFIG_FOLDER ${CMAKE_SOURCE_DIR}/cmake)

# the main library name
set(GRIDDLY_LIB_NAME Griddly)

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

# project resources folder (e.g. map files, shaders, configs etc.)
set(GRIDDLY_RESOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/resources)

option(BUILD_SHARED_LIBS "Enable compilation of shared libraries" OFF)
option(ENABLE_CACHE "Enable cache if available" ON)
option(ENABLE_CLANG_TIDY "Enable static analysis with clang-tidy" OFF)
option(ENABLE_COVERAGE "Enable coverage reporting for gcc/clang" OFF)
option(ENABLE_CPPCHECK "Enable static analysis with cppcheck" OFF)
option(ENABLE_INCLUDE_WHAT_YOU_USE "Enable static analysis with include-what-you-use" OFF)
option(ENABLE_IPO "Enable Interprocedural Optimization, aka Link Time Optimization (LTO)" OFF)
option(ENABLE_PCH "Enable Precompiled Headers" ON)
option(ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" OFF)
option(ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
option(ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF)
option(ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
option(ENABLE_SANITIZER_UNDEFINED_BEHAVIOR "Enable undefined behavior sanitizer" OFF)
option(ENABLE_PYTHON_BINDINGS "Enable to build the bindings to other languages." ON)
option(ENABLE_TESTING "Enable Test Builds" ON)
option(WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(WASM "Enable Web-assembly build" OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIR})

# Link this 'library' to set the c++ standard / compile-time options requested
add_library(project_options INTERFACE)

# Link this 'library' to use the warnings specified in CompilerWarnings.cmake
add_library(project_warnings INTERFACE)

message(STATUS ${CMAKE_SYSTEM_NAME})

if(WASM)
  message(STATUS "Compiling for webassembly using emscripten")
  target_compile_definitions(project_options INTERFACE "-DWASM")
  target_compile_options(project_options INTERFACE "-fexceptions")
endif()

if(MSVC)
  message("Compiling with MSVC")

  target_compile_options(project_options INTERFACE "/bigobj")
  set_target_properties(project_options PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

target_compile_features(project_options INTERFACE ${PROJ_CXX_STD_FEATURE})

if(ENABLE_PCH)
  target_precompile_headers(project_options
    INTERFACE
    <vector>
    <string>
    <map>
    <utility>
    <memory>
    <array>
    <optional>
  )
endif()

# import utility methods for cmake
include(${CMAKE_CONFIG_FOLDER}/settings/Utilities.cmake)

# enable clang-format and clang-tidy project wide
include(${CMAKE_CONFIG_FOLDER}/settings/Clang-cxx-dev-tools.cmake)

# enable cache system
include(${CMAKE_CONFIG_FOLDER}/settings/Cache.cmake)

# standard compiler warnings
include(${CMAKE_CONFIG_FOLDER}/settings/CompilerWarnings.cmake)
set_project_warnings(project_warnings)

# sanitizer options if supported by compiler
include(${CMAKE_CONFIG_FOLDER}/settings/Sanitizers.cmake)
enable_sanitizers(project_options)

# allow for static analysis
include(${CMAKE_CONFIG_FOLDER}/settings/StaticAnalyzers.cmake)

include(${CMAKE_CONFIG_FOLDER}/settings/Conan.cmake)
run_conan()
include(${PROJECT_BINARY_DIR}/conan_paths.cmake)

if(NOT WASM)
  # find the dependencies from conan
  set(PYBIND11_FINDPYTHON FALSE)

  # set(Python_ROOT_DIR /opt/python/$ENV{PYBIN})
  find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)
  # pybind11
  find_package(pybind11 REQUIRED)

  # GTest
  if(ENABLE_TESTING)
    find_package(GTest REQUIRED)
  endif()

  # stb
  find_package(stb REQUIRED)

  # Vulkan
  find_package(volk REQUIRED)

  # ShaderC for compiling shaders
  find_package(shaderc REQUIRED)
endif()

# glm
find_package(glm REQUIRED)

# Yaml-Cpp
find_package(yaml-cpp REQUIRED)

# spdlog
find_package(spdlog REQUIRED)

include(${CMAKE_CONFIG_FOLDER}/targets/griddly.cmake)

if(WASM)
  include(${CMAKE_CONFIG_FOLDER}/targets/wasm.cmake)
else()
  # Compile shaders and copy them into resources directory in build output
  if(NOT WASM)
    message(STATUS "Compiling shaders...")

    set(ENV{GLSLC_BIN} ${CONAN_SHADERC_ROOT}/bin/glslc)

    if(MSVC)
      execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/compile_shaders.bat RESULT_VARIABLE rv)
    else()
      execute_process(COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/compile_shaders.sh RESULT_VARIABLE rv)
    endif()

    if(ENABLE_PYTHON_BINDINGS)
      message("Configuring Python Bindings.")
      include(${CMAKE_CONFIG_FOLDER}/targets/python_griddly.cmake)
    endif()

    if(ENABLE_TESTING)
      message("Configuring Tests.")
      set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1) # prevent CTest from flooding the target space with CI/CD targets
      include(CTest)
      enable_testing()
      include(${CMAKE_CONFIG_FOLDER}/targets/test.cmake)
    endif()
  endif()

  include(CMakePackageConfigHelpers)

  # Want the python lib to be output in the same directory as the other dll/so
  if(MSVC)
    foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
      string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
      set_target_properties(${PYTHON_MODULE} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${BIN_OUTPUT_DIR})
    endforeach(OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES)
  endif()

  write_basic_package_version_file(
    "${PROJECT_BINARY_DIR}/${PROJECT_NAME_LOWERCASE}ConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
  )
  install(TARGETS project_options EXPORT ${PROJECT_NAME_LOWERCASE}Options)
  install(TARGETS ${GRIDDLY_LIB_NAME}_interface ${GRIDDLY_LIB_NAME}_shared ${GRIDDLY_LIB_NAME}_static
    EXPORT ${PROJECT_NAME_LOWERCASE}Targets
    LIBRARY DESTINATION lib COMPONENT Runtime
    ARCHIVE DESTINATION lib COMPONENT Development
    RUNTIME DESTINATION bin COMPONENT Runtime
    PUBLIC_HEADER DESTINATION include COMPONENT Development
    BUNDLE DESTINATION bin COMPONENT Runtime
  )

  include(CMakePackageConfigHelpers)
  configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME_LOWERCASE}Config.cmake.in"
    "${PROJECT_BINARY_DIR}/${PROJECT_NAME_LOWERCASE}Config.cmake"
    INSTALL_DESTINATION lib/cmake/${PROJECT_NAME_LOWERCASE}
  )
  install(EXPORT ${PROJECT_NAME_LOWERCASE}Options DESTINATION lib/cmake/${PROJECT_NAME_LOWERCASE})
  install(EXPORT ${PROJECT_NAME_LOWERCASE}Targets DESTINATION lib/cmake/${PROJECT_NAME_LOWERCASE})
  install(FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME_LOWERCASE}ConfigVersion.cmake"
    "${PROJECT_BINARY_DIR}/${PROJECT_NAME_LOWERCASE}Config.cmake"
    DESTINATION lib/cmake/${PROJECT_NAME_LOWERCASE})
  install(FILES ${GRIDDLY_HEADERS} DESTINATION include)
  install(DIRECTORY ${GRIDDLY_RESOURCE_DIR} DESTINATION resources)
endif()
