#include <pybind11/pybind11.h>

namespace py = pybind11;

void bind_types(py::module_& m);
void bind_client(py::module_& m);

PYBIND11_MODULE(_fourier_aurora_sdk, m) {
    bind_types(m);
    bind_client(m);
}
