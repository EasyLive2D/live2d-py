/**
 * v2 (v2cpp) test — loads kasumi model and runs update/draw loop.
 *
 * Build:
 *   cmake -DV2CPP_TEST=ON -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -S . -B build \
 *         -G "Visual Studio 18 2026" -T host=x64 -A x64
 *   cmake --build build --config Release --target V2Test -j 24
 *
 * Run (from repo root):
 *   .\build\tests\v2\Release\V2Test.exe [model.json path]
 *
 * Default model: Resources/v2/kasumi2/kasumi2.model.json
 */

#include <cstdio>
#include <cstdlib>
#include <string>
#include <filesystem>
#include <thread>
#include <chrono>

// GLFW_INCLUDE_NONE — glad (GL/glew.h) provides all GL symbols
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <GL/glew.h>   // glad OpenGL 4.6 core loader

#include "LAppModel.hpp"
#include "Log.hpp"

// ============================================================================
// Helpers
// ============================================================================

static void glfwErrorCallback(int /*code*/, const char* msg) {
    fprintf(stderr, "[GLFW ERROR] %s\n", msg);
}

static void keyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

// ============================================================================
// Entry point
// ============================================================================

int main(int argc, char* argv[]) {
    // --- Resolve model path --------------------------------------------------
    std::string modelPath;
    if (argc > 1) {
        modelPath = argv[1];
    } else {
        // Default: kasumi2 model relative to repo root
        // When running from build dir, walk up to find Resources/
        namespace fs = std::filesystem;
        fs::path cwd = fs::current_path();
        fs::path candidate;
        while (true) {
            candidate = cwd / "Resources" / "v2" / "kasumi2" / "kasumi2.model.json";
            if (fs::exists(candidate)) break;
            candidate = cwd / ".." / "Resources" / "v2" / "kasumi2" / "kasumi2.model.json";
            if (fs::exists(candidate)) break;
            if (!cwd.has_parent_path() || cwd == cwd.parent_path()) {
                fprintf(stderr, "ERROR: Cannot find kasumi2.model.json\n");
                fprintf(stderr, "Usage: %s [path/to/model.json]\n", argv[0]);
                return 1;
            }
            cwd = cwd.parent_path();
        }
        modelPath = candidate.string();
    }
    printf("Model path: %s\n", modelPath.c_str());

    // --- GLFW + OpenGL window ------------------------------------------------
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) {
        fprintf(stderr, "ERROR: glfwInit() failed\n");
        return 1;
    }
    // Use Compatibility profile — v2cpp shaders are GLSL 1.20 (attribute/varying)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    int winW = 800, winH = 800;
    GLFWwindow* window = glfwCreateWindow(winW, winH, "v2cpp Test — kasumi", nullptr, nullptr);
    if (!window) {
        fprintf(stderr, "ERROR: glfwCreateWindow() failed\n");
        glfwTerminate();
        return 1;
    }
    glfwSetKeyCallback(window, keyCallback);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // --- Load OpenGL via glad ------------------------------------------------
    if (!gladLoadGL()) {
        fprintf(stderr, "ERROR: gladLoadGL() failed\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    printf("OpenGL %s, GLSL %s\n",
           glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));

    using namespace std::chrono_literals;
    // --- Baseline (idle, no model) -------------------------------------------
    // std::this_thread::sleep_for(1s);

    // --- Load model ----------------------------------------------------------
    EnableLive2DLog(true);
    SetLive2DLogLevel(LV_INFO);

    {
        live2d::LAppModel model;
        model.loadModelJson(modelPath);

        printf("Model loaded successfully!\n");
        printf("  Canvas: %.0f x %.0f\n", model.getCanvasWidth(), model.getCanvasHeight());
        printf("  Parameters: %d\n", model.getParameterCount());
        printf("  Parts: %d\n", model.getPartCount());

        int nParams = model.getParameterCount();
        int showParams = nParams < 15 ? nParams : 15;
        printf("  First %d parameters:\n", showParams);
        for (int i = 0; i < showParams; i++) {
            printf("    [%d] %-31s  val=%.3f  min=%.2f  max=%.2f  def=%.2f\n",
                   i, model.getParameterId(i).c_str(),
                   model.getParameterValue(i),
                   model.getParameterMin(i),
                   model.getParameterMax(i),
                   model.getParameterDefault(i));
        }
        if (nParams > 15) printf("    ... (%d more)\n", nParams - 15);

        int nParts = model.getPartCount();
        printf("  Parts:\n");
        for (int i = 0; i < nParts; i++) {
            printf("    [%d] %s\n", i, model.getPartId(i).c_str());
        }

        model.resize(winW, winH);
        model.startRandomMotion("", 3);
        printf("\nStarted random idle motion.\n");

        // std::this_thread::sleep_for(1s);  // model loaded, before render

        printf("Running... (press ESC to exit)\n");
        int frameCount = 0;
        while (!glfwWindowShouldClose(window)) {
            model.update();
            glViewport(0, 0, winW, winH);
            glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            model.draw();
            frameCount++;
            glfwSwapBuffers(window);
            glfwPollEvents();
            if (model.isMotionFinished()) {
                model.startRandomMotion("", 3);
            }
        }
        printf("Frames rendered: %d\n", frameCount);

        // std::this_thread::sleep_for(1s);  // render ended, before destruction
    } // model destroyed — GL context still alive for glDeleteTextures
    // std::this_thread::sleep_for(1s);       // after destruction baseline

    // --- Cleanup -------------------------------------------------------------
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
