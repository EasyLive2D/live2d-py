#pragma once

#include <IModel.hpp>

#include "Python.hpp"

struct PyModelObject {
    PyObject_HEAD Live2D::IModel* model;
    PyObject* onStart;
    PyObject* onFinish;
};

extern PyType_Spec PyModel_Spec;
