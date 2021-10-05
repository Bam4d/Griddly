file(GLOB_RECURSE
        ALL_CXX_SOURCE_FILES
        ${GRIDDLY_SRC_DIR}/*.[ch]pp
        ${GRIDDLY_SRC_DIR}/*.h
        ${GRIDDLY_TEST_DIR}/*.[ch]pp
        ${GRIDDLY_TEST_DIR}/*.h
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
