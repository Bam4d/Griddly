
file(GLOB_RECURSE TEST_SOURCES "${GRIDDLY_TEST_SRC_DIR}/*.cpp")

add_executable(
        ${GRIDDLY_TEST_BIN_NAME}
        ${TEST_SOURCES}
)
add_test(NAME ${GRIDDLY_TEST_BIN_NAME} COMMAND ${GRIDDLY_TEST_BIN_NAME} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..)

#target_include_directories (
#        ${GRIDDLY_TEST_BIN_NAME}
#        PUBLIC
#        ${CMAKE_CURRENT_SOURCE_DIR}/src
#        ${CMAKE_CURRENT_SOURCE_DIR}/../src
#)

target_link_libraries(${GRIDDLY_TEST_BIN_NAME}
        PRIVATE
        ${GRIDDLY_LIB_NAME}
        CONAN_PKG::gtest
        )