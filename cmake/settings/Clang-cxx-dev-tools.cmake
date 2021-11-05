file(GLOB_RECURSE
  ALL_CXX_SOURCE_FILES
  src/*.[ch]pp
  src/*.h
  tests/*.[ch]pp
  tests/*.h
)

# Adding clang-format target if executable is found
message(STATUS "finding clang format")
find_program(CLANG_FORMAT "clang-format")
if(CLANG_FORMAT)
  add_custom_target(
      clang-format
      COMMAND ${CLANG_FORMAT}
      -i
      -style=file
      ${ALL_CXX_SOURCE_FILES}
)
endif()