#pragma once

#include <fine-grained/Model.hpp>

#include "Python.hpp"

struct PyModelObject {
    PyObject_HEAD
    Model* model;
};

extern PyType_Spec PyModel_Spec;