#pragma once

#include <Model.hpp>

#include "Python.hpp"

using namespace Live2D::V3;

struct PyModelObject
{
    PyObject_HEAD Model* model;
};

extern PyType_Spec PyModel_Spec;