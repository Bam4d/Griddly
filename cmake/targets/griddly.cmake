file(GLOB_RECURSE GRIDDLY_SOURCES "${GRIDDLY_SRC_DIR}/*.cpp")
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
  CONAN_PKG::yaml-cpp
  CONAN_PKG::glm
  CONAN_PKG::spdlog
  CONAN_PKG::stb
)

if(NOT WASM)
  target_link_libraries(
    ${GRIDDLY_LIB_NAME}_interface
    INTERFACE
    volk::volk
  )
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