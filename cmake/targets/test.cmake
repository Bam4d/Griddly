

file(GLOB_RECURSE TEST_SOURCES "${GRIDDLY_TEST_SRC_DIR}/*.cpp")

add_executable(
  ${GRIDDLY_TEST_BIN_NAME}
  ${TEST_SOURCES}
)

add_test(
  NAME ${GRIDDLY_TEST_BIN_NAME}
  COMMAND ${GRIDDLY_TEST_BIN_NAME}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} # this needs to be the griddly project dir!
)

target_include_directories(
  ${GRIDDLY_TEST_BIN_NAME}
  PRIVATE
  ${GRIDDLY_TEST_SRC_DIR}
)
target_link_libraries(
  ${GRIDDLY_TEST_BIN_NAME}
  PRIVATE
  project_warnings
  ${GRIDDLY_LIB_NAME}_static
  CONAN_PKG::gtest
  CONAN_PKG::stb
)
