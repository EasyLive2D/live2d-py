#pragma once
#include <LAppModel.hpp>

#include "Python.hpp"

struct PyLAppModelObject
{
    PyObject_HEAD
    LAppModel* model;
    std::string lastExpression;
    time_t expStartedAt;
    time_t fadeout;
};

extern PyType_Spec PyLAppModel_spec;
