#include <GL/glew.h>

#include <CubismFramework.hpp>
#include <LAppAllocator.hpp>
#include <LAppPal.hpp>
#include <Log.hpp>
#include <Rendering/OpenGL/CubismShader_OpenGLES2.hpp>
#include <cstdlib>

#ifdef WIN32
#include <Windows.h>
#endif

#ifdef DEBUG_ENABLE_CALL_STACK
#include <Debug.hpp>
using namespace Live2D::Common::Debug;
#endif

#include "PyModel.hpp"

using namespace Live2D::Common::Log;

#ifdef DEBUG_ENABLE_CALL_STACK
static void GLAPIENTRY glDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                                       GLsizei length, const GLchar* message, const void* userParam)
{
    // 过滤掉通知级别的消息，只关注错误和警告
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
        return;

    LOGE("[GL ERROR]: %s type = 0x%x, severity = 0x%x, message = %s\n",
          (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
          type,
          severity,
          message);

    // 让程序崩溃，便于调试器捕获
    if (severity == GL_DEBUG_SEVERITY_HIGH) {
        char* arr = nullptr;
        arr[10] = 10;
    }
}
#endif

static LAppAllocator _cubismAllocator;
static Csm::CubismFramework::Option _cubismOption;

static PyObject* live2d_init_internal(PyObject* self, PyObject* args)
{
    const char* path;
    if (PyArg_ParseTuple(args, "s", &path) < 0) {
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

static PyObject* live2d_dispose()
{
    Csm::CubismFramework::Dispose();
    Py_RETURN_NONE;
}

static PyObject* live2d_glInit()
{
    if (!gladLoadGL()) {
        LOGE("Can't initilize glad.");
    }

#ifdef DEBUG_ENABLE_CALL_STACK
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

static PyObject* live2d_glRelease()
{
    Csm::Rendering::CubismRenderer::StaticRelease();
    Py_RETURN_NONE;
}

static PyObject* live2d_clear_buffer(PyObject* self, PyObject* args)
{
    // 默认为黑色
    float r = 0.0, g = 0.0, b = 0.0, a = 0.0;

    // 解析传入的参数，允许指定颜色
    if (!PyArg_ParseTuple(args, "|ffff", &r, &g, &b, &a)) {
        return NULL;
    }

    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT);
    glClearDepth(1.0);

    Py_RETURN_NONE;
}

static PyObject* live2d_enable_log(PyObject* self, PyObject* args)
{
    bool enable;
    if (!PyArg_ParseTuple(args, "b", &enable)) {
        PyErr_SetString(PyExc_TypeError, "invalid param");
        return NULL;
    }

    EnableLive2DLog(enable);

    Py_RETURN_NONE;
}

static PyObject* live2d_is_log_enabled(PyObject* self, PyObject* args)
{
    if (IsLive2DLogEnabled())
        Py_RETURN_TRUE;
    Py_RETURN_FALSE;
}

static PyObject* live2d_set_log_level(PyObject* self, PyObject* args)
{
    int level;
    if (!PyArg_ParseTuple(args, "i", &level)) {
        PyErr_SetString(PyExc_TypeError, "invalid param");
        return NULL;
    }

    SetLive2DLogLevel(level);

    if (IsLive2DLogEnabled()) {
        switch (GetLive2DLogLevel()) {
        case LV_DEBUG:
            LOGD("[Log] Level=DEBUG");
            break;
        case LV_INFO:
            LOGI("[Log] Level=INFO");
            break;
        case LV_WARN:
            LOGW("[Log] Level=WARN");
            break;
        case LV_ERROR:
            LOGE("[Log] Level=Error");
            break;
        default:
            break;
        }
    }

    Py_RETURN_NONE;
}

static PyObject* live2d_get_log_level(PyObject* self, PyObject* args)
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
    PyModuleDef_HEAD_INIT, "live2d", "Module that creates live2d objects", -1, live2d_methods};

// 模块初始化函数的实现
PyMODINIT_FUNC PyInit__v3cpp(void)
{
#ifdef DEBUG_ENABLE_CALL_STACK
    InstallCrashHandler();
#endif
    PyObject* m = PyModule_Create(&liv2d_module);
    if (!m) {
        return NULL;
    }

    if (PyModule_AddObject(m, "Model", PyType_FromSpec(&PyModel_Spec)) < 0) {
        Py_DECREF(m);
        return NULL;
    }

#ifdef CSM_TARGET_WIN_GL
    SetConsoleOutputCP(65001);
#endif

    printf("[v3] CubismNativeSDK(%s), Python(%s)\n", CUBISM_NATIVE_SDK_VERSION, PY_VERSION);
    printf("[v3] official: https://www.live2d.com/sdk/download/native/\n");
    return m;
}
