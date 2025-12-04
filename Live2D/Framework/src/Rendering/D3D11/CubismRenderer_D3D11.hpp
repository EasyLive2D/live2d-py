/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#pragma once

#include "CubismNativeInclude_D3D11.hpp"

#include "../CubismRenderer.hpp"
#include "../CubismClippingManager.hpp"
#include "CubismFramework.hpp"
#include "CubismType_D3D11.hpp"
#include "Type/csmVector.hpp"
#include "Type/csmRectF.hpp"
#include "Math/CubismVector2.hpp"
#include "Type/csmMap.hpp"
#include "Rendering/D3D11/CubismOffscreenSurface_D3D11.hpp"
#include "CubismRenderState_D3D11.hpp"

//------------ LIVE2D NAMESPACE ------------
namespace Live2D { namespace Cubism { namespace Framework { namespace Rendering {

//  前方宣言
class CubismRenderer_D3D11;
class CubismShader_D3D11;
class CubismClippingContext_D3D11;

/**
 * @brief DirectX::XMMATRIXに変換
 */
DirectX::XMMATRIX ConvertToD3DX(CubismMatrix44& mtx);

/**
 * @brief  クリッピングマスクの処理を実行するクラス
 *
 */
class CubismClippingManager_D3D11 : public CubismClippingManager<CubismClippingContext_D3D11, CubismOffscreenSurface_D3D11>
{
public:

    /**
     * @brief   クリッピングコンテキストを作成する。モデル描画時に実行する。
     *
     * @param[in]   model       ->  モデルのインスタンス
     * @param[in]   renderer    ->  レンダラのインスタンス
     */
    void SetupClippingContext(ID3D11Device* device, ID3D11DeviceContext* renderContext, CubismModel& model, CubismRenderer_D3D11* renderer, csmInt32 offscreenCurrent);
};


/**
 * @brief   クリッピングマスクのコンテキスト
 */
class CubismClippingContext_D3D11 : public CubismClippingContext
{
    friend class CubismClippingManager_D3D11;
    friend class CubismShader_D3D11;
    friend class CubismRenderer_D3D11;

public:
    /**
     * @brief   引数付きコンストラクタ
     *
     */
    CubismClippingContext_D3D11(CubismClippingManager<CubismClippingContext_D3D11, CubismOffscreenSurface_D3D11>* manager, CubismModel& model, const csmInt32* clippingDrawableIndices, csmInt32 clipCount);

    /**
     * @brief   デストラクタ
     */
    virtual ~CubismClippingContext_D3D11();

    /**
     * @brief   このマスクを管理するマネージャのインスタンスを取得する。
     *
     * @return  クリッピングマネージャのインスタンス
     */
    CubismClippingManager<CubismClippingContext_D3D11, CubismOffscreenSurface_D3D11>* GetClippingManager();

    CubismClippingManager<CubismClippingContext_D3D11, CubismOffscreenSurface_D3D11>* _owner;        ///< このマスクを管理しているマネージャのインスタンス
};


/**
 * @brief   DirectX11用の描画命令を実装したクラス
 *
 */
class CubismRenderer_D3D11 : public CubismRenderer
{
    friend class CubismRenderer;
    friend class CubismClippingManager_D3D11;
    friend class CubismShader_D3D11;

public:

    /**
     * @brief    レンダラを作成するための各種設定
     *           モデルロードの前に一度だけ呼び出す
     *
     * @param[in]   bufferSetNum -> 描画コマンドバッファ数
     * @param[in]   device       -> 使用デバイス
     */
    static void InitializeConstantSettings(csmUint32 bufferSetNum, ID3D11Device* device);

    /**
     *  @brief  CubismRenderStateにデフォルトの設定をセットする
     */
    static void SetDefaultRenderState();

    /**
     * @brief  Cubism描画関連の先頭で行う処理。
     *          各フレームでのCubism処理前にこれを呼んでもらう
     *
     * @param[in]   device         -> 使用デバイス
     */
    static void StartFrame(ID3D11Device* device, ID3D11DeviceContext* renderContext, csmUint32 viewportWidth, csmUint32 viewportHeight);

