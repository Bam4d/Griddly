macro(run_conan)
    # Download automatically
    # if (NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
    #     message(
    #             STATUS
    #             "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
    #     file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/0.18.1/conan.cmake"
    #             "${CMAKE_BINARY_DIR}/conan.cmake")
    # endif ()

    # include(${CMAKE_BINARY_DIR}/conan.cmake)
    include(${CMAKE_CONFIG_FOLDER}/settings/conan019.cmake)

    find_program(CONAN conan PATHS ${CONAN_PATH})

    # Create a conan profile for building WASM with Emscripten
    if(WASM) 

        conan_cmake_configure(
            REQUIRES 
                glm/0.9.9.8
                yaml-cpp/0.6.3
                spdlog/1.9.2
            OPTIONS
                gtest:build_gmock=True
            GENERATORS
                cmake
                cmake_find_package
                cmake_paths
        )

        conan_cmake_install(
            PATH_OR_REFERENCE .
            CONAN_COMMAND ${CONAN}
            ${CONAN_EXTRA_REQUIRES}
            OPTIONS
            ${CONAN_EXTRA_OPTIONS}
            PROFILE_HOST emsdk
            PROFILE_BUILD default
            BUILD missing
        )

        include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
        conan_basic_setup(TARGETS)

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
