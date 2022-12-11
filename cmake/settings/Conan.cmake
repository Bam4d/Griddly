macro(run_conan)
    # Download automatically
    if (NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
        message(
                STATUS
                "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
        file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/0.18.1/conan.cmake"
                "${CMAKE_BINARY_DIR}/conan.cmake")
    endif ()

    include(${CMAKE_BINARY_DIR}/conan.cmake)

    find_program(CONAN conan PATHS ${CONAN_PATH})

    # Create a conan profile for building WASM with Emscripten
    if(WASM) 
        conan_cmake_run(
            CONANFILE ${GRIDDLY_DEPS_DIR}/${CONANFILE_WASM}
            CONAN_COMMAND ${CONAN}
            ${CONAN_EXTRA_REQUIRES}
            OPTIONS
            ${CONAN_EXTRA_OPTIONS}
            BASIC_SETUP
            PROFILE emsdk
            PROFILE_BUILD
            NO_OUTPUT_DIRS
            CMAKE_TARGETS # individual targets to link to
            KEEP_RPATHS
            BUILD missing
        )

    else()
        conan_cmake_run(
                CONANFILE ${GRIDDLY_DEPS_DIR}/${CONANFILE}
                CONAN_COMMAND ${CONAN}
                ${CONAN_EXTRA_REQUIRES}
                OPTIONS
                ${CONAN_EXTRA_OPTIONS}
                BASIC_SETUP
                NO_OUTPUT_DIRS
                CMAKE_TARGETS # individual targets to link to
                KEEP_RPATHS
                BUILD missing
        )

    endif ()
    

    
endmacro()