    /**
     * @brief  Cubism描画関連の終了時行う処理。
     *          各フレームでのCubism処理前にこれを呼んでもらう
     *
     * @param[in]   device         -> 使用デバイス
     */
    static void EndFrame(ID3D11Device* device);

    /**
     * @brief  CubismRenderer_D3D11で使用するレンダーステート管理マネージャ取得
     */
    static CubismRenderState_D3D11* GetRenderStateManager();

    /**
     * @brief   レンダーステート管理マネージャ削除
     */
    static void DeleteRenderStateManager();

    /**
     * @brief   シェーダ管理機構の取得
     */
    static CubismShader_D3D11* GetShaderManager();

    /**
     * @brief   シェーダ管理機構の削除
     */
    static void DeleteShaderManager();

    /**
     * @brief   デバイスロスト・デバイス再作成時コールする
     */
    static void OnDeviceLost();

    /**
     * @brief   使用シェーダー作成
     */
    static void GenerateShader(ID3D11Device* device);

    /**
     * @brief   使用中デバイス取得
     */
    static ID3D11Device* GetCurrentDevice();

    /**
     * @brief   レンダラの初期化処理を実行する<br>
     *           引数に渡したモデルからレンダラの初期化処理に必要な情報を取り出すことができる
     *
     * @param[in]  model -> モデルのインスタンス
     */
    virtual void Initialize(Framework::CubismModel* model);

    /**
     * @brief   レンダラの初期化処理を実行する<br>
     *           引数に渡したモデルからレンダラの初期化処理に必要な情報を取り出すことができる
     *
     * @param[in]  model -> モデルのインスタンス
     */
    virtual void Initialize(Framework::CubismModel* model, csmInt32 maskBufferCount);

    /**
     * @brief   OpenGLテクスチャのバインド処理<br>
     *           CubismRendererにテクスチャを設定し、CubismRenderer中でその画像を参照するためのIndex値を戻り値とする
     *
     * @param[in]   modelTextureAssign  ->  セットするモデルテクスチャのアサイン番号
     * @param[in]   textureView            ->  描画に使用するテクスチャ
     *
     */
    void BindTexture(csmUint32 modelTextureAssign, ID3D11ShaderResourceView* textureView);

    /**
     * @brief   OpenGLにバインドされたテクスチャのリストを取得する
     *
     * @return  テクスチャのアドレスのリスト
     */
    const csmMap<csmInt32, ID3D11ShaderResourceView*>& GetBindedTextures() const;

    /**
     * @brief  クリッピングマスクバッファのサイズを設定する<br>
     *         マスク用のOffscreenSurfaceを破棄・再作成するため処理コストは高い。
     *
     * @param[in]  size -> クリッピングマスクバッファのサイズ
     *
     */
    void SetClippingMaskBufferSize(csmFloat32 width, csmFloat32 height);

    /**
     * @brief  レンダーテクスチャの枚数を取得する。
     *
     * @return  レンダーテクスチャの枚数
     *
     */
    csmInt32 GetRenderTextureCount() const;

    /**
     * @brief  クリッピングマスクバッファのサイズを取得する
     *
     * @return クリッピングマスクバッファのサイズ
     *
     */
    CubismVector2 GetClippingMaskBufferSize() const;

    /**
     * @brief  クリッピングマスクのバッファを取得する
     *
     * @return クリッピングマスクのバッファへの参照
     *
     */
    CubismOffscreenSurface_D3D11* GetMaskBuffer(csmUint32 backbufferNum, csmInt32 offscreenIndex);

protected:
    /**
     * @brief   コンストラクタ
     */
    CubismRenderer_D3D11();

    /**
     * @brief   デストラクタ
     */
    virtual ~CubismRenderer_D3D11();

