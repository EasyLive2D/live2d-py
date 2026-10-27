#include "AppRun.hpp"

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>

#include "GraphicsBackend.hpp"
#include <IModel.hpp>
#include <Log.hpp>

// GLFW_INCLUDE_NONE — glad (GL/glew.h) provides all GL symbols
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#ifdef CSM_TARGET_VULKAN
#include <vulkan/vulkan.h>
#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#elif __linux__
#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_WAYLAND
#endif
#include <GLFW/glfw3native.h>
#include <Rendering/Vulkan/CubismRenderer_Vulkan.hpp>
#include <VulkanManager.hpp>
#endif

#if defined(CSM_TARGET_WIN_GL) || defined(CSM_TARGET_LINUX_GL) || defined(CSM_TARGET_MAC_GL)
#include <GL/glew.h>   // glad OpenGL 4.6 core loader
#endif

using namespace Live2D::Common::Log;

namespace {

int winW = 800;
int winH = 800;
Live2D::IModel* sModelForResize = nullptr;

void glfwErrorCallback(int /*code*/, const char* msg) {
    fprintf(stderr, "[GLFW ERROR] %s\n", msg);
}

void keyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void windowSizeCallback(GLFWwindow* /*win*/, int width, int height) {
    if (!sModelForResize)
        return;
    winW = width;
    winH = height;
#if defined(CSM_TARGET_WIN_GL) || defined(CSM_TARGET_LINUX_GL) || defined(CSM_TARGET_MAC_GL)
    if (Live2D::Common::Backend::IsOpenGL()) {
        glViewport(0, 0, width, height);
    }
#endif
    sModelForResize->Resize(width, height);
}

#ifdef CSM_TARGET_VULKAN
// device / swapchain / Framework 渲染器常量（CreateRenderer 前必须）
bool InitVulkan(GLFWwindow* window) {
    VulkanManager* vkManager = VulkanManager::GetInstance();
    SurfaceCreateContext surfaceCtx{};

#ifdef _WIN32
    // 获取原生窗口句柄 (HWND) 并传给 VulkanManager。
    // surface 创建由 VulkanManager 内部的 vkCreateWin32SurfaceKHR 完成。
    surfaceCtx.platform = PLATFORM_WINDOWS;
    surfaceCtx.window = glfwGetWin32Window(window);
#elif defined(__linux__)
    // 运行时判断 GLFW 当前使用的后端，填充对应的原生句柄。
    // surface 创建由 VulkanManager 内部根据 platform 分发到
    // vkCreateXlibSurfaceKHR 或 vkCreateWaylandSurfaceKHR。
    const auto platform = glfwGetPlatform();
    if (glfwGetPlatform() == GLFW_PLATFORM_WAYLAND) {
        surfaceCtx.platform = PLATFORM_WAYLAND;
        surfaceCtx.display = glfwGetWaylandDisplay();
        surfaceCtx.window = glfwGetWaylandWindow(window);
        LOGI("platform: wayland");
    } else if (platform == GLFW_PLATFORM_X11) {
        // GLFW 的 X11 后端，句柄是 Xlib 类型
        surfaceCtx.platform = PLATFORM_XLIB;
        surfaceCtx.display = glfwGetX11Display();
        surfaceCtx.window = (void*)(uintptr_t)glfwGetX11Window(window);
        LOGI("platform: x11");
    } else {
        LOGF("unsupported platform 0x%x", platform);
    }
#endif

    vkManager->SetSurfaceCreateContext(surfaceCtx);

    // 不调用 SetInstanceExtensions → GetRequiredExtensions() 自动补齐所需扩展
    // 不调用 SetCreateSurfaceFunc → 使用平台标准实现
    vkManager->Initialize();
    if (!vkManager->IsInitialized())
        return false;

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
    return true;
}
#endif

#if defined(CSM_TARGET_WIN_GL) || defined(CSM_TARGET_LINUX_GL) || defined(CSM_TARGET_MAC_GL)
// 创建 GL 上下文并加载 glad；v2 着色器是 GLSL 1.20（attribute/varying），
// 因此使用 3.3 Compatibility profile。
bool InitOpenGL(GLFWwindow* window) {
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGL())
        return false;
    printf(
        "OpenGL %s, GLSL %s\n", glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));
    return true;
}
#endif

// 模型信息打印（加载后、进入循环前）
void PrintModelInfo(Live2D::IModel& model) {
    float cw, ch;
    model.GetCanvasSize(cw, ch);
    printf("Model loaded successfully! (%s / %s)\n",
           model.IsV2() ? "v2" : "v3",
           Live2D::Common::Backend::IsVulkan() ? "Vulkan" : "OpenGL");
    printf("  Canvas: %.0f x %.0f\n", cw, ch);
    printf("  Parameters: %d\n", model.GetParameterCount());
    printf("  Parts: %d\n", model.GetPartCount());

    int nParams = model.GetParameterCount();
    int showParams = nParams < 15 ? nParams : 15;
    printf("  First %d parameters:\n", showParams);
    for (int i = 0; i < showParams; i++) {
        printf("    [%d] %-31s  val=%.3f  min=%.2f  max=%.2f  def=%.2f\n",
               i,
               model.GetParameterId(i),
               model.GetParameterValue(i),
               model.GetParameterMinimumValue(i),
               model.GetParameterMaximumValue(i),
               model.GetParameterDefaultValue(i));
    }
    if (nParams > 15)
        printf("    ... (%d more)\n", nParams - 15);

    int nParts = model.GetPartCount();
    printf("  Parts:\n");
    for (int i = 0; i < nParts; i++) {
        printf("    [%d] %s\n", i, model.GetPartId(i));
    }
}

