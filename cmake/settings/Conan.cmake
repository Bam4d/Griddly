macro(run_conan)
    # Download automatically
    if (NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
        message(
                STATUS
                "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
        file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/release/0.17/conan.cmake"
                "${CMAKE_BINARY_DIR}/conan.cmake")
    endif ()

    include(${CMAKE_BINARY_DIR}/conan.cmake)

    find_program(CONAN conan PATHS ${CONAN_PATH})

    if (LINUX)
        file(STRINGS ${GRIDDLY_DEPS_DIR}/conan_extra_options_linux.txt CONAN_EXTRA_OPTIONS)
    endif ()

    conan_cmake_run(
            CONANFILE ${GRIDDLY_DEPS_DIR}/${CONANFILE}
            CONAN_COMMAND ${CONAN}
            ${CONAN_EXTRA_REQUIRES}
            OPTIONS
            ${CONAN_EXTRA_OPTIONS}
            BASIC_SETUP
            CMAKE_TARGETS # individual targets to link to
            KEEP_RPATHS
            BUILD missing
    )
endmacro()
