macro(run_conan)
  if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
    message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
    file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/0.18.1/conan.cmake"
      "${CMAKE_BINARY_DIR}/conan.cmake"
      TLS_VERIFY ON)
  endif()

  include(${CMAKE_BINARY_DIR}/conan.cmake)

  # Create a conan profile for building WASM with Emscripten
  if(WASM)
    set(REQUIREMENTS
      glm/0.9.9.8
      yaml-cpp/0.6.3
      spdlog/1.9.2)

    set(BUILD_PROFILE_HOST emsdk)
  else()
    set(REQUIREMENTS
      gtest/1.11.0
      shaderc/2021.1
      pybind11/2.10.0
      glm/0.9.9.8
      yaml-cpp/0.6.3
      spdlog/1.9.2
      stb/20200203
      volk/1.3.224.1)
    set(BUILD_PROFILE_HOST default)
  endif()

  set(CONAN_DISABLE_CHECK_COMPILER on)

  if(MANYLINUX)
    set(BUILD_COMPILE_OPTION all)
  else()
    set(BUILD_COMPILE_OPTION missing)
  endif()

  conan_cmake_configure(
    REQUIRES
    ${REQUIREMENTS}
    OPTIONS
    gtest:build_gmock=True
    GENERATORS
    cmake_find_package
    cmake_paths
    CMakeToolchain
  )

  conan_cmake_autodetect(settings)

  conan_cmake_install(
    PATH_OR_REFERENCE .
    CONAN_COMMAND ${CONAN}
    ${CONAN_EXTRA_REQUIRES}
    OPTIONS
    ${CONAN_EXTRA_OPTIONS}
    PROFILE_HOST ${BUILD_PROFILE_HOST}
    PROFILE_BUILD default
    BUILD ${BUILD_COMPILE_OPTION}
    SETTINGS ${settings}
  )
endmacro()
