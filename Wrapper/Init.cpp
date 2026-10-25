#include "GraphicsBackend.hpp"
#if defined(CSM_TARGET_WIN_GL) || defined(CSM_TARGET_LINUX_GL) || defined(CSM_TARGET_MAC_GL)
#include <GL/glew.h>
#endif

#include <Init.hpp>
#include <Log.hpp>
#if defined(TRACY_ENABLE)
#include <tracy/Tracy.hpp>
#endif
#ifdef CSM_TARGET_VULKAN
#include <vulkan/vulkan.h>
#include <VulkanManager.hpp>
#include <Rendering/Vulkan/CubismRenderer_Vulkan.hpp>
#endif

#if defined(CSM_TARGET_WIN_GL) || defined(CSM_TARGET_LINUX_GL) || defined(CSM_TARGET_MAC_GL)
#include <Rendering/OpenGL/CubismShader_OpenGLES2.hpp>
#endif
#include <cstdlib>

#ifdef _WIN32
#include <Windows.h>
#endif

#ifdef DEBUG_ENABLE_CALLSTACK
#include <Debug.hpp>
using namespace Live2D::Common::Debug;
#endif

#include "PyModel.hpp"
using namespace Live2D;
using namespace Live2D::Common::Log;

#ifdef DEBUG_ENABLE_CALLSTACK
#if defined(CSM_TARGET_WIN_GL) || defined(CSM_TARGET_LINUX_GL) || defined(CSM_TARGET_MAC_GL)
static void GLAPIENTRY glDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                                       GLsizei length, const GLchar* message,
                                       const void* userParam) {
    // 过滤掉通知级别的消息，只关注错误和警告
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
        return;

    LOGE("[GL] %s type = 0x%x, severity = 0x%x, message = %s\n",
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
#endif


static PyObject* live2d_init_internal(PyObject* self, PyObject* args) {
    const char* path;
    if (PyArg_ParseTuple(args, "s", &path) < 0) {
        PyErr_SetString(PyExc_TypeError, "Invalid params (str)");
        return NULL;
    }

    Live2D::Init(path);
    Py_RETURN_NONE;
}

static PyObject* live2d_dispose() {
    Live2D::Dispose();
    Py_RETURN_NONE;
}

// 后端专属入口的运行时校验。
// Dual 构建下 GL / Vulkan 的编译期宏同时成立，因此不能再用 #if 区分后端，
// 必须按 Common::Backend 的运行时状态判断；不匹配时抛 RuntimeError。
static bool RequireBackend(Live2D::Common::Backend::Type want, const char* funcName) {
    if (Live2D::Common::Backend::GetBackend() != want) {
        const char* wantName =
            want == Live2D::Common::Backend::BACKEND_OPENGL ? "opengl" : "vulkan";
        PyErr_Format(PyExc_RuntimeError, "%s requires %s backend", funcName, wantName);
        return false;
    }
    return true;
}

// 初始化入口互斥：glInit / vkInit 只能成功调用其中一个
static bool RejectIfBackendInitialized(const char* funcName) {
    if (Live2D::Common::Backend::GetBackend() != Live2D::Common::Backend::BACKEND_NONE) {
        PyErr_Format(PyExc_RuntimeError,
                     "%s: graphics backend is already initialized "
                     "(call glRelease/vkDestroy before initializing again)",
                     funcName);
        return true;
    }
    return false;
}


static PyObject* live2d_glInit() {
#if defined(CSM_TARGET_WIN_GL) || defined(CSM_TARGET_LINUX_GL) || defined(CSM_TARGET_MAC_GL)
    if (RejectIfBackendInitialized("glInit")) {
        return NULL;
    }

    if (!gladLoadGL()) {
        PyErr_SetString(PyExc_RuntimeError, "Can't initialize glad");
        return NULL;
    }
    Common::Backend::SetBackend(Live2D::Common::Backend::BACKEND_OPENGL);

    const unsigned char* renderer = glGetString(GL_RENDERER);
    LOGI("Device: %s", renderer);
#ifdef DEBUG_ENABLE_CALLSTACK
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(glDebugCallback, NULL);

    // 先全部关闭
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_FALSE);

    // 只打开 HIGH 严重级别（通常是真正的错误）
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH, 0, NULL, GL_TRUE);
#endif
#else
    PyErr_SetString(PyExc_RuntimeError, "OpenGL backend is not supported on this build");
    return NULL;
#endif
    Py_RETURN_NONE;
}

