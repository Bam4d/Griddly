# WASM build: dependencies come from CMake FetchContent (header-only where possible)
# and SDL3 from the emscripten port (-sUSE_SDL=3).
include(FetchContent)

FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG 0.9.9.8
)
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.9.2
)
FetchContent_Declare(
  stb
  GIT_REPOSITORY https://github.com/nothings/stb.git
  GIT_TAG master
)
FetchContent_Declare(
  yaml-cpp
  GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
  GIT_TAG yaml-cpp-0.6.3
)
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glm spdlog yaml-cpp stb)
include_directories(${stb_SOURCE_DIR} ${stb_SOURCE_DIR}/deprecated)

# SDL3 comes from the emscripten port (-sUSE_SDL=3); its headers live in the
# emscripten sysroot. EMSCRIPTEN_SYSROOT is only set as a CMake variable by the
# Emscripten toolchain module, so fall back to $ENV{EMSCRIPTEN_SYSROOT} or the
# sysroot next to the toolchain (derived from CMAKE_TOOLCHAIN_FILE) if unset.
if(NOT EMSCRIPTEN_SYSROOT)
  if(DEFINED ENV{EMSCRIPTEN_SYSROOT})
    set(EMSCRIPTEN_SYSROOT "$ENV{EMSCRIPTEN_SYSROOT}")
  elseif(CMAKE_TOOLCHAIN_FILE)
    get_filename_component(_emsdk_emscripten_dir "${CMAKE_TOOLCHAIN_FILE}" DIRECTORY)
    get_filename_component(_emsdk_emscripten_dir "${_emsdk_emscripten_dir}" DIRECTORY)
    get_filename_component(_emsdk_upstream "${_emsdk_emscripten_dir}" DIRECTORY)
    set(EMSCRIPTEN_SYSROOT "${_emsdk_upstream}/cache/sysroot")
  endif()
endif()
message(STATUS "WASM: using emscripten sysroot ${EMSCRIPTEN_SYSROOT}")
# Attach the sysroot include to the library target itself: include_directories()
# here runs after add_library(Griddly_static) in griddly.cmake, so a plain
# directory-level include never reaches the core sources.
add_compile_options("-I${EMSCRIPTEN_SYSROOT}/include")

file(GLOB_RECURSE JIDDLY_SOURCES "js/bindings/*.cpp")

add_executable(griddlyjs ${JIDDLY_SOURCES})
target_link_libraries(
  griddlyjs
  PRIVATE
  ${GRIDDLY_LIB_NAME}_static
  project_options
  project_warnings
  yaml-cpp
  spdlog::spdlog
  glm::glm
)

set_target_properties(
  griddlyjs
  PROPERTIES
  LINK_FLAGS
  "-lembind -fexceptions -s ENVIRONMENT=web -s ALLOW_MEMORY_GROWTH=1 -sNO_DISABLE_EXCEPTION_THROWING -sASYNCIFY -sMODULARIZE=1 -sEXPORT_ES6=1 -sUSE_SDL=3 -sEXPORTED_RUNTIME_METHODS=FS --preload-file ${CMAKE_SOURCE_DIR}/resources@/resources"
)
