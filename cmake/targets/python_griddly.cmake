# the python extension of griddly
set(PYTHON_MODULE python_griddly)

file(GLOB_RECURSE GRIDDLY_PYBINDING_SOURCES ${GRIDDLY_PYBINDING_DIR}/**.cpp)

pybind11_add_module(${PYTHON_MODULE} ${GRIDDLY_PYBINDING_SOURCES})

set_target_properties(
        ${PYTHON_MODULE}
        PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        )
target_link_libraries(${PYTHON_MODULE} PRIVATE
        ${GRIDDLY_LIB_NAME}_static
        )
