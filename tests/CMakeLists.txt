cmake_minimum_required(VERSION 3.10.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

# Build the Google Test library
add_subdirectory(vendor)

set(BINARY ${CMAKE_PROJECT_NAME}_Test)

file(GLOB_RECURSE TEST_SOURCES "src/*.cpp")

add_executable(
	${BINARY}
	${TEST_SOURCES}
)
add_test(NAME ${BINARY} COMMAND ${BINARY})

target_include_directories (
    ${BINARY}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)


# Link against Google Test library
target_link_libraries(${BINARY} PUBLIC ${CMAKE_PROJECT_NAME}_lib gmock_main)