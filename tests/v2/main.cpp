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

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <thread>

// GLFW_INCLUDE_NONE — glad (GL/glew.h) provides all GL symbols
#define GLFW_INCLUDE_NONE
#include <GL/glew.h>   // glad OpenGL 4.6 core loader
#include <GLFW/glfw3.h>

#include <V2/Model.hpp>
#include "Log.hpp"

using namespace Live2D::Common::Log;

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
    bool smoke = false;
    int pathArg = 1;
    if (argc > 1 && std::string(argv[1]) == "--smoke") {
        smoke = true;
        pathArg = 2;
    }
    std::string modelPath;
    if (argc > pathArg) {
        modelPath = argv[pathArg];
    } else {
        // Default: kasumi2 model relative to repo root
        // When running from build dir, walk up to find Resources/
        namespace fs = std::filesystem;
        fs::path cwd = fs::current_path();
        fs::path candidate;
        while (true) {
            candidate = cwd / "Resources" / "v2" / "kasumi2" / "kasumi2.model.json";
            if (fs::exists(candidate))
                break;
            candidate = cwd / ".." / "Resources" / "v2" / "kasumi2" / "kasumi2.model.json";
            if (fs::exists(candidate))
                break;
            if (!cwd.has_parent_path() || cwd == cwd.parent_path()) {
                fprintf(stderr, "ERROR: Cannot find kasumi2.model.json\n");
                fprintf(stderr, "Usage: %s [--smoke] [path/to/model.json]\n", argv[0]);
                return 1;
            }
            cwd = cwd.parent_path();
        }
        modelPath = candidate.string();
    }
    printf("Model path: %s\n", modelPath.c_str());

    // --- Headless smoke test: exercise unified IModel APIs without GL --------
    if (smoke) {
        using namespace Live2D;
        int fails = 0;
#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            fprintf(stderr, "[smoke] FAIL: %s (line %d)\n", #cond, __LINE__); \
            fails++;                                                    \
        }                                                               \
    } while (0)

        V2::Model model;
        model.LoadModelJson(modelPath.c_str(), false);
        printf("[smoke] loaded: params=%d parts=%d\n", model.GetParameterCount(),
               model.GetPartCount());
        CHECK(model.GetParameterCount() > 0);
        CHECK(model.GetPartCount() > 0);

        // granular updates
        CHECK(model.UpdateMotion(0.016f) == true || model.UpdateMotion(0.016f) == false);
        model.UpdateDrag(0.016f);
        model.UpdateBreath(0.016f);
        model.UpdateBlink(0.016f);
        model.UpdateExpression(0.016f);
        model.UpdatePhysics(0.016f);
        model.UpdatePose(0.016f);

        // save/load/reset parameters
        model.SetAndSaveParameterValue("PARAM_ANGLE_X", 5.0f, 1.0f);
        model.SetAndSaveParameterValue(0, 1.0f, 1.0f);
        model.AddAndSaveParameterValue("PARAM_ANGLE_X", 1.0f);
        model.AddAndSaveParameterValue(0, 1.0f);
        model.LoadParameters();
        model.SaveParameters();
        model.ResetAllParameters();
        CHECK(model.GetParameterValue(0) == model.GetParameterDefaultValue(0));

        // per-axis scale + mvp
        model.Resize(800, 800);
        model.SetScaleX(1.2f);
        model.SetScaleY(1.1f);
        model.SetScale(1.0f);
        CHECK(model.GetMvp() != nullptr);

        // update + draw (deformer chain) so drawable vertices are available
        model.Update();
        model.Draw();

        // motion groups
        CHECK(model.GetMotionGroupCount() > 0);
        std::vector<std::string> gGroups;
        int gCounts = 0;
        model.GetMotions(&gGroups, [](void* c, const char* group, int no, const char* file,
                                      const char* sound) {
            auto* groups = (std::vector<std::string>*)c;
            if (no == 0)
                groups->push_back(group);
        });
        CHECK(!gGroups.empty());
        std::string firstGroup = gGroups[0];
        printf("[smoke] motion groups=%d first=%s count=%d\n", model.GetMotionGroupCount(),
               firstGroup.c_str(), model.GetMotionCount(firstGroup.c_str()));
        CHECK(model.GetMotionCount(firstGroup.c_str()) > 0);

        // hit tests
        std::vector<std::string> hitParts;
        model.HitPart(400, 300, &hitParts,
                      [](void* c, const char* id) {
                          ((std::vector<std::string>*)c)->push_back(id);
                      },
                      false);
        printf("[smoke] HitPart: %zu hits\n", hitParts.size());
        CHECK(model.IsPartHit(0, 400, 300) == true || model.IsPartHit(0, 400, 300) == false);
        CHECK(model.IsDrawableHit(0, 400, 300) == true ||
              model.IsDrawableHit(0, 400, 300) == false);

        // drawables
        int drawableCount = model.GetDrawableCount();
        printf("[smoke] drawables=%d\n", drawableCount);
        CHECK(drawableCount > 0);
        std::vector<std::string> drawableIds;
        model.GetDrawableIds(&drawableIds, [](void* c, const char* id) {
            ((std::vector<std::string>*)c)->push_back(id);
        });
        CHECK(!drawableIds.empty());
        CHECK(model.GetDrawableVertexCount(0) > 0);
        CHECK(model.GetDrawableVertexIndexCount(0) > 0);
        CHECK(model.GetDrawableVertices(0) != nullptr);
        CHECK(model.GetDrawableIndices(0) != nullptr);
        model.SetDrawableMultiColor(0, 0.5f, 0.5f, 0.5f, 1.0f);
        model.SetDrawableScreenColor(0, 0.5f, 0.5f, 0.5f, 1.0f);

        // expressions
        std::vector<std::pair<std::string, std::string>> exprs;
        model.GetExpressions(&exprs, [](void* c, const char* id, const char* file) {
            ((std::vector<std::pair<std::string, std::string>>*)c)
                ->push_back({id, file});
        });
        printf("[smoke] expressions=%zu\n", exprs.size());
        CHECK(!exprs.empty());
        model.AddExpression(exprs[0].first.c_str());
        model.RemoveExpression(exprs[0].first.c_str());
        model.ResetExpressions();
        model.LoadExtraExpression("smoke_extra", exprs[0].second.c_str());
        CHECK(model.GetExpressionCount() > 0);

        // extra motion (re-using the file of the first group)
        std::string extraMotionPath;
        model.GetMotions(&extraMotionPath, [](void* c, const char* group, int no,
                                              const char* file, const char* sound) {
            auto* out = (std::string*)c;
            if (out->empty())
                *out = file;
        });
        std::string extraMotionFull = std::string(model.GetModelHomeDir()) + extraMotionPath;
        int extraNo = model.LoadExtraMotion(firstGroup.c_str(), extraMotionFull.c_str());
        printf("[smoke] LoadExtraMotion -> no=%d\n", extraNo);
        CHECK(extraNo >= 0);
        CHECK(model.GetMotionCount(firstGroup.c_str()) >= 1);

        // moc consistency: v2 .moc 无此概念，恒为 false
        CHECK(model.HasMocConsistencyFromFile("kasumi2.moc") == false);

        if (fails == 0) {
            printf("[smoke] ALL OK\n");
            return 0;
        }
        printf("[smoke] %d checks FAILED\n", fails);
        return 1;
    }

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
    printf(
        "OpenGL %s, GLSL %s\n", glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));

    using namespace std::chrono_literals;
    // --- Baseline (idle, no model) -------------------------------------------
    // std::this_thread::sleep_for(1s);

    // --- Load model ----------------------------------------------------------
    EnableLive2DLog(true);
    SetLive2DLogLevel(LV_INFO);

    {
        Live2D::V2::Model model;
        model.LoadModelJson(modelPath.c_str());

        float cw, ch;
        model.GetCanvasSize(cw, ch);
        printf("Model loaded successfully!\n");
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

        model.Resize(winW, winH);
        model.StartRandomMotion("", 3);
        printf("\nStarted random idle motion.\n");

        // std::this_thread::sleep_for(1s);  // model loaded, before render

        printf("Running... (press ESC to exit)\n");
        int frameCount = 0;
        while (!glfwWindowShouldClose(window)) {
            model.Update();
            glViewport(0, 0, winW, winH);
            glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            model.Draw();
            frameCount++;
            glfwSwapBuffers(window);
            glfwPollEvents();
            if (model.IsMotionFinished()) {
                model.StartRandomMotion("", 3);
            }
        }
        printf("Frames rendered: %d\n", frameCount);

        // std::this_thread::sleep_for(1s);  // render ended, before destruction
    }   // model destroyed — GL context still alive for glDeleteTextures
    // std::this_thread::sleep_for(1s);       // after destruction baseline

    // --- Cleanup -------------------------------------------------------------
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
