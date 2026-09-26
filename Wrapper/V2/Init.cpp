#include <GL/glew.h>

#include "PyLAppModel.hpp"
#include "Python.hpp"
#include <Log.hpp>
#include <stdio.h>

#ifdef DEBUG_ENABLE_CALLSTACK
#include <Debug.hpp>
using namespace Live2D::Common::Debug;
#endif

using namespace Live2D::Common::Log;

#ifdef DEBUG_ENABLE_CALLSTACK
static void GLAPIENTRY glDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                                       GLsizei length, const GLchar* message, const void* userParam)
{
    // 过滤掉通知级别的消息，只关注错误和警告
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
        return;

    fprintf(stderr,
            "[GL ERROR]: %s type = 0x%x, severity = 0x%x, message = %s\n",
            (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
            type,
            severity,
            message);

    // 让程序崩溃，便于调试器捕获
    if (severity == GL_DEBUG_SEVERITY_HIGH) {
        int* a = nullptr;
        a[10] = 1000;
    }
}
#endif


static PyObject* v2cpp_init(PyObject*, PyObject*)
{
    Py_RETURN_NONE;
}
static PyObject* v2cpp_glInit(PyObject*, PyObject*)
{
    if (!gladLoadGL()) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to initialize OpenGL");
        return nullptr;
    }

#ifdef DEBUG_ENABLE_CALLSTACK
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(glDebugCallback, NULL);

    // 先全部关闭
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_FALSE);

    // 只打开 HIGH 严重级别（通常是真正的错误）
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH, 0, NULL, GL_TRUE);
#endif
    Py_RETURN_NONE;
}
static PyObject* v2cpp_glRelease(PyObject*, PyObject*)
{
    Py_RETURN_NONE;
}
static PyObject* v2cpp_dispose(PyObject*, PyObject*)
{
    Py_RETURN_NONE;
}
static PyObject* v2cpp_clearBuffer(PyObject*, PyObject* args)
{
    float r = 0, g = 0, b = 0, a = 0;
    if (!PyArg_ParseTuple(args, "|ffff", &r, &g, &b, &a))
        return nullptr;
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT);
    glClearDepth(1.0);
    Py_RETURN_NONE;
}
static PyObject* v2cpp_enableLog(PyObject*, PyObject* args)
{
    bool e;
    PyArg_ParseTuple(args, "b", &e);
    EnableLive2DLog(e);
    Py_RETURN_NONE;
}
static PyObject* v2cpp_isLogEnabled(PyObject*, PyObject*)
{
    if (IsLive2DLogEnabled()) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}
static PyObject* v2cpp_setLogLevel(PyObject*, PyObject* args)
{
    int l;
    PyArg_ParseTuple(args, "i", &l);
    SetLive2DLogLevel(l);
    Py_RETURN_NONE;
}
static PyObject* v2cpp_getLogLevel(PyObject*, PyObject*)
{
    return PyLong_FromLong(GetLive2DLogLevel());
}

static PyMethodDef v2cpp_methods[] = {{"init", v2cpp_init, METH_VARARGS, ""},
                                      {"glInit", v2cpp_glInit, METH_VARARGS, ""},
                                      {"glRelease", v2cpp_glRelease, METH_VARARGS, ""},
                                      {"dispose", v2cpp_dispose, METH_VARARGS, ""},
                                      {"clearBuffer", v2cpp_clearBuffer, METH_VARARGS, ""},
                                      {"enableLog", v2cpp_enableLog, METH_VARARGS, ""},
                                      {"isLogEnabled", v2cpp_isLogEnabled, METH_VARARGS, ""},
                                      {"setLogLevel", v2cpp_setLogLevel, METH_VARARGS, ""},
                                      {"getLogLevel", v2cpp_getLogLevel, METH_VARARGS, ""},
                                      {NULL, NULL, 0, NULL}};

static PyModuleDef v2cpp_module = {
    PyModuleDef_HEAD_INIT, "_v2cpp", "Live2D Cubism v2 C++ port", -1, v2cpp_methods};

PyMODINIT_FUNC PyInit__v2cpp(void)
{
#ifdef DEBUG_ENABLE_CALLSTACK
    InstallCrashHandler();
#endif
    PyObject* m = PyModule_Create(&v2cpp_module);
    if (!m)
        return nullptr;
    PyType_Spec spec = {"_v2cpp.LAppModel",
                        sizeof(PyLAppModelObject),
                        0,
                        Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
                        PyLAppModel_slots};
    PyObject* t = PyType_FromSpec(&spec);
    if (!t) {
        Py_DECREF(m);
        return nullptr;
    }
    PyModule_AddObject(m, "LAppModel", t);

    printf("[v2cpp] C++ port, Python(%s)\n", PY_VERSION);
    return m;
}
