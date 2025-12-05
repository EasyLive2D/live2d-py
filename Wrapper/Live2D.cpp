#include <GL/glew.h>

#include <CubismFramework.hpp>
#include <LAppPal.hpp>
#include <LAppAllocator.hpp>
#include <Log.hpp>
#include <Rendering/OpenGL/CubismShader_OpenGLES2.hpp>

#ifdef WIN32
#include <Windows.h>
#endif

#include "PyLAppModel.hpp"
#include "PyModel.hpp"

static LAppAllocator _cubismAllocator;
static Csm::CubismFramework::Option _cubismOption;

static PyObject *live2d_init_internal(PyObject *self, PyObject *args)
{
    const char *path;
    if (PyArg_ParseTuple(args, "s", &path) < 0)
    {
        PyErr_SetString(PyExc_TypeError, "Invalid params (str)");
        return NULL;
    }

    LAppPal::InitShaderDir(path);
    _cubismOption.LogFunction = LAppPal::PrintLn;
    _cubismOption.LoggingLevel = Csm::CubismFramework::Option::LogLevel_Verbose;
    _cubismOption.LoadFileFunction = LAppPal::LoadFileAsBytes;
    _cubismOption.ReleaseBytesFunction = LAppPal::ReleaseBytes;

    Csm::CubismFramework::StartUp(&_cubismAllocator, &_cubismOption);
    Csm::CubismFramework::Initialize();
    Py_RETURN_NONE;
}

static PyObject *live2d_dispose()
{
    Csm::CubismFramework::Dispose();
    Py_RETURN_NONE;
}

static PyObject *live2d_glInit()
{
    if (!gladLoadGL())
    {
        Error("Can't initilize glad.");
    }
    Py_RETURN_NONE;
}

static PyObject *live2d_glRelease()
{
    Csm::Rendering::CubismRenderer::StaticRelease();
    Py_RETURN_NONE;
}

static PyObject *live2d_clear_buffer(PyObject *self, PyObject *args)
{
    // 默认为黑色
    float r = 0.0, g = 0.0, b = 0.0, a = 0.0;

    // 解析传入的参数，允许指定颜色
    if (!PyArg_ParseTuple(args, "|ffff", &r, &g, &b, &a))
    {
        return NULL;
    }

    // 设置清屏颜色
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearDepth(1.0);

    Py_RETURN_NONE;
}

static PyObject *live2d_enable_log(PyObject *self, PyObject *args)
{
    bool enable;
    if (!PyArg_ParseTuple(args, "b", &enable))
    {
        PyErr_SetString(PyExc_TypeError, "invalid param");
        return NULL;
    }

    EnableLive2DLog(enable);

    Py_RETURN_NONE;
}

static PyObject *live2d_is_log_enabled(PyObject *self, PyObject *args)
{
    if (IsLive2DLogEnabled())
        Py_RETURN_TRUE;
    Py_RETURN_FALSE;
}

static PyObject *live2d_set_log_level(PyObject *self, PyObject *args)
{
    int level;
    if (!PyArg_ParseTuple(args, "i", &level))
    {
        PyErr_SetString(PyExc_TypeError, "invalid param");
        return NULL;
    }

    SetLive2DLogLevel(level);

    if (IsLive2DLogEnabled())
    {
        switch (GetLive2DLogLevel())
        {
        case LV_DEBUG:
            Debug("[Log] Level=DEBUG");
            break;
        case LV_INFO:
            Info("[Log] Level=INFO");
            break;
        case LV_WARN:
            Warn("[Log] Level=WARN");
            break;
        case LV_ERROR:
            Error("[Log] Level=Error");
            break;
        default:
            break;
        }
    }

    Py_RETURN_NONE;
}

static PyObject *live2d_get_log_level(PyObject *self, PyObject *args)
{
    return Py_BuildValue("i", GetLive2DLogLevel());
}

// 定义live2d模块的方法
static PyMethodDef live2d_methods[] = {
    {"init_internal", (PyCFunction)live2d_init_internal, METH_VARARGS, ""},
    {"dispose", (PyCFunction)live2d_dispose, METH_VARARGS, ""},
    {"glInit", (PyCFunction)live2d_glInit, METH_VARARGS, ""},
    {"glRelease", (PyCFunction)live2d_glRelease, METH_VARARGS, ""},
    {"clearBuffer", (PyCFunction)live2d_clear_buffer, METH_VARARGS, ""},
    {"enableLog", (PyCFunction)live2d_enable_log, METH_VARARGS, ""},
    {"isLogEnabled", (PyCFunction)live2d_is_log_enabled, METH_VARARGS, ""},
    {"setLogLevel", (PyCFunction)live2d_set_log_level, METH_VARARGS, ""},
    {"getLogLevel", (PyCFunction)live2d_get_log_level, METH_VARARGS, ""},
    {NULL, NULL, 0, NULL}};

// 定义live2d模块
static PyModuleDef liv2d_module = {
    PyModuleDef_HEAD_INIT,
    "live2d",
    "Module that creates live2d objects",
    -1,
    live2d_methods};

PyObject *module_live2d_v3_params = nullptr;
PyObject *typeobject_live2d_v3_parameter = nullptr;

// 模块初始化函数的实现
PyMODINIT_FUNC PyInit_live2d(void)
{
    PyObject *lappmodel_type;
    PyObject *model_type;

    PyObject *m = PyModule_Create(&liv2d_module);
    if (!m)
    {
        return NULL;
    }

    lappmodel_type = PyType_FromSpec(&PyLAppModel_spec);
    if (!lappmodel_type)
    {
        return NULL;
    }

    if (PyModule_AddObject(m, "LAppModel", lappmodel_type) < 0)
    {
        Py_DECREF(&lappmodel_type);
        Py_DECREF(m);
        return NULL;
    }

    if (PyModule_AddObject(m, "Model", PyType_FromSpec(&PyModel_Spec)) < 0)
    {
        Py_DECREF(&lappmodel_type);
        Py_DECREF(m);
        return NULL;
    }

    // assume that module `params` is already imported in `live2d/v3/__init__.py`
    module_live2d_v3_params = PyImport_AddModule("live2d.v3.params");
    if (module_live2d_v3_params == NULL)
    {
        PyErr_Print();
        return NULL;
    }

    typeobject_live2d_v3_parameter = PyObject_GetAttrString(module_live2d_v3_params, "Parameter");
    if (typeobject_live2d_v3_parameter == NULL)
    {
        Py_DECREF(module_live2d_v3_params);
        PyErr_Print();
        return NULL;
    }

#ifdef CSM_TARGET_WIN_GL
    // windows 下强制utf-8
    SetConsoleOutputCP(65001);
#endif

    printf("─────────────────────────────────────────────────────────────\n");
    printf(" 🐾 live2d-py (built with Python %s)\n", PY_VERSION);
    printf(" 🧸 3rd-party lib integrated with official Live2D native SDK \n");
    printf(" ✨ Official SDK: https://www.live2d.com/sdk/download/native/\n");
    printf("─────────────────────────────────────────────────────────────\n");
    return m;
}