static PyObject* live2d_glRelease() {
#if defined(CSM_TARGET_WIN_GL) || defined(CSM_TARGET_LINUX_GL) || defined(CSM_TARGET_MAC_GL)
    // 已释放时幂等返回（便于 finally 清理）；Vulkan 会话下调 glRelease 才是错误
    if (Common::Backend::GetBackend() == Live2D::Common::Backend::BACKEND_NONE) {
        Py_RETURN_NONE;
    }
    if (!RequireBackend(Live2D::Common::Backend::BACKEND_OPENGL, "glRelease")) {
        return NULL;
    }
    Csm::Rendering::CubismRenderer::StaticRelease();
    Common::Backend::SetBackend(Live2D::Common::Backend::BACKEND_NONE);
#else
    PyErr_SetString(PyExc_RuntimeError, "OpenGL backend is not supported on this build");
    return NULL;
#endif
    Py_RETURN_NONE;
}

static PyObject* live2d_clear_buffer(PyObject* self, PyObject* args) {
#if defined(CSM_TARGET_WIN_GL) || defined(CSM_TARGET_LINUX_GL) || defined(CSM_TARGET_MAC_GL)
    if (!RequireBackend(Live2D::Common::Backend::BACKEND_OPENGL, "clearBuffer")) {
        return NULL;
    }
    // 默认为黑色
    float r = 0.0, g = 0.0, b = 0.0, a = 0.0;

    // 解析传入的参数，允许指定颜色
    if (!PyArg_ParseTuple(args, "|ffff", &r, &g, &b, &a)) {
        return NULL;
    }

    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT);
    glClearDepth(1.0);
#else
    PyErr_SetString(PyExc_RuntimeError, "OpenGL backend is not supported on this build");
    return NULL;
#endif
    Py_RETURN_NONE;
}

