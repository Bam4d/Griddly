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
  "src/Griddly/Core/Observers/Vulkan/.*"
)

file(GLOB_RECURSE JIDDLY_SOURCES "js/bindings/*.cpp")

add_executable(griddlyjs ${JIDDLY_SOURCES})
target_link_libraries(
  griddlyjs
  PRIVATE
  ${GRIDDLY_LIB_NAME}
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
  "-lembind -fexceptions -s ENVIRONMENT=web -s ALLOW_MEMORY_GROWTH=1 -sNO_DISABLE_EXCEPTION_THROWING -sASYNCIFY -sMODULARIZE=1"
)