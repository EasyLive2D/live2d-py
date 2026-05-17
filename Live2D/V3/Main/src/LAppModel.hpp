/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#pragma once

#include <CubismFramework.hpp>
#include <Model/CubismUserModel.hpp>
#include <ICubismModelSetting.hpp>
#include <Type/csmRectF.hpp>
#include <Rendering/OpenGL/CubismOffscreenSurface_OpenGLES2.hpp>

#include "LAppTextureManager.hpp"

#include "MatrixManager.hpp"

#include <Motion/CubismExpressionMotionManager.hpp>

#include <vector>
#include <unordered_map>
#include <string>

/**
 * @brief ユーザーが実際に使用するモデルの実装クラス<br>
 *         モデル生成、機能コンポーネント生成、更新処理とレンダリングの呼び出しを行う。
 *
 */
class LAppModel : public Csm::CubismUserModel
{
public:
    /**
     * @brief コンストラクタ
     */
    LAppModel();

    /**
     * @brief デストラクタ
     *
     */
    virtual ~LAppModel();

    /**
     * @brief model3.jsonが置かれたディレクトリとファイルパスからモデルを生成する
     *
     */
    void LoadModelJson(const Csm::csmChar* fileName, int maskBufferCount = 2);

    /**
     * @brief レンダラを再構築する
     *
     */
    void ReloadRenderer();

    /**
     * @brief   モデルの更新処理。モデルのパラメータから描画状態を決定する。
     *
     */
    void Update();

    /**
     * @brief   モデルを描画する処理。モデルを描画する空間のView-Projection行列を渡す。
     *
     * @param[in]  matrix  View-Projection行列
     */
    void Draw();

    /**
     * @brief   引数で指定したモーションの再生を開始する。
     *
     * @param[in]   group                       モーショングループ名
     * @param[in]   no                          グループ内の番号
     * @param[in]   priority                    優先度
     * @param[in]   onFinishedMotionHandler     モーション再生終了時に呼び出されるコールバック関数。NULLの場合、呼び出されない。
     * @return                                  開始したモーションの識別番号を返す。個別のモーションが終了したか否かを判定するIsFinished()の引数で使用する。開始できない時は「-1」
     */
    Csm::CubismMotionQueueEntryHandle StartMotion(const Csm::csmChar* group, Csm::csmInt32 no, Csm::csmInt32 priority,
                                                  void* onStartedCallee = nullptr,
                                                  Csm::ACubismMotion::BeganMotionCallback onStartMotionHandler =
                                                      nullptr,
                                                  void* onFinishedCallee = nullptr,
                                                  Csm::ACubismMotion::FinishedMotionCallback onFinishedMotionHandler =
                                                      nullptr);

    /**
     * @brief   ランダムに選ばれたモーションの再生を開始する。
     *
     * @param[in]   group                       モーショングループ名
     * @param[in]   priority                    優先度
     * @param[in]   onFinishedMotionHandler     モーション再生終了時に呼び出されるコールバック関数。NULLの場合、呼び出されない。
     * @return                                  開始したモーションの識別番号を返す。個別のモーションが終了したか否かを判定するIsFinished()の引数で使用する。開始できない時は「-1」
     */
    Csm::CubismMotionQueueEntryHandle StartRandomMotion(const Csm::csmChar* group, Csm::csmInt32 priority,
                                                        void* onStartedCallee = nullptr,
                                                        Csm::ACubismMotion::BeganMotionCallback onStartMotionHandler =
                                                            nullptr,
                                                        void* onFinishedCallee = nullptr,
                                                        Csm::ACubismMotion::FinishedMotionCallback
                                                        onFinishedMotionHandler = nullptr);

    /**
     * @brief   引数で指定した表情モーションをセットする
     *
     * @param   expressionID    表情モーションのID
     */
    void SetExpression(const Csm::csmChar* expressionID);

    /**
     * @brief   ランダムに選ばれた表情モーションをセットする
     *
     */
    const char* SetRandomExpression();

    /**
     * @brief   イベントの発火を受け取る
     *
     */
    virtual void MotionEventFired(const Live2D::Cubism::Framework::csmString& eventValue);

    /**
     * @brief    当たり判定テスト。<br>
     *            指定IDの頂点リストから矩形を計算し、座標が矩形範囲内か判定する。
     *
     * @param[in]   hitAreaName     当たり判定をテストする対象のID
     * @param[in]   x               判定を行うX座標
     * @param[in]   y               判定を行うY座標
     */
    virtual Csm::csmBool HitTest(const Csm::csmChar* hitAreaName, Csm::csmFloat32 x, Csm::csmFloat32 y);

    void Resize(int ww, int wh);

    /**
     * @brief   別ターゲットに描画する際に使用するバッファの取得
     */
    Csm::Rendering::CubismOffscreenSurface_OpenGLES2& GetRenderBuffer();

    /**
     * @brief   .moc3ファイルの整合性をチェックする
     *
     * @param[in]   mocName MOC3ファイル名
     * @return      MOC3に整合性があれば'true'、そうでなければ'false'。
     */
    Csm::csmBool HasMocConsistencyFromFile(const Csm::csmChar* mocFileName);

    bool IsMotionFinished();

    void SetParameterValue(const char* paramId, float value, float weight = 1.0f);

    void SetIndexParamValue(int index, float value, float weight = 1.0f);

    void AddParameterValue(const char* paramId, float value);

    void AddIndexParamValue(int index, float value);

    void SetAutoBreathEnable(bool enable);

    void SetAutoBlinkEnable(bool enable);

    int GetParameterCount();

    void GetParameter(int i, const char*& id, int& type, float& value, float& maxValue, float& minValue,
                      float& defaultValue);

    float GetParameterValue(int index);

    int GetPartCount();