// ---- vkInit / vkDestroy ---------------------------------------------------
// Python 侧始终可见；非 Vulkan 平台构建时抛 RuntimeError。
// Vulkan 构建时 surface 创建由 VulkanManager 内部的平台标准实现
// (vkCreate*Surface) 完成，pyd 不依赖 glfw。
// 实例扩展 (VK_KHR_surface / VK_KHR_*_surface / debug_utils) 全部由
// VulkanManager::GetRequiredExtensions() 在 native 层确定，Python 无需传入。
// vkInit(window_ptr: int, display_ptr: int = 0)
// - window_ptr : 原生窗口句柄 (Windows: HWND)
// - display_ptr: 显示连接 (X11/Wayland 使用，Windows 传 0)
// shader 经 FileUtils 的 LoadFileAsBytes 加载，并被重定向到 SHADER_DIR。
static PyObject* live2d_vk_init(PyObject* /*self*/, PyObject* args) {
#ifdef CSM_TARGET_VULKAN
    if (RejectIfBackendInitialized("vkInit")) {
        return NULL;
    }

    unsigned int backend = -1;
    unsigned long long windowAddr = 0;
    unsigned long long displayAddr = 0;
    unsigned long long connnection = 0;
    if (!PyArg_ParseTuple(args, "IKKK", &backend, &windowAddr, &displayAddr, &connnection)) {
        return NULL;
    }

    SurfaceCreateContext surfaceCtx{};
    surfaceCtx.platform = backend;
    surfaceCtx.window = reinterpret_cast<void*>(static_cast<uintptr_t>(windowAddr));
    surfaceCtx.display = reinterpret_cast<void*>(static_cast<uintptr_t>(displayAddr));
    surfaceCtx.connection = reinterpret_cast<void*>(static_cast<uintptr_t>(connnection));
    
    VulkanManager* vkManager = VulkanManager::GetInstance();
    vkManager->SetSurfaceCreateContext(surfaceCtx);
    // 不调用 SetInstanceExtensions → GetRequiredExtensions() 自动补齐所需扩展
    // 不调用 SetCreateSurfaceFunc → 使用 VulkanManager 内部的平台标准实现
    vkManager->Initialize();
    if (!vkManager->IsInitialized()) {
        PyErr_SetString(PyExc_RuntimeError, "Vulkan initialization failed");
        return NULL;
    }

    // 设置 Framework Vulkan 渲染器的设备常量 (CreateRenderer 前必须)
    SwapchainManager* swapchain = vkManager->GetSwapchainManager();
    Live2D::Cubism::Framework::Rendering::CubismRenderer_Vulkan::SetConstantSettings(
        vkManager->GetDevice(),
        vkManager->GetPhysicalDevice(),
        vkManager->GetCommandPool(),
        vkManager->GetGraphicQueue(),
        swapchain->GetImageCount(),
        swapchain->GetExtent(),
        vkManager->GetSwapchainImageView(),
        swapchain->GetSwapchainImageFormat(),
        vkManager->GetDepthFormat());
    Common::Backend::SetBackend(Live2D::Common::Backend::BACKEND_VULKAN);
#else
    PyErr_SetString(PyExc_RuntimeError, "Vulkan backend is not supported on this platform");
    return NULL;
#endif
    Py_RETURN_NONE;
}

static PyObject* live2d_vk_destroy(PyObject* /*self*/, PyObject* /*args*/) {
#ifdef CSM_TARGET_VULKAN
    // 已释放时幂等返回（便于 finally 清理）；OpenGL 会话下调 vkDestroy 才是错误
    if (Common::Backend::GetBackend() == Live2D::Common::Backend::BACKEND_NONE) {
        Py_RETURN_NONE;
    }
    if (!RequireBackend(Live2D::Common::Backend::BACKEND_VULKAN, "vkDestroy")) {
        return NULL;
    }
    VulkanManager::GetInstance()->Destroy();
    Common::Backend::SetBackend(Live2D::Common::Backend::BACKEND_NONE);
#else
    PyErr_SetString(PyExc_RuntimeError, "Vulkan backend is not supported on this platform");
    return NULL;
#endif
    Py_RETURN_NONE;
}

// ---- 帧解耦接口 ------------------------------------------------------------
// 模型 Draw 仅录制命令；帧流程（acquire/clear/present）由 Python 每帧驱动：
//   vkGetHandles() -> {"physical_device", "device", "color_format",
//                      "frames_in_flight"}（vkInit 之后调用）
//   vkBeginFrame() -> (cmd, frame_index, width, height) 或 None（交换链失效，
//                      本帧应跳过；cmd 为帧命令缓冲指针，区间已打开并清屏）
//   vkEndFrame()   -> None（结束区间 → 提交等待 imageAvailable → present）
//   vkSubmitOnce(fn) -> 在 rendering 区间外的一次性命令缓冲上执行 fn(cmd: int)，
//                       提交并等待完成（纹理上传等一次性 GPU 操作）
static PyObject* live2d_vk_get_handles(PyObject* /*self*/, PyObject* /*args*/) {
#ifdef CSM_TARGET_VULKAN
    if (!RequireBackend(Live2D::Common::Backend::BACKEND_VULKAN, "vkGetHandles")) {
        return NULL;
    }
    VulkanManager* vkManager = VulkanManager::GetInstance();
    if (!vkManager->IsInitialized()) {
        PyErr_SetString(PyExc_RuntimeError, "Vulkan is not initialized (call vkInit first)");
        return NULL;
    }
    SwapchainManager* swapchain = vkManager->GetSwapchainManager();
    return Py_BuildValue("{s:K,s:K,s:i,s:I}",
                         "physical_device",
                         (unsigned long long)vkManager->GetPhysicalDevice(),
                         "device",
                         (unsigned long long)vkManager->GetDevice(),
                         "color_format",
                         (int)swapchain->GetSwapchainImageFormat(),
                         "frames_in_flight",
                         (unsigned int)vkManager->GetFramesInFlight());
#else
    PyErr_SetString(PyExc_RuntimeError, "Vulkan backend is not supported on this platform");
    return NULL;
#endif
}

