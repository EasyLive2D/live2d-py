#include <stdio.h>
#include "Python.hpp"
#include "PyLAppModel.hpp"
#include "../../Live2D/V2/Graphics/DrawParamOpenGL.hpp"

// glad GL loader
extern "C" int gladLoadGL();
bool gGLReady = false;

static PyObject* v2cpp_init(PyObject*, PyObject*) { Py_RETURN_NONE; }
static PyObject* v2cpp_glInit(PyObject*, PyObject*) {
    if (!gladLoadGL()) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to initialize OpenGL");
        return nullptr;
    }
    gGLReady = true;
    Py_RETURN_NONE;
}
static PyObject* v2cpp_glRelease(PyObject*, PyObject*) { Py_RETURN_NONE; }
static PyObject* v2cpp_dispose(PyObject*, PyObject*) { Py_RETURN_NONE; }
static PyObject* v2cpp_clearBuffer(PyObject*, PyObject* args) {
    float r=0,g=0,b=0,a=0;
    if (!PyArg_ParseTuple(args, "|ffff", &r,&g,&b,&a)) return nullptr;
    live2d::DrawParamOpenGL::clearBuffer(r, g, b, a);
    Py_RETURN_NONE;
}
static PyObject* v2cpp_enableLog(PyObject*, PyObject* args) { int e; PyArg_ParseTuple(args,"p",&e); Py_RETURN_NONE; }
static PyObject* v2cpp_isLogEnabled(PyObject*, PyObject*) { Py_RETURN_FALSE; }
static PyObject* v2cpp_setLogLevel(PyObject*, PyObject* args) { int l; PyArg_ParseTuple(args,"i",&l); Py_RETURN_NONE; }
static PyObject* v2cpp_getLogLevel(PyObject*, PyObject*) { return PyLong_FromLong(0); }

static PyMethodDef v2cpp_methods[] = {
    {"init", v2cpp_init, METH_VARARGS, ""},
    {"glInit", v2cpp_glInit, METH_VARARGS, ""},
    {"glRelease", v2cpp_glRelease, METH_VARARGS, ""},
    {"dispose", v2cpp_dispose, METH_VARARGS, ""},
    {"clearBuffer", v2cpp_clearBuffer, METH_VARARGS, ""},
    {"enableLog", v2cpp_enableLog, METH_VARARGS, ""},
    {"isLogEnabled", v2cpp_isLogEnabled, METH_VARARGS, ""},
    {"setLogLevel", v2cpp_setLogLevel, METH_VARARGS, ""},
    {"getLogLevel", v2cpp_getLogLevel, METH_VARARGS, ""},
    {NULL, NULL, 0, NULL}
};

static PyModuleDef v2cpp_module = {
    PyModuleDef_HEAD_INIT, "_v2cpp", "Live2D Cubism v2 C++ port", -1, v2cpp_methods
};

PyMODINIT_FUNC PyInit__v2cpp(void) {
    PyObject* m = PyModule_Create(&v2cpp_module);
    if (!m) return nullptr;
    PyType_Spec spec = {
        "_v2cpp.LAppModel", sizeof(PyLAppModelObject), 0,
        Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, PyLAppModel_slots
    };
    PyObject* t = PyType_FromSpec(&spec);
    if (!t) { Py_DECREF(m); return nullptr; }
    PyModule_AddObject(m, "LAppModel", t);

    printf("[live2d.v2cpp] C++ port, Python %s\n", PY_VERSION);
    return m;
}
