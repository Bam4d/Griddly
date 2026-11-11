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
# emscripten sysroot. The sysroot include path is added in CMakeLists.txt before
# the core library target is defined (see the WASM block there).

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
