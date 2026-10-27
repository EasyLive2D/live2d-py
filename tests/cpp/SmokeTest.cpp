#include "SmokeTest.hpp"

#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#include <IModel.hpp>

#include "ModelCatalog.hpp"

#define CHECK(cond)                                                           \
    do {                                                                      \
        if (!(cond)) {                                                        \
            fprintf(stderr, "[smoke] FAIL: %s (line %d)\n", #cond, __LINE__); \
            fails++;                                                          \
        }                                                                     \
    } while (0)

int RunSmokeModel(const std::string& smokePath) {
    using namespace Live2D;
    int fails = 0;

    std::unique_ptr<Live2D::IModel> model(Live2D::IModel::Create(smokePath.c_str()));
    if (!model) {
        fprintf(stderr, "[smoke] FAIL: cannot detect model version: %s\n", smokePath.c_str());
        return 1;
    }
    model->LoadModelJson(smokePath.c_str(), false);
    printf("[smoke] loaded (%s): params=%d parts=%d\n",
           model->IsV2() ? "v2" : "v3",
           model->GetParameterCount(),
           model->GetPartCount());
    CHECK(model->GetParameterCount() > 0);
    CHECK(model->GetPartCount() > 0);

    // granular updates
    CHECK(model->UpdateMotion(0.016f) == true || model->UpdateMotion(0.016f) == false);
    model->UpdateDrag(0.016f);
    model->UpdateBreath(0.016f);
    model->UpdateBlink(0.016f);
    model->UpdateExpression(0.016f);
    model->UpdatePhysics(0.016f);
    model->UpdatePose(0.016f);

    // save/load/reset parameters
    model->SetAndSaveParameterValue("PARAM_ANGLE_X", 5.0f, 1.0f);
    model->SetAndSaveParameterValue(0, 1.0f, 1.0f);
    model->AddAndSaveParameterValue("PARAM_ANGLE_X", 1.0f);
    model->AddAndSaveParameterValue(0, 1.0f);
    model->LoadParameters();
    model->SaveParameters();
    model->ResetAllParameters();
    CHECK(model->GetParameterValue(0) == model->GetParameterDefaultValue(0));

    // per-axis scale + mvp
    model->Resize(800, 800);
    model->SetScaleX(1.2f);
    model->SetScaleY(1.1f);
    model->SetScale(1.0f);
    CHECK(model->GetMvp() != nullptr);

    // update + Draw (deformer chain) so drawable vertices are available
    model->Update();
    model->Draw();

    // motion groups（模型可能不含 motions，如 llny —— 此时跳过）
    if (model->GetMotionGroupCount() > 0) {
        std::vector<std::string> gGroups;
        int gCounts = 0;
        model->GetMotions(
            &gGroups, [](void* c, const char* group, int no, const char* file, const char* sound) {
                auto* groups = (std::vector<std::string>*)c;
                if (no == 0)
                    groups->push_back(group);
            });
        CHECK(!gGroups.empty());
        if (!gGroups.empty()) {
            std::string firstGroup = gGroups[0];
            printf("[smoke] motion groups=%d first=%s count=%d\n",
                   model->GetMotionGroupCount(),
                   firstGroup.c_str(),
                   model->GetMotionCount(firstGroup.c_str()));
            CHECK(model->GetMotionCount(firstGroup.c_str()) > 0);

            // extra motion (re-using the file of the first group)
            std::string extraMotionPath;
            model->GetMotions(
                &extraMotionPath,
                [](void* c, const char* group, int no, const char* file, const char* sound) {
                    auto* out = (std::string*)c;
                    if (out->empty())
                        *out = file;
                });
            std::string extraMotionFull = std::string(model->GetModelHomeDir()) + extraMotionPath;
            int extraNo = model->LoadExtraMotion(firstGroup.c_str(), extraMotionFull.c_str());
            printf("[smoke] LoadExtraMotion -> no=%d\n", extraNo);
            CHECK(extraNo >= 0);
            CHECK(model->GetMotionCount(firstGroup.c_str()) >= 1);

            // ---- motion 回调（V2/V3 对齐语义：per-motion handler）----
            {
                std::vector<std::string> cbEvents;
                auto rec = [&cbEvents](const char* tag) -> IModel::MotionCallback {
                    return [&cbEvents, tag](const std::string& g, int n) {
                        cbEvents.push_back(std::string(tag) + ":" + g + ":" + std::to_string(n));
                    };
                };
                auto hasEv = [&cbEvents](const std::string& prefix) {
                    for (auto& e : cbEvents)
                        if (e.rfind(prefix, 0) == 0)
                            return true;
                    return false;
                };

                // 1) onStart 触发；自然播完触发 onFinish（v3 官方 Create
                //    不读 motion3.json 的 Loop 字段，_isLoop 恒 false，
                //    duration 到达即自然播完触发；v2 smoke 选用的各模型
                //    group0 均非 loop，1200 帧预算内必定触发）
                cbEvents.clear();
                model->StartMotion(firstGroup, 0, 3, rec("startA"), rec("finishA"));
                model->Update(1.0f / 60.0f);   // v3 的 began 在首次 Update 触发
                CHECK(hasEv("startA:" + firstGroup + ":0"));
                int finFrame = 0;
                for (; finFrame < 1200 && !hasEv("finishA:"); ++finFrame)
                    model->Update(1.0f / 60.0f);
                CHECK(hasEv("finishA:" + firstGroup + ":0"));
                printf("[smoke] motion finished at frame %d\n", finFrame);
                model->StopAllMotions();
                model->Update(1.0f / 60.0f);

                // 2) StopAllMotions 不触发 onFinish
                cbEvents.clear();
                model->StartMotion(firstGroup, 0, 3, rec("startC"), rec("finishC"));
                model->Update(1.0f / 60.0f);
                model->StopAllMotions();
                model->Update(1.0f / 60.0f);
                model->Update(1.0f / 60.0f);
                CHECK(hasEv("startC:"));
                CHECK(!hasEv("finishC:"));

                // 3) 低优先级拒绝：两端均静默（无回调）
                cbEvents.clear();
                model->StartMotion(firstGroup, 0, 3, nullptr, nullptr);
                model->Update(1.0f / 60.0f);
                model->StartMotion(firstGroup, 0, 1, rec("startL"), rec("finishL"));
                model->Update(1.0f / 60.0f);
                CHECK(cbEvents.empty());
                model->StopAllMotions();
                model->Update(1.0f / 60.0f);

                // 4) 不存在的组：onStart/onFinish 立即同步双触发
                cbEvents.clear();
                model->StartMotion("smoke_no_such_group", 0, 3, rec("startX"), rec("finishX"));
                CHECK(hasEv("startX:smoke_no_such_group:"));
                CHECK(hasEv("finishX:smoke_no_such_group:"));

                // 5) 重入（两端）：onFinish 内重启同一 motion，不崩溃且
                //    新 onStart 触发。v2 走 erase-before-callback 路径；
                //    v3 的 csmVector 迭代器为 {容器指针, 编号} 结构，
                //    解引用动态走 _vector->_ptr，天然免疫重入 push 的扩容
                {
                    cbEvents.clear();
                    auto* mdl = model.get();
                    auto restartLeft = std::make_shared<int>(1);
                    // v3 的 ReserveMotion 为 <= 语义（current=3 时重入 3
                    // 会被拒），重入用 4；v2 为 < 语义，3 即可通过
                    const int rePrio = model->IsV2() ? 3 : 4;
                    IModel::MotionCallback finR;
                    finR = [mdl, &cbEvents, restartLeft, rePrio](const std::string& g, int n) {
                        cbEvents.push_back("finishR:" + g + ":" + std::to_string(n));
                        if ((*restartLeft)-- > 0) {
                            mdl->StartMotion(
                                g,
                                n,
                                rePrio,
                                [&cbEvents](const std::string& g2, int n2) {
                                    cbEvents.push_back("startR:" + g2 + ":" + std::to_string(n2));
                                },
                                nullptr);
                        }
                    };
                    model->StartMotion(firstGroup, 0, 3, nullptr, finR);
                    // 自然播完 → onFinish 内重入 StartMotion
                    // （v2 startR 同步触发，v3 在下一帧 began 触发）
                    for (int f = 0; f < 1200 && !hasEv("startR:"); ++f)
                        model->Update(1.0f / 60.0f);
                    CHECK(hasEv("finishR:"));
                    CHECK(hasEv("startR:"));
                    model->StopAllMotions();
                    model->Update(1.0f / 60.0f);
                }

                // 6) burst 重入压测（两端）：onFinish 内同步连发 100 次
                //    StartMotion——多轮 vector 扩容（v3 csmVector
                //    10→20→40→80→160）全部发生在队列迭代中途。回归守卫：
                //    不崩溃、前 99 个被抢占 entry 静默、最后一个入队 entry
                //    自然播完仍触发其 onFinish
                {
                    cbEvents.clear();
                    auto* mdl = model.get();
                    auto burstLeft = std::make_shared<int>(100);
                    IModel::MotionCallback finB;
                    finB = [mdl, &cbEvents, burstLeft](const std::string& g, int n) {
                        cbEvents.push_back("finishB:" + g + ":" + std::to_string(n));
                        // 仅首轮 burst，防链式无限膨胀
                        const int left = *burstLeft;
                        *burstLeft = 0;
                        // 递增优先级 4..103 绕过两端优先级拒绝
                        for (int b = 0; b < left; ++b) {
                            mdl->StartMotion(
                                g, n, 4 + b, nullptr, [&cbEvents](const std::string& g2, int n2) {
                                    cbEvents.push_back("finishB2:" + g2 + ":" + std::to_string(n2));
                                });
                        }
                    };
                    model->StartMotion(firstGroup, 0, 3, nullptr, finB);
                    auto countEv = [&cbEvents](const char* tag) {
                        int c = 0;
                        for (auto& e : cbEvents)
                            if (e.rfind(tag, 0) == 0)
                                ++c;
                        return c;
                    };
                    for (int f = 0; f < 3600 && countEv("finishB2:") < 1; ++f)
                        model->Update(1.0f / 60.0f);
                    CHECK(countEv("finishB:") == 1);
                    CHECK(countEv("finishB2:") == 1);
                    model->StopAllMotions();
                    model->Update(1.0f / 60.0f);
                }
                printf("[smoke] motion callbacks OK (%zu events)\n", cbEvents.size());
            }
        }
    } else {
        printf("[smoke] no motion groups in model — skipped\n");
    }

    // hit tests
    std::vector<std::string> hitParts;
    model->HitPart(
        400,
        300,
        &hitParts,
        [](void* c, const char* id) { ((std::vector<std::string>*)c)->push_back(id); },
        false);
    printf("[smoke] HitPart: %zu hits\n", hitParts.size());
    CHECK(model->IsPartHit(0, 400, 300) == true || model->IsPartHit(0, 400, 300) == false);
    CHECK(model->IsDrawableHit(0, 400, 300) == true || model->IsDrawableHit(0, 400, 300) == false);

    // drawables
    int drawableCount = model->GetDrawableCount();
    printf("[smoke] drawables=%d\n", drawableCount);
    CHECK(drawableCount > 0);
    std::vector<std::string> drawableIds;
    model->GetDrawableIds(&drawableIds, [](void* c, const char* id) {
        ((std::vector<std::string>*)c)->push_back(id);
    });
    CHECK(!drawableIds.empty());
    CHECK(model->GetDrawableVertexCount(0) > 0);
    CHECK(model->GetDrawableVertexIndexCount(0) > 0);
    CHECK(model->GetDrawableVertices(0) != nullptr);
    CHECK(model->GetDrawableIndices(0) != nullptr);
    model->SetDrawableMultiColor(0, 0.5f, 0.5f, 0.5f, 1.0f);
    model->SetDrawableScreenColor(0, 0.5f, 0.5f, 0.5f, 1.0f);

    // expressions（模型可能不含 expressions —— 此时跳过）
    std::vector<std::pair<std::string, std::string>> exprs;
    model->GetExpressions(&exprs, [](void* c, const char* id, const char* file) {
        ((std::vector<std::pair<std::string, std::string>>*)c)->push_back({id, file});
    });
    printf("[smoke] expressions=%zu\n", exprs.size());
    if (!exprs.empty()) {
        model->AddExpression(exprs[0].first.c_str());
        model->RemoveExpression(exprs[0].first.c_str());
        model->ResetExpressions();
        // model3.json 里的表情路径是相对路径，LoadExtraExpression 不拼 homeDir
        std::string extraExprFull = std::string(model->GetModelHomeDir()) + exprs[0].second;
        model->LoadExtraExpression("smoke_extra", extraExprFull.c_str());
        CHECK(model->GetExpressionCount() > 0);
    }

    // moc consistency: v2 .moc 无此概念，恒为 false
    CHECK(model->HasMocConsistencyFromFile("kasumi2.moc") == false);

    if (fails == 0) {
        printf("[smoke] ALL OK\n");
        return 0;
    }
    printf("[smoke] %d checks FAILED\n", fails);
    return 1;
}

#undef CHECK

int RunSmokeAll() {
    int passed = 0, failed = 0;
    for (size_t i = 0; i < ModelTableSize(); i++) {
        const char* name = ModelTable()[i].first;
        printf("\n===== [smoke] %s =====\n", name);
        std::string p = FindUpward(ModelTable()[i].second);
        if (p.empty()) {
            fprintf(stderr,
                    "[smoke] %s: SKIP - resource not found: %s\n",
                    name,
                    ModelTable()[i].second);
            failed++;
            continue;
        }
        if (RunSmokeModel(p) == 0)
            passed++;
        else
            failed++;
    }
    printf(
        "\n[smoke] summary: %d passed, %d failed / %d models\n", passed, failed, passed + failed);
    return failed == 0 ? 0 : 1;
}