    /**
     * @brief   モデルを描画する実際の処理
     *
     */
    virtual void DoDrawModel() override;

    /**
     * @brief   描画オブジェクト（アートメッシュ）を描画する。
     *
     * @param[in]   model       ->  描画対象のモデル
     * @param[in]   index       ->  描画対象のインデックス
     */
    void DrawMeshDX11(const CubismModel& model, const csmInt32 index);

private:

    /**
     * @brief  描画用に使用するシェーダの設定・コンスタントバッファの設定などを行い、マスク描画を実行
     *
     * @param[in]   model       ->  描画対象のモデル
     * @param[in]   index       ->  描画対象のインデックス
     */
    void ExecuteDrawForMask(const CubismModel& model, const csmInt32 index);

    /**
     * @brief  マスク作成用に使用するシェーダの設定・コンスタントバッファの設定などを行い、描画を実行
     *
     * @param[in]   model       ->  描画対象のモデル
     * @param[in]   index       ->  描画対象のインデックス
     */
    void ExecuteDrawForDraw(const CubismModel& model, const csmInt32 index);

    /**
     * @brief  指定されたメッシュインデックスに対して描画命令を実行する
     *
     * @param[in]   model       ->  描画対象のモデル
     * @param[in]   index       ->  描画対象のインデックス
     */
    void DrawDrawableIndexed(const CubismModel& model, const csmInt32 index);

    /**
     * @brief   レンダラが保持する静的なリソースを解放する
     */
    static void DoStaticRelease();

    /**
     * @brief   使用シェーダーと頂点定義の削除
     */
    static void ReleaseShader();


    // Prevention of copy Constructor
    CubismRenderer_D3D11(const CubismRenderer_D3D11&);
    CubismRenderer_D3D11& operator=(const CubismRenderer_D3D11&);

    /**
     * @brief   描画開始時の追加処理。<br>
     *           モデルを描画する前にクリッピングマスクに必要な処理を実装している。
     */
    void PreDraw();

    /**
     * @brief   描画完了後の追加処理。<br>
     *           ダブル・トリプルバッファリングに必要な処理を実装している。
     */
    void PostDraw();

    /**
     * @brief   モデル描画直前のステートを保持する
     */
    virtual void SaveProfile() override;

    /**
     * @brief   モデル描画直前のステートを保持する
     */
    virtual void RestoreProfile() override;

    /**
     * @brief   マスクテクスチャに描画するクリッピングコンテキストをセットする。
     */
    void SetClippingContextBufferForMask(CubismClippingContext_D3D11* clip);

    /**
     * @brief   マスクテクスチャに描画するクリッピングコンテキストを取得する。
     *
     * @return  マスクテクスチャに描画するクリッピングコンテキスト
     */
    CubismClippingContext_D3D11* GetClippingContextBufferForMask() const;

    /**
     * @brief   画面上に描画するクリッピングコンテキストをセットする。
     */
    void SetClippingContextBufferForDraw(CubismClippingContext_D3D11* clip);

    /**
     * @brief   画面上に描画するクリッピングコンテキストを取得する。
     *
     * @return  画面上に描画するクリッピングコンテキスト
     */
    CubismClippingContext_D3D11* GetClippingContextBufferForDraw() const;

    /**
     * @brief   GetDrawableVertices,GetDrawableVertexUvsの内容をバッファへコピー
     */
    void CopyToBuffer(ID3D11DeviceContext* renderContext, csmInt32 drawAssign, const csmInt32 vcount, const csmFloat32* varray, const csmFloat32* uvarray);

    /**
     * @brief   modelのindexにバインドされているテクスチャを返す。<br>
     *          もしバインドしていないindexを指定した場合NULLが返る。
     * @param[in]   model       ->  描画対象のモデル
     * @param[in]   index       ->  描画対象のインデックス
     */
    ID3D11ShaderResourceView* GetTextureViewWithIndex(const CubismModel& model, const csmInt32 index);

