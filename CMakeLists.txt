cmake_minimum_required(VERSION 3.16.0)
project(Griddly VERSION 1.2.10)

string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWERCASE)

#set(BIN_OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${CMAKE_BUILD_TYPE}/bin)

if (MSVC)
    message("Compiling with MSVC")

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
elseif (MINGW)
    message("Compiling with Mingw.")
    add_compile_options("-Wa,-mbig-obj")
endif ()

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
set(GRIDDLY_LIB_NAME griddly)
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
# the dependency folder specifying all required libs for conan
set(GRIDDLY_DEPS_DIR deps)
# The dependencies to load by Conan are in this file
set(CONANFILE conanfile.txt)

option(BUILD_SHARED_LIBS "Enable compilation of shared libraries" OFF)
option(ENABLE_BUILD_DOCS "Enable building the docs. Requires doxygen to be installed on the system" OFF)
option(ENABLE_BUILD_WITH_TIME_TRACE "Enable -ftime-trace to generate time tracing .json files on clang" OFF)
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


# Link this 'library' to set the c++ standard / compile-time options requested
add_library(project_options INTERFACE)
# Link this 'library' to use the warnings specified in CompilerWarnings.cmake
add_library(project_warnings INTERFACE)

target_compile_features(project_options INTERFACE ${PROJ_CXX_STD_FEATURE})

if (ENABLE_PCH)
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
endif ()

# import utility methods for cmake
include(${CMAKE_CONFIG_FOLDER}/settings/Utilities.cmake)

#enable clang-format and clang-tidy project wide
include(${CMAKE_CONFIG_FOLDER}/settings/Clang-cxx-dev-tools.cmake)

# enable cache system
include(${CMAKE_CONFIG_FOLDER}/settings/Cache.cmake)

# standard compiler warnings
include(${CMAKE_CONFIG_FOLDER}/settings/CompilerWarnings.cmake)
set_project_warnings(project_warnings)

# sanitizer options if supported by compiler
include(${CMAKE_CONFIG_FOLDER}/settings/Sanitizers.cmake)
enable_sanitizers(project_options)

# enable doxygen
include(${CMAKE_CONFIG_FOLDER}/settings/Doxygen.cmake)
enable_doxygen()

# allow for static analysis
include(${CMAKE_CONFIG_FOLDER}/settings/StaticAnalyzers.cmake)


include(${CMAKE_CONFIG_FOLDER}/settings/Conan.cmake)
run_conan()
include(${PROJECT_BINARY_DIR}/conanbuildinfo.cmake)
include(${PROJECT_BINARY_DIR}/conan_paths.cmake)

# find the dependencies from conan
# pybind11
find_package(pybind11 REQUIRED)
#glm
find_package(glm REQUIRED)
# Yaml-Cpp
find_package(yaml-cpp REQUIRED)
# stb
find_package(stb REQUIRED)
# Vulkan
find_package(Vulkan REQUIRED)
# GTest
if (ENABLE_TESTING)
    find_package(GTest REQUIRED)
endif ()

include(${CMAKE_CONFIG_FOLDER}/targets/griddly.cmake)

if(ENABLE_PYTHON_BINDINGS)
    message("Configuring Python Bindings.")
    include(${CMAKE_CONFIG_FOLDER}/targets/python_griddly.cmake)
endif()

if (ENABLE_TESTING)
    message("Configuring Tests.")
    set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1) # prevent CTest from flooding the target space with CI/CD targets
    include(CTest)
    enable_testing()
    include(${CMAKE_CONFIG_FOLDER}/targets/test.cmake)
endif ()


include(CMakePackageConfigHelpers)
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
