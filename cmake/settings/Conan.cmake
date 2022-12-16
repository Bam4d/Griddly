macro(run_conan)
  # Download automatically
  if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
    message(
      STATUS
      "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
    file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/release/0.18/conan.cmake"
      "${CMAKE_BINARY_DIR}/conan.cmake")
  endif()

  include(${CMAKE_BINARY_DIR}/conan.cmake)

  find_program(CONAN conan PATHS ${CONAN_PATH})

  set(CONAN_DISABLE_CHECK_COMPILER on)

  if(MANYLINUX)
    set(BUILD_OPTION all)
  else()
    set(BUILD_OPTION missing)
  endif()

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
    BUILD ${BUILD_OPTION}
  )
endmacro()
