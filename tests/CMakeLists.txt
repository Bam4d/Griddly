cmake_minimum_required(VERSION 3.10.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

# Build the Google Test library
add_subdirectory(vendor)

file(GLOB_RECURSE test_SOURCES "src/*.cpp")

add_executable(
	tests
	${test_SOURCES}
)
add_test(NAME check COMMAND tests)

target_include_directories (
    tests
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

# Link against Google Test library
target_link_libraries(tests gmock_main)