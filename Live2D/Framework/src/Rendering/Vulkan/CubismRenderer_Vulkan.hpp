/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#pragma once
#include <string>
#include "../CubismRenderer.hpp"
#include "../CubismClippingManager.hpp"
#include <vulkan/vulkan.h>
#include "CubismFramework.hpp"
#include "CubismOffscreenSurface_Vulkan.hpp"
#include "CubismClass_Vulkan.hpp"
#include "Type/csmVector.hpp"
#include "Type/csmMap.hpp"
#include "Math/CubismVector2.hpp"

//------------ LIVE2D NAMESPACE ------------
namespace Live2D { namespace Cubism { namespace Framework { namespace Rendering {

/**
 * @brief   viewportのサイズを設定する
 *
 * @param[in]  width    -> 横幅
 * @param[in]  height   -> 縦幅
 * @param[in]  minDepth -> 最小深度
 * @param[in]  maxDepth -> 最大深度
 */
VkViewport GetViewport(csmFloat32 width, csmFloat32 height, csmFloat32 minDepth, csmFloat32 maxDepth);

/**
 * @brief   scissorのサイズを設定する
 *
 * @param[in]  offsetX  -> x方向のオフセット
 * @param[in]  offsetY  -> y方向のオフセット
 * @param[in]  width    -> 横幅
 * @param[in]  height   -> 縦幅
 */
VkRect2D GetScissor(csmFloat32 offsetX, csmFloat32 offsetY, csmFloat32 width, csmFloat32 height);

/*********************************************************************************************************************
*                                      CubismRenderer_Vulkan
********************************************************************************************************************/
//  前方宣言
class CubismRenderer_Vulkan;
class CubismClippingContext_Vulkan;

/**
 * @brief  クリッピングマスクの処理を実行するクラス
 */
class CubismClippingManager_Vulkan : public CubismClippingManager<
            CubismClippingContext_Vulkan, CubismOffscreenSurface_Vulkan>
{
public:
    /**
     * @brief   クリッピングコンテキストを作成する。モデル描画時に実行する。
     *
     * @param[in]   model          ->  モデルのインスタンス
     * @param[in]   commandBuffer  ->  コマンドバッファ
     * @param[in]   updateCommandBuffer  ->  更新用コマンドバッファ
     * @param[in]   renderer       ->  レンダラのインスタンス
     */
    void SetupClippingContext(CubismModel& model, VkCommandBuffer commandBuffer, VkCommandBuffer updateCommandBuffer,
                              CubismRenderer_Vulkan* renderer, csmInt32 offscreenCurrent);
};

/**
 * @brief   クリッピングマスクのコンテキスト
 */
class CubismClippingContext_Vulkan : public CubismClippingContext
{
    friend class CubismClippingManager_Vulkan;
    friend class CubismRenderer_Vulkan;

public:
    /**
     * @brief   引数付きコンストラクタ
     *
     */
    CubismClippingContext_Vulkan(
        CubismClippingManager<CubismClippingContext_Vulkan, CubismOffscreenSurface_Vulkan>* manager, CubismModel& model,
        const csmInt32* clippingDrawableIndices, csmInt32 clipCount);

    /**
     * @brief   デストラクタ
     */
    virtual ~CubismClippingContext_Vulkan();

    /**
     * @brief   このマスクを管理するマネージャのインスタンスを取得する。
     *
     * @return  クリッピングマネージャのインスタンス
     */
    CubismClippingManager<CubismClippingContext_Vulkan, CubismOffscreenSurface_Vulkan>* GetClippingManager();

    CubismClippingManager<CubismClippingContext_Vulkan, CubismOffscreenSurface_Vulkan>* _owner;
    ///< このマスクを管理しているマネージャのインスタンス
};

enum ShaderNames
{
    // SetupMask
    ShaderNames_SetupMask,

    //Normal
    ShaderNames_Normal,
    ShaderNames_NormalMasked,
    ShaderNames_NormalMaskedInverted,
    ShaderNames_NormalPremultipliedAlpha,
    ShaderNames_NormalMaskedPremultipliedAlpha,
    ShaderNames_NormalMaskedInvertedPremultipliedAlpha,

