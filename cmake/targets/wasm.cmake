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
  "-lembind -fexceptions -s ENVIRONMENT=web -s ALLOW_MEMORY_GROWTH=1 -sNO_DISABLE_EXCEPTION_THROWING -sASYNCIFY -sMODULARIZE=1"
)

