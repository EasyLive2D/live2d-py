#pragma once
#include "V2/Model.hpp"
#include "Python.hpp"

struct PyLAppModelObject
{
    PyObject_HEAD Live2D::V2::Model* model;
};

extern PyType_Spec PyLAppModel_spec;
extern PyType_Slot PyLAppModel_slots[];
extern PyMethodDef PyLAppModel_methods[];

PyObject* PyLAppModel_new(PyTypeObject* type, PyObject* args, PyObject* kwds);
int PyLAppModel_init(PyLAppModelObject* self, PyObject* args, PyObject* kwds);
void PyLAppModel_dealloc(PyLAppModelObject* self);