    //Add
    ShaderNames_Add,
    ShaderNames_AddMasked,
    ShaderNames_AddMaskedInverted,
    ShaderNames_AddPremultipliedAlpha,
    ShaderNames_AddMaskedPremultipliedAlpha,
    ShaderNames_AddMaskedPremultipliedAlphaInverted,

    //Mult
    ShaderNames_Mult,
    ShaderNames_MultMasked,
    ShaderNames_MultMaskedInverted,
    ShaderNames_MultPremultipliedAlpha,
    ShaderNames_MultMaskedPremultipliedAlpha,
    ShaderNames_MultMaskedPremultipliedAlphaInverted,
};

enum Blend
{
    Blend_Normal,
    Blend_Add,
    Blend_Mult,
    Blend_Mask
};

/**
 * @brief   頂点情報を保持する構造体
 *
 */
struct ModelVertex
{
    CubismVector2 pos; // Position
    CubismVector2 texCoord; // UVs

    static VkVertexInputBindingDescription GetBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(ModelVertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static void GetAttributeDescriptions(VkVertexInputAttributeDescription attributeDescriptions[2])
    {
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(ModelVertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(ModelVertex, texCoord);
    }
};

/**
 * @brief  シェーダー関連のプログラムを生成・破棄するクラス<br>
 *         シングルトンなクラスであり、CubismShader_Vulkan::GetInstance()からアクセスする。
 */
class CubismPipeline_Vulkan
{
public:
    /**
     * @brief   コンストラクタ
     */
    CubismPipeline_Vulkan();

    /**
     * @brief   デストラクタ
     */
    ~CubismPipeline_Vulkan();

    struct PipelineResource
    {
        /**
         * @brief   シェーダーモジュールを作成する
         *
         * @param[in]   device        ->  論理デバイス
         * @param[in]   filename      ->  ファイル名
         */
        VkShaderModule CreateShaderModule(VkDevice device, std::string filename);

        /**
         * @brief   パイプラインを作成する
         *
         * @param[in]   vertFileName        ->  Vertexシェーダーのファイル
         * @param[in]   fragFileName        ->  Fragmentシェーダーのファイル
         * @param[in]   descriptorSetLayout ->  ディスクリプタセットレイアウト
         */
        bool CreateGraphicsPipeline(std::string vertFileName, std::string fragFileName,
                                    VkDescriptorSetLayout descriptorSetLayout);
        void Release();
        VkPipeline GetPipeline(csmInt32 index)
        {
             return _pipeline[index];
        }
        VkPipelineLayout GetPipelineLayout(csmInt32 index) { return _pipelineLayout[index]; }

    private:
        csmVector<VkPipeline> _pipeline; ///< normal, add, multi, maskそれぞれのパイプライン
        csmVector<VkPipelineLayout> _pipelineLayout; ///< normal, add, multi, maskそれぞれのパイプラインレイアウト
    };

    /**
     * @brief   シェーダーごとにPipelineResourceのインスタンスを作成する
     *
     * @param[in]   descriptorSetLayout ->  ディスクリプタセットレイアウト
     */
    void CreatePipelines(VkDescriptorSetLayout descriptorSetLayout);

    /**
     * @brief   指定したシェーダーのグラフィックスパイプラインを取得する
     *
     * @param[in]   shaderIndex         ->  シェーダインデックス
     * @param[in]   blendIndex          ->  ブレンドモードのインデックス
     *
     * @return  指定したシェーダーのグラフィックスパイプライン
     */
    VkPipeline GetPipeline(csmInt32 shaderIndex, csmInt32 blendIndex)
    {
        if(_pipelineResource[shaderIndex] == NULL)
        {
            return NULL;
        }
        return _pipelineResource[shaderIndex]->GetPipeline(blendIndex);
    }

    /**
     * @brief   指定したブレンド方法のパイプラインレイアウトを取得する
     *
     * @param[in]   shaderIndex         ->  シェーダインデックス
     * @param[in]   blendIndex          ->  ブレンドモードのインデックス
     *
     * @return  指定したブレンド方法のパイプラインレイアウト
     */
    VkPipelineLayout GetPipelineLayout(csmInt32 shaderIndex, csmInt32 blendIndex)
    {
        if(_pipelineResource[shaderIndex] == NULL)
        {
            return NULL;
        }
        return _pipelineResource[shaderIndex]->GetPipelineLayout(blendIndex);
    }

    /**
     * @brief   インスタンスを取得する（シングルトン）
     *
     * @return  インスタンスのポインタ
     */
    static CubismPipeline_Vulkan* GetInstance();

    /**
     * @brief   リソースを開放する
     */
    void ReleaseShaderProgram();

private:
    csmVector<PipelineResource*> _pipelineResource;
};

/**
 * @brief   Vulkan用の描画命令を実装したクラス
 *
 */
class CubismRenderer_Vulkan : public CubismRenderer
{
    friend class CubismClippingManager_Vulkan;
    friend class CubismRenderer;

    /**
     * @brief   モデル用ユニフォームバッファオブジェクトの中身を保持する構造体
     */
    struct ModelUBO
    {
        csmFloat32 projectionMatrix[16]; ///< シェーダープログラムに渡すデータ(ProjectionMatrix)
        csmFloat32 clipMatrix[16]; ///< シェーダープログラムに渡すデータ(ClipMatrix)
        csmFloat32 baseColor[4]; ///< シェーダープログラムに渡すデータ(BaseColor)
        csmFloat32 multiplyColor[4]; ///< シェーダープログラムに渡すデータ(MultiplyColor)
        csmFloat32 screenColor[4]; ///< シェーダープログラムに渡すデータ(ScreenColor)
        csmFloat32 channelFlag[4]; ///< シェーダープログラムに渡すデータ(ChannelFlag)
    };

    /**
     * @brief   ディスクリプタセットとUBOを保持する構造体
     *          コマンドバッファでまとめて描画する場合、ディスクリプタセットをバインド後に
     *          更新してはならない。<br>
     *          マスクされる描画対象のみ、フレームバッファをディスクリプタセットに設定する必要があるが、
     *          その際バインド済みのディスクリプタセットを更新しないよう、別でマスクされる用のディスクリプタセットを用意する。
     */
    struct Descriptor
    {
        CubismBufferVulkan uniformBuffer; ///< ユニフォームバッファ
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE; ///< ディスクリプタセット
        bool isDescriptorSetUpdated = false; ///< ディスクリプタセットが更新されたか
        VkDescriptorSet descriptorSetMasked = VK_NULL_HANDLE; ///< マスクされる描画対象用のディスクリプタセット
        bool isDescriptorSetMaskedUpdated = false; ///< マスクされる描画対象用のディスクリプタセットが更新されたか
    };

protected:
    /**
     * @brief   コンストラクタ
     */
    CubismRenderer_Vulkan();

    /**
     * @brief   デストラクタ
     */
    ~CubismRenderer_Vulkan() override;

    /**
     * @brief   レンダラが保持する静的なリソースを解放する。
     */
    static void DoStaticRelease();

public:

    /**
     * @brief   コマンドの記録を開始する。
     *
     * @return  記録を開始したコマンドバッファ
     */
    VkCommandBuffer BeginSingleTimeCommands();

    /**
     * @brief   コマンドを実行する。
     *
     * @param[in]   commandBuffer                   -> コマンドバッファ
     * @param[in]   signalUpdateFinishedSemaphore   -> コマンド終了時にシグナルを出すセマフォ
     * @param[in]   waitUpdateFinishedSemaphore     -> コマンド実行前に待つセマフォ
     */
    void SubmitCommand(VkCommandBuffer commandBuffer, VkSemaphore signalUpdateFinishedSemaphore = VK_NULL_HANDLE, VkSemaphore waitUpdateFinishedSemaphore = VK_NULL_HANDLE);

    /**
     * @brief    レンダラを作成するための各種設定
     *
     *           モデルを読み込む前に一度だけ呼び出す
     *
     * @param[in]   device              -> 論理デバイス
     * @param[in]   physicalDevice      -> 物理デバイス
     * @param[in]   commandPool         -> コマンドプール
     * @param[in]   queue               -> キュー
     * @param[in]   swapchainImageCount -> スワップチェーンのイメージ数
     * @param[in]   extent              -> 描画解像度
     * @param[in]   imageView           -> イメージビュー
     * @param[in]   imageFormat         -> 描画対象のフォーマット
     * @param[in]   depthFormat         -> 深度フォーマット
     */
    static void InitializeConstantSettings(VkDevice device, VkPhysicalDevice physicalDevice,
                                           VkCommandPool commandPool, VkQueue queue,
                                           csmUint32 swapchainImageCount, VkExtent2D extent,
                                           VkImageView imageView, VkFormat imageFormat, VkFormat depthFormat);

    /**
     * @brief    レンダーターゲット変更を有効にする
     */
    static void EnableChangeRenderTarget();

    /**
     * @brief    レンダリング対象の指定
     *
     * @param[in]   image          -> イメージ
     * @param[in]   view           -> イメージビュー
     * @param[in]   format         -> フォーマット
     * @param[in]   extent         -> 描画解像度
     */
    static void SetRenderTarget(VkImage image, VkImageView view, VkFormat format, VkExtent2D extent);

    /**
     * @brief   描画対象の解像度を設定する
     *
     * @param[in]  extent 　　-> 描画解像度
     */
    static void SetImageExtent(VkExtent2D extent);

    /**
     * @brief   コマンドバッファを作成する。
     */
    void CreateCommandBuffer();

    /**
     * @brief   空の頂点バッファを作成する。
     */
    void CreateVertexBuffer();

    /**
     * @brief   インデックスバッファを作成する。
     */
    void CreateIndexBuffer();

    /**
     * @brief   ユニフォームバッファとディスクリプタセットを作成する。
     *          ディスクリプタセットレイアウトはユニフォームバッファ1つとモデル用テクスチャ1つとマスク用テクスチャ1つを指定する。
     */
    void CreateDescriptorSets();

    /**
     * @brief  深度バッファを作成する。
     */
    void CreateDepthBuffer();

    /**
     * @brief   レンダラーを初期化する。
     */
    void InitializeRenderer();

    /**
     * @brief    レンダラの初期化処理を実行する<br>
     *           引数に渡したモデルからレンダラの初期化処理に必要な情報を取り出すことができる
     *
     * @param[in]  model -> モデルのインスタンス
     */
    void Initialize(Framework::CubismModel* model) override;

    /**
     * @brief    レンダラの初期化処理を実行する<br>
     *           引数に渡したモデルからレンダラの初期化処理に必要な情報を取り出すことができる
     *
     * @param[in]  model -> モデルのインスタンス
     * @param[in]  maskBufferCount -> マスクバッファの数
     */
    void Initialize(Framework::CubismModel* model, csmInt32 maskBufferCount) override;

    /**
     * @brief   頂点バッファを更新する。
     * @param[in]   drawAssign    -> 描画インデックス
     * @param[in]   vcount        -> 頂点数
     * @param[in]   varray        -> 頂点配列
     * @param[in]   uvarray       -> uv配列
     * @param[in]   commandBuffer -> コマンドバッファ
     */
    void CopyToBuffer(csmInt32 drawAssign, const csmInt32 vcount, const csmFloat32* varray, const csmFloat32* uvarray,
                      VkCommandBuffer commandBuffer);

    /**
     * @brief   行列を更新する
     *
     * @param[in]   vkMat4     -> 更新する行列
     * @param[in]   cubismMat  -> 新しい値
     */
    static void UpdateMatrix(csmFloat32 vkMat4[16], CubismMatrix44 cubismMat);

    /**
     * @brief   カラーベクトルを更新する
     *
     * @param[in]   vkVec4     -> 更新するカラーベクトル
     * @param[in]   r, g, b, a -> 新しい値
     */
    void UpdateColor(csmFloat32 vkVec4[4], csmFloat32 r, csmFloat32 g, csmFloat32 b, csmFloat32 a);

    /**
     * @brief  ディスクリプタセットを更新する
     * @param[in]   descriptor    -> 1つのシェーダーが使用するディスクリプタセットとUBO
     * @param[in]   textureIndex  -> テクスチャインデックス
     * @param[in]   isMasked      -> 描画対象がマスクされるか
     */
    void UpdateDescriptorSet(Descriptor& descriptor, csmUint32 textureIndex, bool isMasked);

    /**
     * @brief   メッシュ描画を実行する
     *
     * @param[in]   model                 ->  描画対象のモデル
     * @param[in]   index                 ->  描画オブジェクトのインデックス
     * @param[in]   cmdBuffer             ->  フレームバッファ関連のコマンドバッファ
     */
    void ExecuteDrawForDraw(const CubismModel& model, const csmInt32 index, VkCommandBuffer& cmdBuffer);

    /**
     * @brief   マスク描画を実行する
     *
     * @param[in]   model                 ->  描画対象のモデル
     * @param[in]   index                 ->  描画オブジェクトのインデックス
     * @param[in]   cmdBuffer             ->  フレームバッファ関連のコマンドバッファ
     */
    void ExecuteDrawForMask(const CubismModel& model, const csmInt32 index, VkCommandBuffer& cmdBuffer);

    /**
     * @brief   [オーバーライド]<br>
     *           描画オブジェクト（アートメッシュ）を描画する。<br>
     * @param[in]   model                 ->  描画対象のモデル
     * @param[in]   index           ->  描画メッシュのインデックス
     * @param[in]   commandBuffer    ->  コマンドバッファ
     * @param[in]   updateCommandBuffer ->  更新用コマンドバッファ
     */
    void DrawMeshVulkan(const CubismModel& model, const csmInt32 index,
                        VkCommandBuffer commandBuffer, VkCommandBuffer updateCommandBuffer);

    /**
     * @brief   レンダリング開始
     *
     * @param[in]   drawCommandBuffer       ->  コマンドバッファ
     * @param[in]   isResume                ->  レンダリング再開かのフラグ
     */
    void BeginRendering(VkCommandBuffer drawCommandBuffer, bool isResume);

    /**
     * @brief   レンダリング終了
     *
     * @param[in]   drawCommandBuffer       ->  コマンドバッファ
     */
    void EndRendering(VkCommandBuffer drawCommandBuffer);

    /**
     * @brief   モデルを描画する実際の処理
     *
     */
    void DoDrawModel() override;

    /**
     * @brief   描画完了後の追加処理。<br>
     *          マルチバッファリングに必要な処理を実装している。
     */
    void PostDraw();

    /**
     * @brief   モデル描画直前のステートを保持する
     */
    void SaveProfile() override {}

    /**
     * @brief   モデル描画直前のステートを保持する
     */
    void RestoreProfile() override {}

    /**
     * @brief   マスクテクスチャに描画するクリッピングコンテキストをセットする。
     *
     * @param[in]  clip ->  クリッピングコンテキスト
     */
    void SetClippingContextBufferForMask(CubismClippingContext_Vulkan* clip);

    /**
     * @brief   マスクテクスチャに描画するクリッピングコンテキストを取得する。
     *
     * @return  マスクテクスチャに描画するクリッピングコンテキスト
     */
    CubismClippingContext_Vulkan* GetClippingContextBufferForMask() const;

    /**
     * @brief   画面上に描画するクリッピングコンテキストをセットする。
     *
     * @param[in]  clip ->  クリッピングコンテキスト
     */
    void SetClippingContextBufferForDraw(CubismClippingContext_Vulkan* clip);

    /**
     * @brief   画面上に描画するクリッピングコンテキストを取得する。
     *
     * @return  画面上に描画するクリッピングコンテキスト
     */
    CubismClippingContext_Vulkan* GetClippingContextBufferForDraw() const;

    /**
     * @brief   テクスチャマネージャーで作成したものをバインドする
     *
     * @param[in]   image         ->  テクスチャビューやサンプラーを保持しているインスタンス
     */
    void BindTexture(CubismImageVulkan& image);

    /**
     * @brief  クリッピングマスクバッファのサイズを設定する
     *
     * @param[in]  width  -> クリッピングマスクバッファの横幅
     * @param[in]  height -> クリッピングマスクバッファの立幅
     *
     */
    void SetClippingMaskBufferSize(csmFloat32 width, csmFloat32 height);

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
     * @param[in] backbufferNum  -> バックバッファの番号
     * @param[in] offscreenIndex -> オフスクリーンのインデックス
     *
     * @return クリッピングマスクのバッファへのポインタ
     *
     */
    CubismOffscreenSurface_Vulkan* GetMaskBuffer(csmUint32 backbufferNum, csmInt32 offscreenIndex);

private:

    /**
     * @brief  色定数バッファを設定する
     *
     * @param[in]   ubo                   ->  ユニフォームバッファ
     * @param[in]   baseColor             ->  ベースカラー
     * @param[in]   multiplyColor         ->  乗算カラー
     * @param[in]   screenColor           ->  スクリーンカラー
     */
    void SetColorUniformBuffer(ModelUBO& ubo, const CubismTextureColor& baseColor,
                               const CubismTextureColor& multiplyColor, const CubismTextureColor& screenColor);

    /**
     * @brief  頂点バッファとインデックスバッファをバインドする
     *
     * @param[in]   index            ->  描画メッシュのインデックス
     * @param[in]   cmdBuffer        ->  コマンドバッファ
     */
    void BindVertexAndIndexBuffers(const csmInt32 index, VkCommandBuffer& cmdBuffer);

    /**
     * @brief  描画に使用するカラーチャンネルを設定
     *
     * @param[in]   ubo              ->  ユニフォームバッファ
     * @param[in]   contextBuffer    ->  描画コンテキスト
     */
    void SetColorChannel(ModelUBO& ubo, CubismClippingContext_Vulkan* contextBuffer);

    PFN_vkCmdSetCullModeEXT vkCmdSetCullModeEXT;

    CubismRenderer_Vulkan(const CubismRenderer_Vulkan&);
    CubismRenderer_Vulkan& operator=(const CubismRenderer_Vulkan&);

    CubismClippingManager_Vulkan* _clippingManager; ///< クリッピングマスク管理オブジェクト
    csmVector<csmInt32> _sortedDrawableIndexList; ///< 描画オブジェクトのインデックスを描画順に並べたリスト
    CubismClippingContext_Vulkan* _clippingContextBufferForMask; ///< マスクテクスチャに描画するためのクリッピングコンテキスト
    CubismClippingContext_Vulkan* _clippingContextBufferForDraw; ///< 画面上描画するためのクリッピングコンテキスト

    csmUint32 _commandBufferCurrent; ///< スワップチェーン用に使用中のバッファインデックス

    csmVector<csmVector<CubismOffscreenSurface_Vulkan>> _offscreenFrameBuffers; ///< マスク描画用のフレームバッファ
    csmVector<csmVector<CubismBufferVulkan>> _vertexBuffers; ///< 頂点バッファ
    csmVector<csmVector<CubismBufferVulkan>> _stagingBuffers; ///< 頂点バッファを更新する際に使うステージングバッファ
    csmVector<csmVector<CubismBufferVulkan>> _indexBuffers; ///< インデックスバッファ

    VkDescriptorPool _descriptorPool; ///< ディスクリプタプール
    VkDescriptorSetLayout _descriptorSetLayout; ///< ディスクリプタセットのレイアウト
    csmVector<csmVector<Descriptor>> _descriptorSets; ///< ディスクリプタ管理オブジェクト

    csmVector<CubismImageVulkan> _textures; ///< モデルが使うテクスチャ
    CubismImageVulkan _depthImage; ///< オフスクリーンの色情報を保持する深度画像
    VkClearValue _clearColor; ///< クリアカラー

    csmVector<VkSemaphore> _updateFinishedSemaphores; ///< セマフォ
    csmVector<VkCommandBuffer> _updateCommandBuffers; ///< 更新用コマンドバッファ
    csmVector<VkCommandBuffer> _drawCommandBuffers; ///< 描画用コマンドバッファ
};
}}}}

//------------ LIVE2D NAMESPACE ------------