// 单帧：update → draw → present → 事件
void RenderFrame(GLFWwindow* window, Live2D::IModel& model) {
    model.Update();
#if defined(CSM_TARGET_WIN_GL) || defined(CSM_TARGET_LINUX_GL) || defined(CSM_TARGET_MAC_GL)
    if (Live2D::Common::Backend::IsOpenGL()) {
        glViewport(0, 0, winW, winH);
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        model.Draw();
    }
#endif
#ifdef CSM_TARGET_VULKAN
    if (Live2D::Common::Backend::IsVulkan()) {
        // 帧解耦：帧流程由 BeginFrame/EndFrame 驱动，Draw 仅录制命令
        if (VulkanManager::GetInstance()->BeginFrame()) {
            model.Draw();
            VulkanManager::GetInstance()->EndFrame();
        }
    }
#endif
#if defined(CSM_TARGET_WIN_GL) || defined(CSM_TARGET_LINUX_GL) || defined(CSM_TARGET_MAC_GL)
    if (Live2D::Common::Backend::IsOpenGL()) {
        glfwSwapBuffers(window);
    }
#endif
    glfwPollEvents();
    if (model.IsMotionFinished()) {
        // model.StartRandomMotion(3);
    }
}

}   // namespace

int RunInteractive(const std::string& modelPath) {
#if defined(__linux__) && defined(CSM_TARGET_VULKAN)   // temp for wsl
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
#endif
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) {
        fprintf(stderr, "ERROR: glfwInit() failed\n");
        return 1;
    }
#ifdef CSM_TARGET_VULKAN
    if (Live2D::Common::Backend::IsVulkan()) {
        // No GL context — rendering goes through Vulkan
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    }
#endif

#if defined(CSM_TARGET_WIN_GL) || defined(CSM_TARGET_LINUX_GL) || defined(CSM_TARGET_MAC_GL)
    if (Live2D::Common::Backend::IsOpenGL()) {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
    }
#endif
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    const char* backendName = Live2D::Common::Backend::IsVulkan() ? "Vulkan" : "OpenGL";
    const std::string winTitleBase = std::string("Live2D Test — ") + backendName + " — " +
                                     std::filesystem::u8path(modelPath).filename().string();
    GLFWwindow* window = glfwCreateWindow(winW, winH, winTitleBase.c_str(), nullptr, nullptr);
    if (!window) {
        fprintf(stderr, "ERROR: glfwCreateWindow() failed\n");
        glfwTerminate();
        return 1;
    }
    glfwSetKeyCallback(window, keyCallback);
    glfwSetWindowSizeCallback(window, windowSizeCallback);

#ifdef CSM_TARGET_VULKAN
    if (Live2D::Common::Backend::IsVulkan() && !InitVulkan(window)) {
        fprintf(stderr, "ERROR: Vulkan initialization failed\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
#endif

#if defined(CSM_TARGET_WIN_GL) || defined(CSM_TARGET_LINUX_GL) || defined(CSM_TARGET_MAC_GL)
    if (Live2D::Common::Backend::IsOpenGL() && !InitOpenGL(window)) {
        fprintf(stderr, "ERROR: gladLoadGL() failed\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
#endif

    EnableLive2DLog(true);
    SetLive2DLogLevel(LV_INFO);

    int exitCode = 0;
    {
        std::unique_ptr<Live2D::IModel> model(Live2D::IModel::Create(modelPath.c_str()));
        if (!model) {
            fprintf(stderr, "ERROR: cannot detect model version: %s\n", modelPath.c_str());
            glfwDestroyWindow(window);
            glfwTerminate();
            return 1;
        }
        sModelForResize = model.get();
        model->LoadModelJson(modelPath.c_str());

        PrintModelInfo(*model);

        model->Resize(winW, winH);
        model->StartRandomMotion("", 3);
        printf("\nStarted random idle motion.\n");

        printf("Running... (press ESC to exit)\n");
        int frameCount = 0;
        int fpsFrames = 0;
        auto fpsWindowStart = std::chrono::steady_clock::now();
        while (!glfwWindowShouldClose(window)) {
            RenderFrame(window, *model);
            frameCount++;
            fpsFrames++;

            // 每 0.5s 刷新一次标题栏帧率
            const auto now = std::chrono::steady_clock::now();
            const float fpsElapsed = std::chrono::duration<float>(now - fpsWindowStart).count();
            if (fpsElapsed >= 0.5f) {
                const std::string title = winTitleBase + " — " +
                                          std::to_string((int)(fpsFrames / fpsElapsed + 0.5f)) +
                                          " FPS";
                glfwSetWindowTitle(window, title.c_str());
                fpsWindowStart = now;
                fpsFrames = 0;
            }
        }
        printf("Frames rendered: %d\n", frameCount);

        model->DestroyRenderer();
        sModelForResize = nullptr;
    }   // model destroyed — GL context still alive for glDeleteTextures

    // --- Cleanup -------------------------------------------------------------
#ifdef CSM_TARGET_VULKAN
    if (Live2D::Common::Backend::IsVulkan()) {
        VulkanManager::GetInstance()->Destroy();
    }
#endif
    glfwDestroyWindow(window);
    glfwTerminate();
    return exitCode;
}
