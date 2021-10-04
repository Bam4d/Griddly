file(GLOB_RECURSE GRIDDLY_SOURCES "${GRIDDLY_SRC_DIR}/*.cpp")
file(GLOB_RECURSE GRIDDLY_HEADERS ${GRIDDLY_SRC_DIR}/**.hpp)

set(GRIDDLY_INCLUDE_DIRS "")
foreach (_headerFile ${GRIDDLY_HEADERS})
    get_filename_component(_dir ${_headerFile} PATH)
    list(APPEND GRIDDLY_INCLUDE_DIRS ${_dir})
endforeach ()
list(REMOVE_DUPLICATES GRIDDLY_INCLUDE_DIRS)

# Compile shaders and copy them into resources directory in build output
find_program(glslc_exe glslc PATHS ${CONAN_BIN_DIRS_SHADERC})
set(_shaders_src_dir ${CMAKE_CURRENT_SOURCE_DIR}/src/Griddly/Core/Observers/Vulkan/resources/shaders)
set(_shaders_output_dir ${CMAKE_CURRENT_SOURCE_DIR}/resources/shaders)
add_custom_target(compile_shaders
        COMMAND ${CMAKE_COMMAND} -E make_directory ${_shaders_output_dir}
        COMMAND ${glslc_exe} "${_shaders_src_dir}/triangle.frag" -o "${_shaders_output_dir}/triangle.frag.spv"
        COMMAND ${glslc_exe} "${_shaders_src_dir}/triangle.vert" -o "${_shaders_output_dir}/triangle.vert.spv"
        COMMAND ${glslc_exe} "${_shaders_src_dir}/triangle-textured.frag" -o "${_shaders_output_dir}/triangle-textured.frag.spv"
        COMMAND ${glslc_exe} "${_shaders_src_dir}/triangle-textured.vert" -o "${_shaders_output_dir}/triangle-textured.vert.spv"
        )

# the main Griddly library
add_library(${GRIDDLY_LIB_NAME} STATIC ${GRIDDLY_SOURCES})
add_dependencies(${GRIDDLY_LIB_NAME} compile_shaders)

target_include_directories(
        ${GRIDDLY_LIB_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${GRIDDLY_INCLUDE_DIRS}>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(
        ${GRIDDLY_LIB_NAME}
        PRIVATE
        proj_warnings
        PUBLIC
        proj_options
        CONAN_PKG::vulkan-loader
        CONAN_PKG::yaml-cpp
        CONAN_PKG::glm
        CONAN_PKG::spdlog
        CONAN_PKG::stb
)