static PyObject* live2d_vk_begin_frame(PyObject* /*self*/, PyObject* /*args*/) {
#if defined(TRACY_ENABLE)
    ZoneScopedN("py/vkBeginFrame");
#endif
#ifdef CSM_TARGET_VULKAN
    if (!RequireBackend(Live2D::Common::Backend::BACKEND_VULKAN, "vkBeginFrame")) {
        return NULL;
    }
    VulkanManager* vkManager = VulkanManager::GetInstance();
    if (!vkManager->BeginFrame()) {
        Py_RETURN_NONE;   // 交换链失效/重建，本帧跳过
    }
    VkExtent2D extent = vkManager->GetSwapchainManager()->GetExtent();
    return Py_BuildValue("(KIII)",
                         (unsigned long long)vkManager->GetFrameCommandBuffer(),
                         (unsigned int)vkManager->GetCurrentImageIndex(),
                         (unsigned int)extent.width,
                         (unsigned int)extent.height);
#else
    PyErr_SetString(PyExc_RuntimeError, "Vulkan backend is not supported on this platform");
    return NULL;
#endif
}

static PyObject* live2d_vk_end_frame(PyObject* /*self*/, PyObject* /*args*/) {
#if defined(TRACY_ENABLE)
    ZoneScopedN("py/vkEndFrame");
#endif
#ifdef CSM_TARGET_VULKAN
    if (!RequireBackend(Live2D::Common::Backend::BACKEND_VULKAN, "vkEndFrame")) {
        return NULL;
    }
    VulkanManager::GetInstance()->EndFrame();
#else
    PyErr_SetString(PyExc_RuntimeError, "Vulkan backend is not supported on this platform");
    return NULL;
#endif
    Py_RETURN_NONE;
}

static PyObject* live2d_vk_submit_once(PyObject* /*self*/, PyObject* args) {
#ifdef CSM_TARGET_VULKAN
    if (!RequireBackend(Live2D::Common::Backend::BACKEND_VULKAN, "vkSubmitOnce")) {
        return NULL;
    }
    PyObject* callback = NULL;
    if (!PyArg_ParseTuple(args, "O", &callback) || !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "expected a callable");
        return NULL;
    }
    VulkanManager* vkManager = VulkanManager::GetInstance();
    if (!vkManager->IsInitialized()) {
        PyErr_SetString(PyExc_RuntimeError, "Vulkan is not initialized (call vkInit first)");
        return NULL;
    }

    VkCommandBuffer cmd = vkManager->BeginSingleTimeCommands();
    PyObject* cmdObj = PyLong_FromUnsignedLongLong((unsigned long long)cmd);
    if (!cmdObj) {
        return NULL;
    }
    PyObject* ret = PyObject_CallFunctionObjArgs(callback, cmdObj, NULL);
    Py_DECREF(cmdObj);
    if (!ret) {
        return NULL;   // 回调抛出的异常向上传递
    }
    Py_DECREF(ret);

    vkManager->SubmitCommand(cmd);
#else
    PyErr_SetString(PyExc_RuntimeError, "Vulkan backend is not supported on this platform");
    return NULL;
#endif
    Py_RETURN_NONE;
}

