/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "CubismRenderer_D3D11.hpp"

#include <cfloat> // FLT_MAX,MIN

#include "Math/CubismMatrix44.hpp"
#include "Type/csmVector.hpp"
#include "Model/CubismModel.hpp"
#include "CubismShader_D3D11.hpp"
#include "CubismRenderState_D3D11.hpp"

//------------ LIVE2D NAMESPACE ------------
namespace Live2D { namespace Cubism { namespace Framework { namespace Rendering {

DirectX::XMMATRIX ConvertToD3DX(CubismMatrix44& mtx)
{
    DirectX::XMMATRIX retMtx;
    retMtx.r[0].m128_f32[0] = mtx.GetArray()[ 0];
    retMtx.r[0].m128_f32[1] = mtx.GetArray()[ 1];
    retMtx.r[0].m128_f32[2] = mtx.GetArray()[ 2];
    retMtx.r[0].m128_f32[3] = mtx.GetArray()[ 3];

    retMtx.r[1].m128_f32[0] = mtx.GetArray()[ 4];
    retMtx.r[1].m128_f32[1] = mtx.GetArray()[ 5];
    retMtx.r[1].m128_f32[2] = mtx.GetArray()[ 6];
    retMtx.r[1].m128_f32[3] = mtx.GetArray()[ 7];

    retMtx.r[2].m128_f32[0] = mtx.GetArray()[ 8];
    retMtx.r[2].m128_f32[1] = mtx.GetArray()[ 9];
    retMtx.r[2].m128_f32[2] = mtx.GetArray()[10];
    retMtx.r[2].m128_f32[3] = mtx.GetArray()[11];

    retMtx.r[3].m128_f32[0] = mtx.GetArray()[12];
    retMtx.r[3].m128_f32[1] = mtx.GetArray()[13];
    retMtx.r[3].m128_f32[2] = mtx.GetArray()[14];
    retMtx.r[3].m128_f32[3] = mtx.GetArray()[15];

    return retMtx;
}

/*********************************************************************************************************************
*                                      CubismClippingManager_D3D11
********************************************************************************************************************/
void CubismClippingManager_D3D11::SetupClippingContext(ID3D11Device* device, ID3D11DeviceContext* renderContext, CubismModel& model, CubismRenderer_D3D11* renderer, csmInt32 offscreenCurrent)
{
    // 全てのクリッピングを用意する
    // 同じクリップ（複数の場合はまとめて１つのクリップ）を使う場合は１度だけ設定する
    csmInt32 usingClipCount = 0;
    for (csmUint32 clipIndex = 0; clipIndex < _clippingContextListForMask.GetSize(); clipIndex++)
    {
        // １つのクリッピングマスクに関して
        CubismClippingContext_D3D11* cc = _clippingContextListForMask[clipIndex];

        // このクリップを利用する描画オブジェクト群全体を囲む矩形を計算
        CalcClippedDrawTotalBounds(model, cc);

        if (cc->_isUsing)
        {
            usingClipCount++; //使用中としてカウント
        }
    }

    if (usingClipCount <= 0)
    {
        return;
    }

    // マスク作成処理
    // ビューポートは退避済み
    // 生成したOffscreenSurfaceと同じサイズでビューポートを設定
    CubismRenderer_D3D11::GetRenderStateManager()->SetViewport(renderContext,
            0,
            0,
            static_cast<FLOAT>(_clippingMaskBufferSize.X),
            static_cast<FLOAT>(_clippingMaskBufferSize.Y),
            0.0f, 1.0f);

    // 後の計算のためにインデックスの最初をセット
    _currentMaskBuffer = renderer->GetMaskBuffer(offscreenCurrent, 0);

    // ----- マスク描画処理 -----
    // マスク用RenderTextureをactiveにセット
    _currentMaskBuffer->BeginDraw(renderContext);

    // 各マスクのレイアウトを決定していく
    SetupLayoutBounds(usingClipCount);

    // サイズがレンダーテクスチャの枚数と合わない場合は合わせる
    if (_clearedMaskBufferFlags.GetSize() != _renderTextureCount)
    {
        _clearedMaskBufferFlags.Clear();

        for (csmInt32 i = 0; i < _renderTextureCount; ++i)
        {
            _clearedMaskBufferFlags.PushBack(false);
        }
    }
    else
    {
        // マスクのクリアフラグを毎フレーム開始時に初期化
        for (csmInt32 i = 0; i < _renderTextureCount; ++i)
        {
            _clearedMaskBufferFlags[i] = false;
        }
    }

    // 実際にマスクを生成する
    // 全てのマスクをどの様にレイアウトして描くかを決定し、ClipContext , ClippedDrawContext に記憶する
    for (csmUint32 clipIndex = 0; clipIndex < _clippingContextListForMask.GetSize(); clipIndex++)
    {
        // --- 実際に１つのマスクを描く ---
        CubismClippingContext_D3D11* clipContext = _clippingContextListForMask[clipIndex];
        csmRectF* allClippedDrawRect = clipContext->_allClippedDrawRect; //このマスクを使う、全ての描画オブジェクトの論理座標上の囲み矩形
        csmRectF* layoutBoundsOnTex01 = clipContext->_layoutBounds; //この中にマスクを収める
        const csmFloat32 MARGIN = 0.05f;
        const csmBool isRightHanded = true;

        // clipContextに設定したレンダーテクスチャをインデックスで取得
        CubismOffscreenSurface_D3D11* clipContextRenderTexture = renderer->GetMaskBuffer(offscreenCurrent, clipContext->_bufferIndex);

        // 現在のレンダーテクスチャがclipContextのものと異なる場合
        if (_currentMaskBuffer != clipContextRenderTexture)
        {
            _currentMaskBuffer->EndDraw(renderContext);
            _currentMaskBuffer = clipContextRenderTexture;

            // マスク用RenderTextureをactiveにセット
            _currentMaskBuffer->BeginDraw(renderContext);
        }

        // モデル座標上の矩形を、適宜マージンを付けて使う
        _tmpBoundsOnModel.SetRect(allClippedDrawRect);
        _tmpBoundsOnModel.Expand(allClippedDrawRect->Width * MARGIN, allClippedDrawRect->Height * MARGIN);
        //########## 本来は割り当てられた領域の全体を使わず必要最低限のサイズがよい
        // シェーダ用の計算式を求める。回転を考慮しない場合は以下のとおり
        // movePeriod' = movePeriod * scaleX + offX [[ movePeriod' = (movePeriod - tmpBoundsOnModel.movePeriod)*scale + layoutBoundsOnTex01.movePeriod ]]
        csmFloat32 scaleX = layoutBoundsOnTex01->Width / _tmpBoundsOnModel.Width;
        csmFloat32 scaleY = layoutBoundsOnTex01->Height / _tmpBoundsOnModel.Height;

        // マスク生成時に使う行列を求める
        createMatrixForMask(isRightHanded, layoutBoundsOnTex01, scaleX, scaleY);

        clipContext->_matrixForMask.SetMatrix(_tmpMatrixForMask.GetArray());
        clipContext->_matrixForDraw.SetMatrix(_tmpMatrixForDraw.GetArray());

        const csmInt32 clipDrawCount = clipContext->_clippingIdCount;
        for (csmInt32 i = 0; i < clipDrawCount; i++)
        {
            const csmInt32 clipDrawIndex = clipContext->_clippingIdList[i];

            // 頂点情報が更新されておらず、信頼性がない場合は描画をパスする
            if (!model.GetDrawableDynamicFlagVertexPositionsDidChange(clipDrawIndex))
            {
                continue;
            }

            renderer->IsCulling(model.GetDrawableCulling(clipDrawIndex) != 0);

            // マスクがクリアされていないなら処理する
            if (!_clearedMaskBufferFlags[clipContext->_bufferIndex])
            {
                // マスクをクリアする
                // (仮仕様) 1が無効（描かれない）領域、0が有効（描かれる）領域。（シェーダーCd*Csで0に近い値をかけてマスクを作る。1をかけると何も起こらない）
                renderer->GetMaskBuffer(offscreenCurrent, clipContext->_bufferIndex)->Clear(renderContext, 1.0f, 1.0f, 1.0f, 1.0f);
                _clearedMaskBufferFlags[clipContext->_bufferIndex] = true;
            }

            // 今回専用の変換を適用して描く
            // チャンネルも切り替える必要がある(A,R,G,B)
            renderer->SetClippingContextBufferForMask(clipContext);
            renderer->DrawMeshDX11(model, clipDrawIndex);
        }
    }

    // --- 後処理 ---
    _currentMaskBuffer->EndDraw(renderContext);
    renderer->SetClippingContextBufferForMask(NULL);
}

/*********************************************************************************************************************
*                                      CubismClippingContext_D3D11
********************************************************************************************************************/
CubismClippingContext_D3D11::CubismClippingContext_D3D11(CubismClippingManager<CubismClippingContext_D3D11, CubismOffscreenSurface_D3D11>* manager, CubismModel& model, const csmInt32* clippingDrawableIndices, csmInt32 clipCount)
    : CubismClippingContext(clippingDrawableIndices, clipCount)
{
    _isUsing = false;

    _owner = manager;
}

CubismClippingContext_D3D11::~CubismClippingContext_D3D11()
{
}

CubismClippingManager<CubismClippingContext_D3D11, CubismOffscreenSurface_D3D11>* CubismClippingContext_D3D11::GetClippingManager()
{
    return _owner;
}


/*********************************************************************************************************************
 *                                      CubismRenderer_D3D11
 ********************************************************************************************************************/

// 各種静的変数
namespace
{
    CubismRenderState_D3D11* s_renderStateManager = NULL;   ///< レンダーステートの管理
    CubismShader_D3D11* s_shaderManagerInstance = NULL;     ///< シェーダー管理

