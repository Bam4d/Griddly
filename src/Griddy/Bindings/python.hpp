#include <pybind11/pybind11.h>
#include "../Core/Grid.hpp"

namespace py = pybind11;

std::shared_ptr<Grid> make_grid(int )

PYBIND11_MODULE(griddy, m) {
    m.doc() = "GriddyRTS python bindings";
    m.def("grid", )
}