    /**
     *  @brief   ブレンドステートをセットする。
     */
    void SetBlendState(const CubismBlendMode blendMode);

    /**
     * @brief   シェーダをコンテキストにセットする<br>
     * @param[in]   model       ->  描画対象のモデル
     * @param[in]   index       ->  描画対象のインデックス
     */
    Csm::csmBool SetShader(const CubismModel& model, const csmInt32 index);

    /**
     * @brief  描画に使用するテクスチャを設定する。
     *
     * @param[in]   model       ->  描画対象のモデル
     * @param[in]   index       ->  描画対象のインデックス
     */
    void SetTextureView(const CubismModel& model, const csmInt32 index);

    /**
     * @brief  色関連の定数バッファを設定する
     *
     * @param[in]   cb             ->  設定する定数バッファ
     * @param[in]   model          ->  描画対象のモデル
     * @param[in]   index          ->  描画対象のインデックス
     * @param[in]   baseColor      ->  ベースカラー
     * @param[in]   multiplyColor  ->  乗算カラー
     * @param[in]   screenColor    ->  スクリーンカラー
     */
    void SetColorConstantBuffer(CubismConstantBufferD3D11& cb, const CubismModel& model, const csmInt32 index,
                                CubismTextureColor& baseColor, CubismTextureColor& multiplyColor, CubismTextureColor& screenColor);

    /**
     * @brief  描画に使用するカラーチャンネルを設定
     *
     * @param[in]   cb            ->  設定する定数バッファ
     * @param[in]   contextBuffer ->  描画コンテキスト
     */
    void SetColorChannel(CubismConstantBufferD3D11& cb, CubismClippingContext_D3D11* contextBuffer);

    /**
     * @brief  描画に使用するプロジェクション行列を更新する
     *
     * @param[in]   cb          ->  設定する定数バッファ
     * @param[in]   matrix      ->  設定するプロジェクション行列
     */
    void SetProjectionMatrix(CubismConstantBufferD3D11& cb, CubismMatrix44 matrix);

    /**
     * @brief  定数バッファを更新する
     *
     * @param[in]   cb          ->  書き込む定数バッファ情報
     * @param[in]   index       ->  描画対象のインデックス
     */
    void UpdateConstantBuffer(CubismConstantBufferD3D11& cb, csmInt32 index);

    /**
     * @brief   異方向フィルタリングの値によってサンプラーを設定する。
     */
    void SetSamplerAccordingToAnisotropy();

    /**
     * @brief   マスク生成時かを判定する
     *
     * @return  判定値
     */
    const csmBool inline IsGeneratingMask() const;

    ID3D11Buffer*** _vertexBuffers;         ///< 描画バッファ カラー無し、位置+UV
    ID3D11Buffer*** _indexBuffers;          ///< インデックスのバッファ
    ID3D11Buffer*** _constantBuffers;       ///< 定数のバッファ
    csmUint32 _drawableNum;           ///< _vertexBuffers, _indexBuffersの確保数

    csmInt32 _commandBufferNum;
    csmInt32 _commandBufferCurrent;

    csmVector<csmInt32> _sortedDrawableIndexList;       ///< 描画オブジェクトのインデックスを描画順に並べたリスト

    csmMap<csmInt32, ID3D11ShaderResourceView*> _textures;              ///< モデルが参照するテクスチャとレンダラでバインドしているテクスチャとのマップ

    csmVector<csmVector<CubismOffscreenSurface_D3D11> > _offscreenSurfaces; ///< マスク描画用のフレームバッファ

    CubismClippingManager_D3D11* _clippingManager;               ///< クリッピングマスク管理オブジェクト
    CubismClippingContext_D3D11* _clippingContextBufferForMask;  ///< マスクテクスチャに描画するためのクリッピングコンテキスト
    CubismClippingContext_D3D11* _clippingContextBufferForDraw;  ///< 画面上描画するためのクリッピングコンテキスト
};

}}}}
//------------ LIVE2D NAMESPACE ------------