    csmUint32 s_bufferSetNum = 0;           ///< 作成コンテキストの数。モデルロード前に設定されている必要あり。
    ID3D11Device* s_device = NULL;          ///< 使用デバイス。モデルロード前に設定されている必要あり。
    ID3D11DeviceContext* s_context = NULL;  ///< 使用描画コンテキスト

    csmUint32 s_viewportWidth = 0;          ///< 描画ターゲット幅 CubismRenderer_D3D11::startframeで渡される
    csmUint32 s_viewportHeight = 0;         ///< 描画ターゲット高さ CubismRenderer_D3D11::startframeで渡される
}

CubismRenderer* CubismRenderer::Create()
{
    return CSM_NEW CubismRenderer_D3D11();
}

void CubismRenderer::StaticRelease()
{
    CubismRenderer_D3D11::DoStaticRelease();
}

CubismRenderState_D3D11* CubismRenderer_D3D11::GetRenderStateManager()
{
    if (s_renderStateManager == NULL)
    {
        s_renderStateManager = CSM_NEW CubismRenderState_D3D11();
    }
    return s_renderStateManager;
}

void CubismRenderer_D3D11::DeleteRenderStateManager()
{
    if (s_renderStateManager)
    {
        CSM_DELETE_SELF(CubismRenderState_D3D11, s_renderStateManager);
        s_renderStateManager = NULL;
    }
}

CubismShader_D3D11* CubismRenderer_D3D11::GetShaderManager()
{
    if (s_shaderManagerInstance == NULL)
    {
        s_shaderManagerInstance = CSM_NEW CubismShader_D3D11();
    }
    return s_shaderManagerInstance;
}

void CubismRenderer_D3D11::DeleteShaderManager()
{
    if (s_shaderManagerInstance)
    {
        CSM_DELETE_SELF(CubismShader_D3D11, s_shaderManagerInstance);
        s_shaderManagerInstance = NULL;
    }
}

void CubismRenderer_D3D11::GenerateShader(ID3D11Device* device)
{
    CubismShader_D3D11* shaderManager = GetShaderManager();
    if(shaderManager)
    {
        shaderManager->GenerateShaders(device);
    }
}

ID3D11Device* CubismRenderer_D3D11::GetCurrentDevice()
{
    return s_device;
}

void CubismRenderer_D3D11::OnDeviceLost()
{
    // シェーダー・頂点宣言開放
    ReleaseShader();
}

void CubismRenderer_D3D11::ReleaseShader()
{
    CubismShader_D3D11* shaderManager = GetShaderManager();
    if (shaderManager)
    {
        shaderManager->ReleaseShaderProgram();
    }
}

CubismRenderer_D3D11::CubismRenderer_D3D11()
    : _vertexBuffers(NULL)
    , _indexBuffers(NULL)
    , _constantBuffers(NULL)
    , _drawableNum(0)
    , _clippingManager(NULL)
    , _clippingContextBufferForMask(NULL)
    , _clippingContextBufferForDraw(NULL)
{
    _commandBufferNum = 0;
    _commandBufferCurrent = 0;

    // テクスチャ対応マップの容量を確保しておく.
    _textures.PrepareCapacity(32, true);
}

CubismRenderer_D3D11::~CubismRenderer_D3D11()
{
    {
        // オフスクリーンを作成していたのなら開放
        for (csmUint32 i = 0; i < _offscreenSurfaces.GetSize(); i++)
        {
            for (csmUint32 j = 0; j < _offscreenSurfaces[i].GetSize(); j++)
            {
                _offscreenSurfaces[i][j].DestroyOffscreenSurface();
            }
            _offscreenSurfaces[i].Clear();
        }
        _offscreenSurfaces.Clear();
    }

    const csmInt32 drawableCount = _drawableNum; //GetModel()->GetDrawableCount();

    for (csmUint32 buffer = 0; buffer < _commandBufferNum; buffer++)
    {
        for (csmUint32 drawAssign = 0; drawAssign < drawableCount; drawAssign++)
        {
            if (_constantBuffers[buffer][drawAssign])
            {
                _constantBuffers[buffer][drawAssign]->Release();
                _constantBuffers[buffer][drawAssign] = NULL;
            }
            // インデックス
            if (_indexBuffers[buffer][drawAssign])
            {
                _indexBuffers[buffer][drawAssign]->Release();
                _indexBuffers[buffer][drawAssign] = NULL;
            }
            // 頂点
            if (_vertexBuffers[buffer][drawAssign])
            {
                _vertexBuffers[buffer][drawAssign]->Release();
                _vertexBuffers[buffer][drawAssign] = NULL;
            }
        }

        CSM_FREE(_constantBuffers[buffer]);
        CSM_FREE(_indexBuffers[buffer]);
        CSM_FREE(_vertexBuffers[buffer]);
    }

    CSM_FREE(_constantBuffers);
    CSM_FREE(_indexBuffers);
    CSM_FREE(_vertexBuffers);

    CSM_DELETE_SELF(CubismClippingManager_D3D11, _clippingManager);
}

void CubismRenderer_D3D11::DoStaticRelease()
{
    // レンダーステートマネージャ削除
    DeleteRenderStateManager();
    // シェーダマネージャ削除
    DeleteShaderManager();
}


void CubismRenderer_D3D11::Initialize(CubismModel* model)
{
    Initialize(model, 1);
}

void CubismRenderer_D3D11::Initialize(CubismModel* model, csmInt32 maskBufferCount)
{
    // 0は許されず ここに来るまでに設定しなければならない
    if (s_device == 0)
    {
        CubismLogError("Device has not been set.");
        CSM_ASSERT(0);
        return;
    }

    // 1未満は1に補正する
    if (maskBufferCount < 1)
    {
        maskBufferCount = 1;
        CubismLogWarning("The number of render textures must be an integer greater than or equal to 1. Set the number of render textures to 1.");
    }

    if (model->IsUsingMasking())
    {
        _clippingManager = CSM_NEW CubismClippingManager_D3D11();  //クリッピングマスク・バッファ前処理方式を初期化
        _clippingManager->Initialize(
            *model,
            maskBufferCount
        );

        const csmInt32 bufferWidth = _clippingManager->GetClippingMaskBufferSize().X;
        const csmInt32 bufferHeight = _clippingManager->GetClippingMaskBufferSize().Y;

        _offscreenSurfaces.Clear();

        // バックバッファ分確保
        for (csmUint32 i = 0; i < s_bufferSetNum; i++)
        {
            csmVector<CubismOffscreenSurface_D3D11> vector;
            _offscreenSurfaces.PushBack(vector);
            for (csmUint32 j = 0; j < maskBufferCount; j++)
            {
                CubismOffscreenSurface_D3D11 offscreenSurface;
                offscreenSurface.CreateOffscreenSurface(s_device, bufferWidth, bufferHeight);
                _offscreenSurfaces[i].PushBack(offscreenSurface);
            }
        }
    }

    _sortedDrawableIndexList.Resize(model->GetDrawableCount(), 0);

    CubismRenderer::Initialize(model, maskBufferCount);  //親クラスの処理を呼ぶ

    // コマンドバッファごとに確保
    // 頂点バッファをコンテキスト分
    _vertexBuffers = static_cast<ID3D11Buffer***>(CSM_MALLOC(sizeof(ID3D11Buffer**) * s_bufferSetNum));
    _indexBuffers = static_cast<ID3D11Buffer***>(CSM_MALLOC(sizeof(ID3D11Buffer**) * s_bufferSetNum));
    _constantBuffers = static_cast<ID3D11Buffer***>(CSM_MALLOC(sizeof(ID3D11Buffer**) * s_bufferSetNum));

    // モデルパーツごとに確保
    const csmInt32 drawableCount = GetModel()->GetDrawableCount();
    _drawableNum = drawableCount;

    for (csmUint32 buffer = 0; buffer < s_bufferSetNum; buffer++)
    {
        // 頂点バッファ
        _vertexBuffers[buffer] = static_cast<ID3D11Buffer**>(CSM_MALLOC(sizeof(ID3D11Buffer*) * drawableCount));
        _indexBuffers[buffer] = static_cast<ID3D11Buffer**>(CSM_MALLOC(sizeof(ID3D11Buffer*) * drawableCount));
        _constantBuffers[buffer] = static_cast<ID3D11Buffer**>(CSM_MALLOC(sizeof(ID3D11Buffer*) * drawableCount));

        for (csmUint32 drawAssign = 0; drawAssign < drawableCount; drawAssign++)
        {
            // 頂点
            const csmInt32 vcount = GetModel()->GetDrawableVertexCount(drawAssign);
            if (vcount != 0)
            {
                D3D11_BUFFER_DESC bufferDesc;
                memset(&bufferDesc, 0, sizeof(bufferDesc));
                bufferDesc.ByteWidth = sizeof(CubismVertexD3D11) * vcount;    // 総長 構造体サイズ*個数
                bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
                bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
                bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
                bufferDesc.MiscFlags = 0;
                bufferDesc.StructureByteStride = 0;

                // 後で頂点を入れるので領域だけ
                if (FAILED(s_device->CreateBuffer(&bufferDesc, NULL, &_vertexBuffers[buffer][drawAssign])))
                {
                    CubismLogError("Vertexbuffer create failed : %d", vcount);
                }
            }
            else
            {
                _vertexBuffers[buffer][drawAssign] = NULL;
            }

            // インデックスはここで要素コピーを済ませる
            _indexBuffers[buffer][drawAssign] = NULL;
            const csmInt32 icount = GetModel()->GetDrawableVertexIndexCount(drawAssign);
            if (icount != 0)
            {
                D3D11_BUFFER_DESC bufferDesc;
                memset(&bufferDesc, 0, sizeof(bufferDesc));
                bufferDesc.ByteWidth = sizeof(WORD) * icount;    // 総長 構造体サイズ*個数
                bufferDesc.Usage = D3D11_USAGE_DEFAULT;
                bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
                bufferDesc.CPUAccessFlags = 0;
                bufferDesc.MiscFlags = 0;
                bufferDesc.StructureByteStride = 0;

                D3D11_SUBRESOURCE_DATA subResourceData;
                memset(&subResourceData, 0, sizeof(subResourceData));
                subResourceData.pSysMem = GetModel()->GetDrawableVertexIndices(drawAssign);
                subResourceData.SysMemPitch = 0;
                subResourceData.SysMemSlicePitch = 0;

                if (FAILED(s_device->CreateBuffer(&bufferDesc, &subResourceData, &_indexBuffers[buffer][drawAssign])))
                {
                    CubismLogError("Indexbuffer create failed : %d", icount);
                }
            }

            _constantBuffers[buffer][drawAssign] = NULL;
            {
                D3D11_BUFFER_DESC bufferDesc;
                memset(&bufferDesc, 0, sizeof(bufferDesc));
                bufferDesc.ByteWidth = sizeof(CubismConstantBufferD3D11);    // 総長 構造体サイズ*個数
                bufferDesc.Usage = D3D11_USAGE_DEFAULT; // 定数バッファに関しては「Map用にDynamic」にしなくともよい
                bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
                bufferDesc.CPUAccessFlags = 0;
                bufferDesc.MiscFlags = 0;
                bufferDesc.StructureByteStride = 0;

                if (FAILED(s_device->CreateBuffer(&bufferDesc, NULL, &_constantBuffers[buffer][drawAssign])))
                {
                    CubismLogError("ConstantBuffers create failed");
                }
            }
        }
    }

    _commandBufferNum = s_bufferSetNum;
    _commandBufferCurrent = 0;
}

void CubismRenderer_D3D11::PreDraw()
{
    SetDefaultRenderState();
}

void CubismRenderer_D3D11::PostDraw()
{
    // ダブル・トリプルバッファを回す
    _commandBufferCurrent++;
    if (_commandBufferNum <= _commandBufferCurrent)
    {
        _commandBufferCurrent = 0;
    }
}

void CubismRenderer_D3D11::DoDrawModel()
{
    // NULLは許されず
    CSM_ASSERT(s_device != NULL);
    CSM_ASSERT(s_context != NULL);

    PreDraw();

    //------------ クリッピングマスク・バッファ前処理方式の場合 ------------
    if (_clippingManager != NULL)
    {
        // サイズが違う場合はここで作成しなおし
        for (csmInt32 i = 0; i < _clippingManager->GetRenderTextureCount(); ++i)
        {
            if (_offscreenSurfaces[_commandBufferCurrent][i].GetBufferWidth() != static_cast<csmUint32>(_clippingManager->GetClippingMaskBufferSize().X) ||
                _offscreenSurfaces[_commandBufferCurrent][i].GetBufferHeight() != static_cast<csmUint32>(_clippingManager->GetClippingMaskBufferSize().Y))
            {
                _offscreenSurfaces[_commandBufferCurrent][i].CreateOffscreenSurface(s_device,
                    static_cast<csmUint32>(_clippingManager->GetClippingMaskBufferSize().X), static_cast<csmUint32>(_clippingManager->GetClippingMaskBufferSize().Y));
            }
        }

        if (IsUsingHighPrecisionMask())
        {
            _clippingManager->SetupMatrixForHighPrecision(*GetModel(), true);
        }
        else
        {
            _clippingManager->SetupClippingContext(s_device, s_context, *GetModel(), this, _commandBufferCurrent);
        }

        if (!IsUsingHighPrecisionMask())
        {
            // ビューポートを元に戻す
            GetRenderStateManager()->SetViewport(s_context,
                0.0f,
                0.0f,
                static_cast<float>(s_viewportWidth),
                static_cast<float>(s_viewportHeight),
                0.0f, 1.0f);
        }
    }

    const csmInt32 drawableCount = GetModel()->GetDrawableCount();
    const csmInt32* renderOrder = GetModel()->GetDrawableRenderOrders();

    // インデックスを描画順でソート
    for (csmInt32 i = 0; i < drawableCount; ++i)
    {
        const csmInt32 order = renderOrder[i];
        _sortedDrawableIndexList[order] = i;
    }

    // 描画
    for (csmInt32 i = 0; i < drawableCount; ++i)
    {
        const csmInt32 drawableIndex = _sortedDrawableIndexList[i];

        // Drawableが表示状態でなければ処理をパスする
        if (!GetModel()->GetDrawableDynamicFlagIsVisible(drawableIndex))
        {
            continue;
        }

        // クリッピングマスクをセットする
        CubismClippingContext_D3D11* clipContext = (_clippingManager != NULL)
            ? (*_clippingManager->GetClippingContextListForDraw())[drawableIndex]
            : NULL;

        if (clipContext != NULL && IsUsingHighPrecisionMask()) // マスクを書く必要がある
        {
            if (clipContext->_isUsing) // 書くことになっていた
            {
                CubismRenderer_D3D11::GetRenderStateManager()->SetViewport(s_context,
                    0,
                    0,
                    static_cast<FLOAT>(_clippingManager->GetClippingMaskBufferSize().X),
                    static_cast<FLOAT>(_clippingManager->GetClippingMaskBufferSize().Y),
                    0.0f, 1.0f);

                // 正しいレンダーターゲットを持つオフスクリーンサーフェイスバッファを呼ぶ
                CubismOffscreenSurface_D3D11* currentHighPrecisionMaskColorBuffer = &_offscreenSurfaces[_commandBufferCurrent][clipContext->_bufferIndex];

                currentHighPrecisionMaskColorBuffer->BeginDraw(s_context);
                currentHighPrecisionMaskColorBuffer->Clear(s_context, 1.0f, 1.0f, 1.0f, 1.0f);

                const csmInt32 clipDrawCount = clipContext->_clippingIdCount;
                for (csmInt32 ctx = 0; ctx < clipDrawCount; ctx++)
                {
                    const csmInt32 clipDrawIndex = clipContext->_clippingIdList[ctx];

                    // 頂点情報が更新されておらず、信頼性がない場合は描画をパスする
                    if (!GetModel()->GetDrawableDynamicFlagVertexPositionsDidChange(clipDrawIndex))
                    {
                        continue;
                    }

                    IsCulling(GetModel()->GetDrawableCulling(clipDrawIndex) != 0);

                    // 今回専用の変換を適用して描く
                    // チャンネルも切り替える必要がある(A,R,G,B)
                    SetClippingContextBufferForMask(clipContext);
                    DrawMeshDX11(*GetModel(), clipDrawIndex);
                }

                {
                    // --- 後処理 ---
                    currentHighPrecisionMaskColorBuffer->EndDraw(s_context);
                    SetClippingContextBufferForMask(NULL);

                    // ビューポートを元に戻す
                    GetRenderStateManager()->SetViewport(s_context,
                        0.0f,
                        0.0f,
                        static_cast<float>(s_viewportWidth),
                        static_cast<float>(s_viewportHeight),
                        0.0f, 1.0f);

                    PreDraw(); // バッファをクリアする
                }
            }
        }

        // クリッピングマスクをセットする
        SetClippingContextBufferForDraw(clipContext);

        IsCulling(GetModel()->GetDrawableCulling(drawableIndex) != 0);

        DrawMeshDX11(*GetModel(), drawableIndex);
    }

    // ダブルバッファ・トリプルバッファを回す
    PostDraw();
}

void CubismRenderer_D3D11::ExecuteDrawForMask(const CubismModel& model, const csmInt32 index)
{
    // 使用シェーダエフェクト取得
    CubismShader_D3D11* shaderManager = Live2D::Cubism::Framework::Rendering::CubismRenderer_D3D11::GetShaderManager();
    if(!shaderManager)
    {
        return;
    }

    ID3D11VertexShader* vs = shaderManager->GetVertexShader(ShaderNames_SetupMask);
    if(vs == NULL)
    {
        return;
    }
    ID3D11PixelShader* ps = shaderManager->GetPixelShader(ShaderNames_SetupMask);
    if(ps == NULL)
    {
        return;
    }

    // テクスチャ+サンプラーセット
    SetTextureView(model, index);
    SetSamplerAccordingToAnisotropy();

    // シェーダーセット
    s_context->VSSetShader(vs, NULL, 0);
    s_context->PSSetShader(ps, NULL, 0);

    // マスク用ブレンドステート
    GetRenderStateManager()->SetBlend(s_context,
        CubismRenderState_D3D11::Blend_Mask,
        DirectX::XMFLOAT4(0, 0, 0, 0),
        0xffffffff);

    // 定数バッファ
    {
        CubismConstantBufferD3D11 cb;
        memset(&cb, 0, sizeof(cb));

        // 使用するカラーチャンネルを設定
        CubismClippingContext_D3D11* contextBuffer = GetClippingContextBufferForMask();
        SetColorChannel(cb, contextBuffer);

        // 色
        csmRectF* rect = GetClippingContextBufferForMask()->_layoutBounds;
        CubismTextureColor baseColor = {rect->X * 2.0f - 1.0f, rect->Y * 2.0f - 1.0f, rect->GetRight() * 2.0f - 1.0f, rect->GetBottom() * 2.0f - 1.0f};
        CubismTextureColor multiplyColor = model.GetMultiplyColor(index);
        CubismTextureColor screenColor = model.GetScreenColor(index);
        SetColorConstantBuffer(cb, model, index, baseColor, multiplyColor, screenColor);

        // プロジェクションMtx
        SetProjectionMatrix(cb, GetClippingContextBufferForMask()->_matrixForMask);

        // Update
        UpdateConstantBuffer(cb, index);
    }

    // トライアングルリスト
    s_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 描画
    DrawDrawableIndexed(model, index);
}

void CubismRenderer_D3D11::ExecuteDrawForDraw(const CubismModel& model, const csmInt32 index)
{
    // 使用シェーダエフェクト取得
    CubismShader_D3D11* shaderManager = Live2D::Cubism::Framework::Rendering::CubismRenderer_D3D11::GetShaderManager();
    if(!shaderManager)
    {
        return;
    }

    // テクスチャ+サンプラーセット
    SetTextureView(model, index);
    SetSamplerAccordingToAnisotropy();

    // シェーダーセット
    if(!SetShader(model, index))
    {
        return;
    }

    // ブレンドステート
    {
        CubismBlendMode colorBlendMode = model.GetDrawableBlendMode(index);
        SetBlendState(colorBlendMode);
    }

    // 定数バッファ
    {
        CubismConstantBufferD3D11 cb;
        memset(&cb, 0, sizeof(cb));

        const csmBool masked = GetClippingContextBufferForDraw() != NULL;
        if (masked)
        {
            // View座標をClippingContextの座標に変換するための行列を設定
            DirectX::XMMATRIX clip = ConvertToD3DX(GetClippingContextBufferForDraw()->_matrixForDraw);
            XMStoreFloat4x4(&cb.clipMatrix, DirectX::XMMatrixTranspose(clip));

            // 使用するカラーチャンネルを設定
            CubismClippingContext_D3D11* contextBuffer = GetClippingContextBufferForDraw();
            SetColorChannel(cb, contextBuffer);
        }

        // 色
        CubismTextureColor baseColor = GetModelColorWithOpacity(model.GetDrawableOpacity(index));
        CubismTextureColor multiplyColor = model.GetMultiplyColor(index);
        CubismTextureColor screenColor = model.GetScreenColor(index);
        SetColorConstantBuffer(cb, model, index, baseColor, multiplyColor, screenColor);

        // プロジェクションMtx
        SetProjectionMatrix(cb, GetMvpMatrix());

        // Update
        UpdateConstantBuffer(cb, index);
    }

    // トライアングルリスト
    s_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 描画
    DrawDrawableIndexed(model, index);
}

void CubismRenderer_D3D11::DrawDrawableIndexed(const CubismModel& model, const csmInt32 index)
{
    UINT strides = sizeof(Csm::CubismVertexD3D11);
    UINT offsets = 0;
    ID3D11Buffer* vertexBuffer = _vertexBuffers[_commandBufferCurrent][index];
    ID3D11Buffer* indexBuffer = _indexBuffers[_commandBufferCurrent][index];
    const csmInt32 indexCount = model.GetDrawableVertexIndexCount(index);

    s_context->IASetVertexBuffers(0, 1, &vertexBuffer, &strides, &offsets);
    s_context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R16_UINT, 0);
    s_context->DrawIndexed(indexCount, 0, 0);
}

void CubismRenderer_D3D11::DrawMeshDX11(const CubismModel& model, const csmInt32 index)
{
    // デバイス未設定
    if (s_device == NULL)
    {
        return;
    }

    // 描画物無し
    if(model.GetDrawableVertexIndexCount(index) == 0)
    {
        return;
    }

    // 描画不要なら描画処理をスキップする
    if (model.GetDrawableOpacity(index) <= 0.0f && !IsGeneratingMask())
    {
        return;
    }

    // モデルが参照するテクスチャがバインドされていない場合は描画をスキップする
    if (GetTextureViewWithIndex(model, index) == NULL)
    {
        return;
    }

    // 裏面描画の有効・無効
    GetRenderStateManager()->SetCullMode(s_context, (IsCulling() ? CubismRenderState_D3D11::Cull_Ccw : CubismRenderState_D3D11::Cull_None));

    // 頂点バッファにコピー
    {
        const csmInt32 drawableIndex = index;
        const csmInt32 vertexCount = model.GetDrawableVertexCount(index);
        const csmFloat32* vertexArray = model.GetDrawableVertices(index);
        const csmFloat32* uvArray = reinterpret_cast<const csmFloat32*>(model.GetDrawableVertexUvs(index));
        CopyToBuffer(s_context, drawableIndex, vertexCount, vertexArray, uvArray);
    }

    // シェーダーセット・描画
    if (IsGeneratingMask())
    {
        ExecuteDrawForMask(model, index);
    }
    else
    {
        ExecuteDrawForDraw(model, index);
    }

    SetClippingContextBufferForDraw(NULL);
    SetClippingContextBufferForMask(NULL);
}

void CubismRenderer_D3D11::SaveProfile()
{
    // NULLは許されず
    CSM_ASSERT(s_device != NULL);
    CSM_ASSERT(s_context != NULL);

    // 現在のレンダリングステートをPush
    GetRenderStateManager()->SaveCurrentNativeState(s_device, s_context);
}

void CubismRenderer_D3D11::RestoreProfile()
{
    // NULLは許されず
    CSM_ASSERT(s_device != NULL);
    CSM_ASSERT(s_context != NULL);

    // SaveCurrentNativeStateと対
    GetRenderStateManager()->RestoreNativeState(s_device, s_context);
}

void CubismRenderer_D3D11::BindTexture(csmUint32 modelTextureAssign, ID3D11ShaderResourceView* textureView)
{
    _textures[modelTextureAssign] = textureView;
}

const csmMap<csmInt32, ID3D11ShaderResourceView*>& CubismRenderer_D3D11::GetBindedTextures() const
{
    return _textures;
}

void CubismRenderer_D3D11::SetClippingMaskBufferSize(csmFloat32 width, csmFloat32 height)
{
    if (_clippingManager == NULL)
    {
        return;
    }

    // インスタンス破棄前にレンダーテクスチャの数を保存
    const csmInt32 renderTextureCount = _clippingManager->GetRenderTextureCount();

    //OffscreenSurfaceのサイズを変更するためにインスタンスを破棄・再作成する
    CSM_DELETE_SELF(CubismClippingManager_D3D11, _clippingManager);

    _clippingManager = CSM_NEW CubismClippingManager_D3D11();

    _clippingManager->SetClippingMaskBufferSize(width, height);

    _clippingManager->Initialize(
        *GetModel(),
        renderTextureCount
    );
}

csmInt32 CubismRenderer_D3D11::GetRenderTextureCount() const
{
    return _clippingManager->GetRenderTextureCount();
}

CubismVector2 CubismRenderer_D3D11::GetClippingMaskBufferSize() const
{
    return _clippingManager->GetClippingMaskBufferSize();
}

CubismOffscreenSurface_D3D11* CubismRenderer_D3D11::GetMaskBuffer(csmUint32 backbufferNum, csmInt32 offscreenIndex)
{
    return &_offscreenSurfaces[backbufferNum][offscreenIndex];
}

void CubismRenderer_D3D11::InitializeConstantSettings(csmUint32 bufferSetNum, ID3D11Device* device)
{
    s_bufferSetNum = bufferSetNum;
    s_device = device;

    // 実体を作成しておく
    CubismRenderer_D3D11::GetRenderStateManager();
}

void CubismRenderer_D3D11::SetDefaultRenderState()
{
    // Zは無効 描画順で制御
    GetRenderStateManager()->SetZEnable(s_context,
        CubismRenderState_D3D11::Depth_Disable,
        0);

    // ビューポート
    GetRenderStateManager()->SetViewport(s_context,
        0.0f,
        0.0f,
        static_cast<float>(s_viewportWidth),
        static_cast<float>(s_viewportHeight),
        0.0f, 1.0f);
}

void CubismRenderer_D3D11::StartFrame(ID3D11Device* device, ID3D11DeviceContext* renderContext, csmUint32 viewportWidth, csmUint32 viewportHeight)
{
    // フレームで使用するデバイス設定
    s_device = device;
    s_context = renderContext;
    s_viewportWidth = viewportWidth;
    s_viewportHeight = viewportHeight;

    // レンダーステートフレーム先頭処理
    GetRenderStateManager()->StartFrame();

    // シェーダ・頂点宣言
    GetShaderManager()->SetupShader(s_device, s_context);
}

void CubismRenderer_D3D11::EndFrame(ID3D11Device* device)
{
}

void CubismRenderer_D3D11::SetClippingContextBufferForDraw(CubismClippingContext_D3D11* clip)
{
    _clippingContextBufferForDraw = clip;
}

CubismClippingContext_D3D11* CubismRenderer_D3D11::GetClippingContextBufferForDraw() const
{
    return _clippingContextBufferForDraw;
}

void CubismRenderer_D3D11::SetClippingContextBufferForMask(CubismClippingContext_D3D11* clip)
{
    _clippingContextBufferForMask = clip;
}

CubismClippingContext_D3D11* CubismRenderer_D3D11::GetClippingContextBufferForMask() const
{
    return _clippingContextBufferForMask;
}

void CubismRenderer_D3D11::CopyToBuffer(ID3D11DeviceContext* renderContext, csmInt32 drawAssign, const csmInt32 vcount, const csmFloat32* varray, const csmFloat32* uvarray)
{
    // CubismVertexD3D11の書き込み
    if (_vertexBuffers[_commandBufferCurrent][drawAssign])
    {
        D3D11_MAPPED_SUBRESOURCE subRes;
        if (SUCCEEDED(renderContext->Map(_vertexBuffers[_commandBufferCurrent][drawAssign], 0, D3D11_MAP_WRITE_DISCARD, 0, &subRes)))
        {
            CubismVertexD3D11* lockPointer = reinterpret_cast<CubismVertexD3D11*>(subRes.pData);
            for (csmInt32 ct = 0; ct < vcount * 2; ct += 2)
            {// モデルデータからのコピー
                lockPointer[ct / 2].x = varray[ct + 0];
                lockPointer[ct / 2].y = varray[ct + 1];
                lockPointer[ct / 2].u = uvarray[ct + 0];
                lockPointer[ct / 2].v = uvarray[ct + 1];
            }
            renderContext->Unmap(_vertexBuffers[_commandBufferCurrent][drawAssign], 0);
        }
    }
}

ID3D11ShaderResourceView* CubismRenderer_D3D11::GetTextureViewWithIndex(const CubismModel& model, const csmInt32 index)
{
    ID3D11ShaderResourceView* result = NULL;
    const csmInt32 textureIndex = model.GetDrawableTextureIndex(index);
    if (textureIndex >= 0)
    {
        result = _textures[textureIndex];
    }
    return result;
}

void CubismRenderer_D3D11::SetBlendState(const CubismBlendMode blendMode)
{
    switch (blendMode)
    {
    case CubismRenderer::CubismBlendMode::CubismBlendMode_Normal:
    default:
        GetRenderStateManager()->SetBlend(s_context,
            CubismRenderState_D3D11::Blend_Normal,
            DirectX::XMFLOAT4(0, 0, 0, 0),
            0xffffffff);
        break;

    case CubismRenderer::CubismBlendMode::CubismBlendMode_Additive:
        GetRenderStateManager()->SetBlend(s_context,
            CubismRenderState_D3D11::Blend_Add,
            DirectX::XMFLOAT4(0, 0, 0, 0),
            0xffffffff);
        break;

    case CubismRenderer::CubismBlendMode::CubismBlendMode_Multiplicative:
        GetRenderStateManager()->SetBlend(s_context,
            CubismRenderState_D3D11::Blend_Mult,
            DirectX::XMFLOAT4(0, 0, 0, 0),
            0xffffffff);
        break;
    }
}

Csm::csmBool CubismRenderer_D3D11::SetShader(const CubismModel& model, const csmInt32 index)
{
    const csmBool masked = GetClippingContextBufferForDraw() != NULL;
    const csmBool premult = IsPremultipliedAlpha();
    const csmBool invertedMask = model.GetDrawableInvertedMask(index);

    const ShaderNames vertexShaderNames = (masked ?
                                            ShaderNames_NormalMasked :
                                            ShaderNames_Normal);
    ShaderNames pixelShaderNames;
    if (masked)
    {
        if(premult)
        {
            if (invertedMask)
            {
                pixelShaderNames = ShaderNames_NormalMaskedInvertedPremultipliedAlpha;
            }
            else
            {
                pixelShaderNames = ShaderNames_NormalMaskedPremultipliedAlpha;
            }
        }
        else
        {
            if (invertedMask)
            {
                pixelShaderNames = ShaderNames_NormalMaskedInverted;
            }
            else
            {
                pixelShaderNames = ShaderNames_NormalMasked;
            }
        }
    }
    else
    {
        if(premult)
        {
            pixelShaderNames = ShaderNames_NormalPremultipliedAlpha;
        }
        else
        {
            pixelShaderNames = ShaderNames_Normal;
        }
    }

    CubismShader_D3D11* shaderManager = Live2D::Cubism::Framework::Rendering::CubismRenderer_D3D11::GetShaderManager();
    ID3D11VertexShader*  vs = shaderManager->GetVertexShader(vertexShaderNames);
    if(vs == NULL)
    {
        return false;
    }
    ID3D11PixelShader* ps = shaderManager->GetPixelShader(pixelShaderNames);
    if(ps == NULL)
    {
        return false;
    }

    s_context->VSSetShader(vs, NULL, 0);
    s_context->PSSetShader(ps, NULL, 0);
    return true;
}

void CubismRenderer_D3D11::SetTextureView(const CubismModel& model, const csmInt32 index)
{
    const csmBool masked = GetClippingContextBufferForDraw() != NULL;
    const csmBool drawing = !IsGeneratingMask();

    ID3D11ShaderResourceView* textureView = GetTextureViewWithIndex(model, index);
    ID3D11ShaderResourceView* maskView = (masked && drawing ? _offscreenSurfaces[_commandBufferCurrent][GetClippingContextBufferForDraw()->_bufferIndex].GetTextureView() : NULL);
    ID3D11ShaderResourceView* const viewArray[2] = { textureView, maskView };
    s_context->PSSetShaderResources(0, 2, viewArray);
}

void CubismRenderer_D3D11::SetColorConstantBuffer(CubismConstantBufferD3D11& cb, const CubismModel& model, const csmInt32 index,
                                                  CubismTextureColor& baseColor, CubismTextureColor& multiplyColor, CubismTextureColor& screenColor)
{
    XMStoreFloat4(&cb.baseColor, DirectX::XMVectorSet(baseColor.R, baseColor.G, baseColor.B, baseColor.A));
    XMStoreFloat4(&cb.multiplyColor, DirectX::XMVectorSet(multiplyColor.R, multiplyColor.G, multiplyColor.B, multiplyColor.A));
    XMStoreFloat4(&cb.screenColor, DirectX::XMVectorSet(screenColor.R, screenColor.G, screenColor.B, screenColor.A));
}

void CubismRenderer_D3D11::SetColorChannel(CubismConstantBufferD3D11& cb, CubismClippingContext_D3D11* contextBuffer)
{
    const csmInt32 channelIndex = contextBuffer->_layoutChannelIndex;
    CubismRenderer::CubismTextureColor* colorChannel = contextBuffer->GetClippingManager()->GetChannelFlagAsColor(channelIndex);
    XMStoreFloat4(&cb.channelFlag, DirectX::XMVectorSet(colorChannel->R, colorChannel->G, colorChannel->B, colorChannel->A));
}

void CubismRenderer_D3D11::SetProjectionMatrix(CubismConstantBufferD3D11& cb, CubismMatrix44 matrix)
{
    DirectX::XMMATRIX proj = ConvertToD3DX(matrix);
    XMStoreFloat4x4(&cb.projectMatrix, DirectX::XMMatrixTranspose(proj));
}

void CubismRenderer_D3D11::UpdateConstantBuffer(CubismConstantBufferD3D11& cb, csmInt32 index)
{
    ID3D11Buffer* constantBuffer = _constantBuffers[_commandBufferCurrent][index];
    s_context->UpdateSubresource(constantBuffer, 0, NULL, &cb, 0, 0);

    s_context->VSSetConstantBuffers(0, 1, &constantBuffer);
    s_context->PSSetConstantBuffers(0, 1, &constantBuffer);
}

void CubismRenderer_D3D11::SetSamplerAccordingToAnisotropy()
{
    if (GetAnisotropy() >= 1.0f)
    {
        GetRenderStateManager()->SetSampler(s_context, CubismRenderState_D3D11::Sampler_Anisotropy, GetAnisotropy(), s_device);
    }
    else
    {
        GetRenderStateManager()->SetSampler(s_context, CubismRenderState_D3D11::Sampler_Normal);
    }
}

const csmBool inline CubismRenderer_D3D11::IsGeneratingMask() const
{
    return (GetClippingContextBufferForMask() != NULL);
}

}}}}

//------------ LIVE2D NAMESPACE ------------
