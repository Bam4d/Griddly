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
  yaml-cpp
  glm::glm
  spdlog::spdlog
  Boost::random
)

# If we are not compiling WASM, add vulkan and stb
if(NOT WASM)
  target_link_libraries(
    ${GRIDDLY_LIB_NAME}_interface
    INTERFACE
    volk::volk
    stb::stb
  )

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
  # If we are compiling WASM, then we remove all the vulkan sources
  list(
    REMOVE_ITEM
    GRIDDLY_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Griddly/Core/Observers/SpriteObserver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Griddly/Core/Observers/BlockObserver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Griddly/Core/Observers/IsometricSpriteObserver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Griddly/Core/Observers/VulkanGridObserver.cpp
  )

  list(
    FILTER
    GRIDDLY_SOURCES
    EXCLUDE
    REGEX
    "${GRIDDLY_SRC_DIR}/Griddly/Core/Observers/Vulkan/.*"
  )

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