    Csm::csmString GetPartId(int idx);

    void SetPartOpacity(int idx, float opacity);

    /**
     * 
     * @param x x in scene
     * @param y y in scene
     * @param collector
     * @param OnItem
     * @param 
     * @return 当前点击的 part no
     */
    void HitPart(float x, float y, bool topOnly, void* collector, void (*OnItem)(void*, const char*));

    void SetPartMultiplyColor(int partNo, float r, float g, float b, float a) const;

    void GetPartMultiplyColor(int partNo, float& r, float& g, float& b, float& a) const;

    void SetPartScreenColor(int partNo, float r, float g, float b, float a) const;

    void GetPartScreenColor(int partNo, float& r, float& g, float& b, float& a) const;

    void Drag(float x, float y);

    void SetOffset(float dx, float dy);

    void SetScale(float scale);

    void SetScaleX(float sx);

    void SetScaleY(float sy);

    void Rotate(float deg);

    void StopAllMotions();

    void ResetParameters();

    void ResetPose();

    void ResetExpression();

    void GetExpressionIds(void* collector, void(*callback)(void* collector, const char* expId));

    void GetMotionGroups(void* collector, void(*callback)(void* collector, const char* groupName, int count));

    int GetDrawableCount();

    void GetDrawableIds(void* collector, void(*callback)(void* collector, const char* drawableId));

    void SetDrawableMultiplyColor(int index, float r, float g, float b, float a);

    void SetDrawableScreenColor(int index, float r, float g, float b, float a);

    const char* GetSoundPath(const char* group, int index);

    void GetCanvasSize(float& w, float& h);

    void GetCanvasSizePixel(float& w, float& h);

    float GetPixelsPerUnit();

    void LoadParameters();

    void SaveParameters();

    void AddExpression(const char* expId);

    void RemoveExpression(const char* expId);

    void ResetExpressions();

protected:
    /**
     *  @brief  モデルを描画する処理。モデルを描画する空間のView-Projection行列を渡す。
     *
     */
    void DoDraw();

private:
    /**
     * @brief model3.jsonからモデルを生成する。<br>
     *         model3.jsonの記述に従ってモデル生成、モーション、物理演算などのコンポーネント生成を行う。
     *
     * @param[in]   setting     ICubismModelSettingのインスタンス
     *
     */
    void SetupModel(Csm::ICubismModelSetting* setting);

    /**
     * @brief OpenGLのテクスチャユニットにテクスチャをロードする
     *
     */
    void SetupTextures();

    /**
     * @brief   モーションデータをグループ名から一括でロードする。<br>
     *           モーションデータの名前は内部でModelSettingから取得する。
     *
     * @param[in]   group  モーションデータのグループ名
     */
    void PreloadMotionGroup(const Csm::csmChar* group);

    /**
     * @brief すべてのモーションデータの解放
     *
     * すべてのモーションデータを解放する。
     */
    void ReleaseMotions();

    /**
     * @brief すべての表情データの解放
     *
     * すべての表情データを解放する。
     */
    void ReleaseExpressions();

    bool IsHit(Csm::CubismIdHandle drawableId, Live2D::Cubism::Framework::csmFloat32 pointX, Live2D::Cubism::Framework::csmFloat32 pointY) override;

    Csm::ICubismModelSetting* _modelSetting; ///< モデルセッティング情報
    Csm::csmString _modelHomeDir; ///< モデルセッティングが置かれたディレクトリ
    Csm::csmVector<Csm::CubismIdHandle> _eyeBlinkIds; ///< モデルに設定されたまばたき機能用パラメータID
    Csm::csmVector<Csm::CubismIdHandle> _lipSyncIds; ///< モデルに設定されたリップシンク機能用パラメータID
    Csm::csmMap<Csm::csmString, Csm::ACubismMotion*> _motions; ///< 読み込まれているモーションのリスト
    Csm::csmMap<Csm::csmString, Csm::ACubismMotion*> _expressions; ///< 読み込まれている表情のリスト
    const Csm::CubismId* _idParamAngleX; ///< パラメータID: ParamAngleX
    const Csm::CubismId* _idParamAngleY; ///< パラメータID: ParamAngleX
    const Csm::CubismId* _idParamAngleZ; ///< パラメータID: ParamAngleX
    const Csm::CubismId* _idParamBodyAngleX; ///< パラメータID: ParamBodyAngleX
    const Csm::CubismId* _idParamEyeBallX; ///< パラメータID: ParamEyeBallX
    const Csm::CubismId* _idParamEyeBallY; ///< パラメータID: ParamEyeBallXY
    // 附加id，详见 https://docs.live2d.com/en/cubism-editor-manual/standard-parameter-list/

    int _iParamAngleX;
    int _iParamAngleY;
    int _iParamAngleZ;
    int _iParamBodyAngleX;
    int _iParamEyeBallX;
    int _iParamEyeBallY;

    LAppTextureManager _textureManager; ///< 纹理管理器

    Csm::Rendering::CubismOffscreenSurface_OpenGLES2 _renderBuffer; ///< フレームバッファ以外の描画先

    MatrixManager _matrixManager; ///< 绘制、点击、变换的矩阵管理器

    bool _autoBreath; ///< 自动呼吸开关
    bool _autoBlink; ///< 自动眨眼开关

    int* _tmpOrderedDrawIndices;

    double _currentFrame;
    double _lastFrame;
    float _deltaTimeSeconds;

    // used to clear motion effect
    const float* _defaultParameterValues;
    float* _parameterValues;
    bool _clearMotionFlag;
    int _parameterCount;

    std::vector<float> _savedParameterValues;

    std::unordered_map<std::string, Live2D::Cubism::Framework::CubismExpressionMotionManager*> _expMgrs;
};
