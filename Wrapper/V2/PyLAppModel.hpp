#pragma once
#include "Python.hpp"
#include "../../Live2D/V2/Framework/LAppModel.hpp"

struct PyLAppModelObject {
    PyObject_HEAD
    live2d::LAppModel* model;
};

extern PyType_Spec PyLAppModel_spec;
extern PyType_Slot PyLAppModel_slots[];
extern PyMethodDef PyLAppModel_methods[];

PyObject* PyLAppModel_new(PyTypeObject* type, PyObject* args, PyObject* kwds);
int PyLAppModel_init(PyLAppModelObject* self, PyObject* args, PyObject* kwds);
void PyLAppModel_dealloc(PyLAppModelObject* self);
