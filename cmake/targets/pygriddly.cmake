# the python extension of griddly
set(PYTHON_MODULE pygriddly)

file(GLOB_RECURSE GRIDDLY_PYBINDING_SOURCES ${GRIDDLY_PYBINDING_DIR}/**.cpp)

pybind11_add_module(${PYTHON_MODULE} ${GRIDDLY_PYBINDING_SOURCES})

# Want the python lib to be output in the same directory as the other dll/so
if (MSVC)
    foreach (OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
        set_target_properties(${PYTHON_MODULE}
                PROPERTIES LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${BIN_OUTPUT_DIR})
    endforeach (OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES)
endif ()

set_target_properties(${PYTHON_MODULE}
        PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIR}
        )
target_link_libraries(${PYTHON_MODULE} PRIVATE
        ${GRIDDLY_LIB_NAME}
        )