static PyObject* live2d_enable_log(PyObject* self, PyObject* args) {
    bool enable;
    if (!PyArg_ParseTuple(args, "b", &enable)) {
        PyErr_SetString(PyExc_TypeError, "invalid param");
        return NULL;
    }

    EnableLive2DLog(enable);

    Py_RETURN_NONE;
}

static PyObject* live2d_is_log_enabled(PyObject* self, PyObject* args) {
    if (IsLive2DLogEnabled())
        Py_RETURN_TRUE;
    Py_RETURN_FALSE;
}

static PyObject* live2d_set_log_level(PyObject* self, PyObject* args) {
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

static PyObject* live2d_get_log_level(PyObject* self, PyObject* args) {
    return Py_BuildValue("i", GetLive2DLogLevel());
}

static PyObject* live2d_supported_backends(PyObject* self, PyObject* args) {
#if defined(CSM_TARGET_VULKAN) && (!defined(CSM_TARGET_WIN_GL) && !defined(CSM_TARGET_LINUX_GL) && !defined(CSM_TARGET_MAC_GL))
    return Py_BuildValue("s", "Vulkan");
#endif
#if !defined(CSM_TARGET_VULKAN) && (defined(CSM_TARGET_WIN_GL) || defined(CSM_TARGET_LINUX_GL) || defined(CSM_TARGET_MAC_GL))
    return Py_BuildValue("s", "OpenGL");
#endif
#if (defined(CSM_TARGET_WIN_GL) || defined(CSM_TARGET_LINUX_GL) || defined(CSM_TARGET_MAC_GL)) && defined(CSM_TARGET_VULKAN)
    return Py_BuildValue("s", "Vulkan | OpenGL");
#endif
}

// 定义live2d模块的方法
static PyMethodDef live2d_methods[] = {
    {"init_internal", (PyCFunction)live2d_init_internal, METH_VARARGS, ""},
    {"dispose", (PyCFunction)live2d_dispose, METH_VARARGS, ""},
    {"glInit", (PyCFunction)live2d_glInit, METH_VARARGS, ""},
    {"glRelease", (PyCFunction)live2d_glRelease, METH_VARARGS, ""},
    {"clearBuffer", (PyCFunction)live2d_clear_buffer, METH_VARARGS, ""},
    {"vkInit", (PyCFunction)live2d_vk_init, METH_VARARGS, ""},
    {"vkDestroy", (PyCFunction)live2d_vk_destroy, METH_VARARGS, ""},
    {"vkGetHandles", (PyCFunction)live2d_vk_get_handles, METH_NOARGS, ""},
    {"vkBeginFrame", (PyCFunction)live2d_vk_begin_frame, METH_NOARGS, ""},
    {"vkEndFrame", (PyCFunction)live2d_vk_end_frame, METH_NOARGS, ""},
    {"vkSubmitOnce", (PyCFunction)live2d_vk_submit_once, METH_VARARGS, ""},
    {"enableLog", (PyCFunction)live2d_enable_log, METH_VARARGS, ""},
    {"isLogEnabled", (PyCFunction)live2d_is_log_enabled, METH_VARARGS, ""},
    {"setLogLevel", (PyCFunction)live2d_set_log_level, METH_VARARGS, ""},
    {"getLogLevel", (PyCFunction)live2d_get_log_level, METH_VARARGS, ""},
    {"supportedBackends", (PyCFunction)live2d_supported_backends, METH_VARARGS, ""},
    {NULL, NULL, 0, NULL}};

// 定义live2d模块
static PyModuleDef liv2d_module = {
    PyModuleDef_HEAD_INIT, "live2d", "Module that creates live2d objects", -1, live2d_methods};

// 模块初始化函数的实现
PyMODINIT_FUNC PyInit__live2d(void) {
#ifdef DEBUG_ENABLE_CALLSTACK
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

#ifdef _WIN32
    SetConsoleOutputCP(65001);
#endif
    return m;
}
