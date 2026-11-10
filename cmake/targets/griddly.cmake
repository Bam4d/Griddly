file(GLOB_RECURSE GRIDDLY_SOURCES "${GRIDDLY_SRC_DIR}/*.cpp")

# The CUDA tiled renderer is only compiled when CUDA is available
if(NOT GRIDDLY_HAS_CUDA)
  list(FILTER GRIDDLY_SOURCES EXCLUDE REGEX ".*/Observers/CUDA/.*")
endif()
file(GLOB_RECURSE GRIDDLY_HEADERS "${GRIDDLY_SRC_DIR}/**.hpp")

set(GRIDDLY_INCLUDE_DIRS ${GRIDDLY_SRC_DIR})

# the main Griddly library
add_library(${GRIDDLY_LIB_NAME}_interface INTERFACE)

target_include_directories(
  ${GRIDDLY_LIB_NAME}_interface
  INTERFACE
  $<BUILD_INTERFACE:${GRIDDLY_INCLUDE_DIRS}>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(
  ${GRIDDLY_LIB_NAME}_interface
  INTERFACE
  project_options
  yaml-cpp
  glm::glm
  spdlog::spdlog
)

# If we are not compiling WASM, add SDL3 and stb
if(NOT WASM)
  target_link_libraries(
    ${GRIDDLY_LIB_NAME}_interface
    INTERFACE
    SDL3::SDL3
    stb::stb
  )

  if(GRIDDLY_HAS_CUDA)
    # Kernels live in their own object library so the host C++ warning flags never reach nvcc
    file(GLOB_RECURSE GRIDDLY_CUDA_SOURCES "${GRIDDLY_SRC_DIR}/*.cu")
    add_library(${GRIDDLY_LIB_NAME}_cuda OBJECT ${GRIDDLY_CUDA_SOURCES})
    set_target_properties(${GRIDDLY_LIB_NAME}_cuda PROPERTIES POSITION_INDEPENDENT_CODE ON CUDA_STANDARD 17)
    target_link_libraries(${GRIDDLY_LIB_NAME}_cuda PRIVATE CUDA::cudart_static)
    target_compile_definitions(${GRIDDLY_LIB_NAME}_interface INTERFACE GRIDDLY_HAS_CUDA)
    target_link_libraries(${GRIDDLY_LIB_NAME}_interface INTERFACE CUDA::cudart_static)
    list(APPEND GRIDDLY_SOURCES $<TARGET_OBJECTS:${GRIDDLY_LIB_NAME}_cuda>)
  endif()

  add_library(${GRIDDLY_LIB_NAME}_static STATIC ${GRIDDLY_SOURCES})
  add_library(${GRIDDLY_LIB_NAME}_shared SHARED ${GRIDDLY_SOURCES})

  set_target_properties(${GRIDDLY_LIB_NAME}_static
    PROPERTIES
    POSITION_INDEPENDENT_CODE ON
  )

  target_link_libraries(${GRIDDLY_LIB_NAME}_static
    PRIVATE
    $<BUILD_INTERFACE:project_warnings>
    PUBLIC
    ${GRIDDLY_LIB_NAME}_interface
  )
  target_link_libraries(${GRIDDLY_LIB_NAME}_shared
    PRIVATE
    $<BUILD_INTERFACE:project_warnings>
    PUBLIC
    ${GRIDDLY_LIB_NAME}_interface
  )
else()
  # WASM build: SDL3 observers are included, SDL3 comes from the emscripten port (see wasm.cmake)
  add_library(${GRIDDLY_LIB_NAME}_static STATIC ${GRIDDLY_SOURCES})

  set_target_properties(${GRIDDLY_LIB_NAME}_static
    PROPERTIES
    POSITION_INDEPENDENT_CODE ON
  )

  target_link_libraries(${GRIDDLY_LIB_NAME}_static
    PRIVATE
    $<BUILD_INTERFACE:project_warnings>
    PUBLIC
    ${GRIDDLY_LIB_NAME}_interface
  )
endif()
