/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "CubismRenderer_Vulkan.hpp"
#include "Math/CubismMatrix44.hpp"
#include "Type/csmVector.hpp"
#include "Model/CubismModel.hpp"

//------------ LIVE2D NAMESPACE ------------
namespace Live2D { namespace Cubism { namespace Framework { namespace Rendering {
// 各種静的変数
namespace {
bool s_useRenderTarget = false;

VkDevice s_device = VK_NULL_HANDLE;
VkPhysicalDevice s_physicalDevice = VK_NULL_HANDLE;
VkCommandPool s_commandPool = VK_NULL_HANDLE;
VkQueue s_queue = VK_NULL_HANDLE;
csmUint32 s_bufferSetNum = 0;

// 描画対象情報
VkExtent2D s_renderExtent;
VkImage s_renderImage;
VkImageView s_imageView;
VkFormat s_imageFormat;
VkFormat s_depthFormat;
}

VkViewport GetViewport(csmFloat32 width, csmFloat32 height, csmFloat32 minDepth, csmFloat32 maxDepth)
{
    VkViewport viewport{};
    viewport.width = width;
    viewport.height = height;
    viewport.minDepth = minDepth;
    viewport.maxDepth = maxDepth;
    return viewport;
}

VkRect2D GetScissor(csmFloat32 offsetX, csmFloat32 offsetY, csmFloat32 width, csmFloat32 height)
{
    VkRect2D rect{};
    rect.offset.x = offsetX;
    rect.offset.y = offsetY;
    rect.extent.height = height;
    rect.extent.width = width;
    return rect;
}

/*********************************************************************************************************************
*                                      CubismClippingManager_Vulkan
********************************************************************************************************************/

void CubismClippingManager_Vulkan::SetupClippingContext(CubismModel& model, VkCommandBuffer commandBuffer,
                                                        VkCommandBuffer updateCommandBuffer,
                                                        CubismRenderer_Vulkan* renderer, csmInt32 offscreenCurrent)
{
    // 全てのクリッピングを用意する
    // 同じクリップ（複数の場合はまとめて１つのクリップ）を使う場合は１度だけ設定する
    csmInt32 usingClipCount = 0;

    for (csmUint32 clipIndex = 0; clipIndex < _clippingContextListForMask.GetSize(); clipIndex++)
    {
        // １つのクリッピングマスクに関して
        CubismClippingContext_Vulkan* cc = _clippingContextListForMask[clipIndex];

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
    // 後の計算のためにインデックスの最初をセット
    _currentMaskBuffer = renderer->GetMaskBuffer(offscreenCurrent, 0);

    // 1が無効（描かれない）領域、0が有効（描かれる）領域。（シェーダで Cd*Csで0に近い値をかけてマスクを作る。1をかけると何も起こらない）
    _currentMaskBuffer->BeginDraw(commandBuffer, 1.0f, 1.0f, 1.0f, 1.0f);

    // 生成したFrameBufferと同じサイズでビューポートを設定
    const VkViewport viewport = GetViewport(
        static_cast<csmFloat32>(_clippingMaskBufferSize.X),
        static_cast<csmFloat32>(_clippingMaskBufferSize.Y),
        0.0, 1.0
    );
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    const VkRect2D rect = GetScissor(
        0.0, 0.0,
        static_cast<csmFloat32>(_clippingMaskBufferSize.X),
        static_cast<csmFloat32>(_clippingMaskBufferSize.Y)
    );
    vkCmdSetScissor(commandBuffer, 0, 1, &rect);

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
        CubismClippingContext_Vulkan* clipContext = _clippingContextListForMask[clipIndex];
        csmRectF* allClippedDrawRect = clipContext->_allClippedDrawRect; //このマスクを使う、全ての描画オブジェクトの論理座標上の囲み矩形
        csmRectF* layoutBoundsOnTex01 = clipContext->_layoutBounds; //この中にマスクを収める
        const csmFloat32 MARGIN = 0.05f;

        // clipContextに設定したオフスクリーンサーフェイスをインデックスで取得
        CubismOffscreenSurface_Vulkan* clipContextOffscreenSurface = renderer->GetMaskBuffer(offscreenCurrent, clipContext->_bufferIndex);

        // 現在のオフスクリーンサーフェイスがclipContextのものと異なる場合
        if (_currentMaskBuffer != clipContextOffscreenSurface)
        {
            _currentMaskBuffer->EndDraw(commandBuffer);
            _currentMaskBuffer = clipContextOffscreenSurface;
            // マスク用RenderTextureをactiveにセット
            _currentMaskBuffer->BeginDraw(commandBuffer, 1.0f, 1.0f, 1.0f, 1.0f);
        }

        // モデル座標上の矩形を、適宜マージンを付けて使う
        _tmpBoundsOnModel.SetRect(allClippedDrawRect);
        _tmpBoundsOnModel.Expand(allClippedDrawRect->Width * MARGIN, allClippedDrawRect->Height * MARGIN);
        //########## 本来は割り当てられた領域の全体を使わず必要最低限のサイズがよい
        // シェーダ用の計算式を求める。回転を考慮しない場合は以下のとおり
        // movePeriod' = movePeriod * scaleX + offX     [[ movePeriod' = (movePeriod - tmpBoundsOnModel.movePeriod)*scale + layoutBoundsOnTex01.movePeriod ]]
        csmFloat32 scaleX = layoutBoundsOnTex01->Width / _tmpBoundsOnModel.Width;
        csmFloat32 scaleY = layoutBoundsOnTex01->Height / _tmpBoundsOnModel.Height;

        // マスク生成時に使う行列を求める
        createMatrixForMask(false, layoutBoundsOnTex01, scaleX, scaleY);

        clipContext->_matrixForMask.SetMatrix(_tmpMatrixForMask.GetArray());
        clipContext->_matrixForDraw.SetMatrix(_tmpMatrixForDraw.GetArray());

        // 実際の描画を行う
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

            // レンダーパス開始時にマスクはクリアされているのでクリアする必要なし
            // 今回専用の変換を適用して描く
            // チャンネルも切り替える必要がある(A,R,G,B)
            renderer->SetClippingContextBufferForMask(clipContext);
            renderer->DrawMeshVulkan(model, clipDrawIndex, commandBuffer, updateCommandBuffer);
        }
    }
    // --- 後処理 ---
    _currentMaskBuffer->EndDraw(commandBuffer);
    renderer->SetClippingContextBufferForMask(NULL);
}

/*********************************************************************************************************************
*                                      CubismClippingContext_Vulkan
********************************************************************************************************************/

CubismClippingContext_Vulkan::CubismClippingContext_Vulkan(
    CubismClippingManager<CubismClippingContext_Vulkan, CubismOffscreenSurface_Vulkan>* manager, CubismModel& model,
    const csmInt32* clippingDrawableIndices, csmInt32 clipCount)
    : CubismClippingContext(clippingDrawableIndices, clipCount)
{
    _isUsing = false;

    _owner = manager;
}

CubismClippingContext_Vulkan::~CubismClippingContext_Vulkan()
{}

CubismClippingManager<CubismClippingContext_Vulkan, CubismOffscreenSurface_Vulkan>*
CubismClippingContext_Vulkan::GetClippingManager()
{
    return _owner;
}

/*********************************************************************************************************************
*                                      CubismPipeline_Vulkan
********************************************************************************************************************/

namespace {
const csmInt32 ShaderCount = 19;
///<シェーダの数 = マスク生成用 + (通常 + 加算 + 乗算) * (マスク無 + マスク有 + マスク有反転 + マスク無の乗算済アルファ対応版 + マスク有の乗算済アルファ対応版 + マスク有反転の乗算済アルファ対応版)
CubismPipeline_Vulkan* s_pipelineManager;
}

CubismPipeline_Vulkan::CubismPipeline_Vulkan()
{}

CubismPipeline_Vulkan::~CubismPipeline_Vulkan()
{
    ReleaseShaderProgram();
}

VkShaderModule CubismPipeline_Vulkan::PipelineResource::CreateShaderModule(VkDevice device, std::string filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        CubismLogError("failed to open file!");
        return NULL;
    }

    csmInt32 fileSize = (csmInt32)file.tellg();
    csmVector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.GetPtr(), fileSize);
    file.close();

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = fileSize;
    createInfo.pCode = reinterpret_cast<const csmUint32*>(buffer.GetPtr());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
        CubismLogError("failed to create shader module!");
        return NULL;
    }

    return shaderModule;
}

bool CubismPipeline_Vulkan::PipelineResource::CreateGraphicsPipeline(std::string vertFileName, std::string fragFileName,
                                                                   VkDescriptorSetLayout descriptorSetLayout)
{
    VkShaderModule vertShaderModule = CreateShaderModule(s_device, vertFileName);
    if(vertShaderModule == NULL)
    {
        return false;
    }

    VkShaderModule fragShaderModule = CreateShaderModule(s_device, fragFileName);
    if(fragShaderModule == NULL)
    {
        return false;
    }

    _pipeline.Resize(4);
    _pipelineLayout.Resize(4);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkVertexInputBindingDescription bindingDescription = ModelVertex::GetBindingDescription();
    VkVertexInputAttributeDescription attributeDescriptions[2];
    ModelVertex::GetAttributeDescriptions(attributeDescriptions);
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = sizeof(attributeDescriptions) / sizeof(attributeDescriptions[0]);
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // 通常
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT
            | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

    VkDynamicState dynamicState[3] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_CULL_MODE};

    VkPipelineDynamicStateCreateInfo dynamicStateCI{};
    dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCI.dynamicStateCount = sizeof(dynamicState) / sizeof(dynamicState[0]);
    dynamicStateCI.pDynamicStates = dynamicState;

    VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo{};
    pipelineDepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    pipelineDepthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
    pipelineDepthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
    pipelineDepthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    pipelineDepthStencilStateCreateInfo.back.compareOp = VK_COMPARE_OP_ALWAYS;

    if (vkCreatePipelineLayout(s_device, &pipelineLayoutInfo, nullptr, &_pipelineLayout[Blend_Normal]) != VK_SUCCESS)
    {
        CubismLogError("failed to create _pipeline layout!");
    }

    VkPipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachmentFormats = &s_imageFormat;
    renderingInfo.depthAttachmentFormat = s_depthFormat;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = _pipelineLayout[Blend_Normal];
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.pDynamicState = &dynamicStateCI;
    pipelineInfo.pDepthStencilState = &pipelineDepthStencilStateCreateInfo;
    pipelineInfo.pNext = &renderingInfo;

    if (vkCreateGraphicsPipelines(s_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline[Blend_Normal]) !=
        VK_SUCCESS)
    {
        CubismLogError("failed to create graphics _pipeline!");
    }

    // 加算
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    if (vkCreatePipelineLayout(s_device, &pipelineLayoutInfo, nullptr, &_pipelineLayout[Blend_Add]) != VK_SUCCESS)
    {
        CubismLogError("failed to create _pipeline layout!");
    }
    pipelineInfo.layout = _pipelineLayout[Blend_Add];
    if (vkCreateGraphicsPipelines(s_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline[Blend_Add]) !=
        VK_SUCCESS)
    {
        CubismLogError("failed to create graphics _pipeline!");
    }

    // 乗算
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    if (vkCreatePipelineLayout(s_device, &pipelineLayoutInfo, nullptr, &_pipelineLayout[Blend_Mult]) != VK_SUCCESS)
    {
        CubismLogError("failed to create _pipeline layout!");
    }
    pipelineInfo.layout = _pipelineLayout[Blend_Mult];
    if (vkCreateGraphicsPipelines(s_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline[Blend_Mult]) !=
        VK_SUCCESS)
    {
        CubismLogError("failed to create graphics _pipeline!");
    }

    // マスク
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    if (vkCreatePipelineLayout(s_device, &pipelineLayoutInfo, nullptr, &_pipelineLayout[Blend_Mask]) != VK_SUCCESS)
    {
        CubismLogError("failed to create _pipeline layout!");
    }
    renderingInfo.pColorAttachmentFormats = &s_imageFormat;
    pipelineInfo.layout = _pipelineLayout[Blend_Mask];
    if (vkCreateGraphicsPipelines(s_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline[Blend_Mask]) !=
        VK_SUCCESS)
    {
        CubismLogError("failed to create graphics _pipeline!");
    }

    vkDestroyShaderModule(s_device, vertShaderModule, nullptr);
    vkDestroyShaderModule(s_device, fragShaderModule, nullptr);

    return true;
}

void CubismPipeline_Vulkan::PipelineResource::Release()
{
    for (csmInt32 i = 0; i < 4; i++)
    {
        if(_pipeline.GetPtr() != NULL)
        {
            vkDestroyPipeline(s_device, _pipeline[i], nullptr);
        }
        if(_pipelineLayout.GetPtr() != NULL)
        {
            vkDestroyPipelineLayout(s_device, _pipelineLayout[i], nullptr);
        }
    }
    _pipeline.Clear();
    _pipelineLayout.Clear();
}

void CubismPipeline_Vulkan::CreatePipelines(VkDescriptorSetLayout descriptorSetLayout)
{
    //パイプラインを作成済みの場合は生成する必要なし
    if (_pipelineResource.GetSize() != 0)
    {
        return;
    }

    _pipelineResource.Resize(ShaderCount);
    for (csmInt32 i = 0; i < 7; i++)
    {
        _pipelineResource[i] = CSM_NEW PipelineResource();
    }

    std::string shaderFiles[7][2] = {
        {"FrameworkShaders/VertShaderSrcSetupMask.spv", "FrameworkShaders/FragShaderSrcSetupMask.spv"},
        {"FrameworkShaders/VertShaderSrc.spv", "FrameworkShaders/FragShaderSrc.spv"},
        {"FrameworkShaders/VertShaderSrcMasked.spv", "FrameworkShaders/FragShaderSrcMask.spv"},
        {"FrameworkShaders/VertShaderSrcMasked.spv", "FrameworkShaders/FragShaderSrcMaskInverted.spv"},
        {"FrameworkShaders/VertShaderSrc.spv", "FrameworkShaders/FragShaderSrcPremultipliedAlpha.spv"},
        {"FrameworkShaders/VertShaderSrcMasked.spv", "FrameworkShaders/FragShaderSrcMaskPremultipliedAlpha.spv"},
        {"FrameworkShaders/VertShaderSrcMasked.spv", "FrameworkShaders/FragShaderSrcMaskInvertedPremultipliedAlpha.spv"},
    };

    for(int i = 0; i < 7; i++)
    {
        if(!_pipelineResource[i]->CreateGraphicsPipeline(shaderFiles[i][0],shaderFiles[i][1], descriptorSetLayout))
        {
            _pipelineResource[i] = NULL;
        }
    }

    _pipelineResource[7] = _pipelineResource[1];
    _pipelineResource[8] = _pipelineResource[2];
    _pipelineResource[9] = _pipelineResource[3];
    _pipelineResource[10] = _pipelineResource[4];
    _pipelineResource[11] = _pipelineResource[5];
    _pipelineResource[12] = _pipelineResource[6];

    _pipelineResource[13] = _pipelineResource[1];
    _pipelineResource[14] = _pipelineResource[2];
    _pipelineResource[15] = _pipelineResource[3];
    _pipelineResource[16] = _pipelineResource[4];
    _pipelineResource[17] = _pipelineResource[5];
    _pipelineResource[18] = _pipelineResource[6];
}

CubismPipeline_Vulkan* CubismPipeline_Vulkan::GetInstance()
{
    if (s_pipelineManager == NULL)
    {
        s_pipelineManager = CSM_NEW CubismPipeline_Vulkan();
    }
    return s_pipelineManager;
}

void CubismPipeline_Vulkan::ReleaseShaderProgram()
{
    for (csmInt32 i = 0; i < 7; i++)
    {
        if(_pipelineResource[i] != NULL)
        {
            _pipelineResource[i]->Release();
            CSM_DELETE(_pipelineResource[i]);
            _pipelineResource[i] = NULL;
        }
    }
}

/*********************************************************************************************************************
*                                       CubismRenderer_Vulkan
********************************************************************************************************************/

CubismRenderer* CubismRenderer::Create()
{
    return CSM_NEW CubismRenderer_Vulkan;
}

void CubismRenderer::StaticRelease()
{
    CubismRenderer_Vulkan::DoStaticRelease();
}

CubismRenderer_Vulkan::CubismRenderer_Vulkan() :
                                               vkCmdSetCullModeEXT()
                                               , _clippingManager(NULL)
                                               , _clippingContextBufferForMask(NULL)
                                               , _clippingContextBufferForDraw(NULL)
                                               , _descriptorPool(VK_NULL_HANDLE)
                                               , _descriptorSetLayout(VK_NULL_HANDLE)
                                               , _clearColor()
                                               , _commandBufferCurrent(0)
{}

CubismRenderer_Vulkan::~CubismRenderer_Vulkan()
{
    CSM_DELETE_SELF(CubismClippingManager_Vulkan, _clippingManager);

    // オフスクリーンを作成していたのなら開放
    for (csmUint32 buffer = 0; buffer < _offscreenFrameBuffers.GetSize(); buffer++)
    {
        for (csmInt32 i = 0; i < _offscreenFrameBuffers[buffer].GetSize(); i++)
        {
            _offscreenFrameBuffers[buffer][i].DestroyOffscreenSurface(s_device);
        }
        _offscreenFrameBuffers[buffer].Clear();
    }
    _offscreenFrameBuffers.Clear();
    _depthImage.Destroy(s_device);

    // セマフォ解放
    for (csmUint32 buffer = 0; buffer < s_bufferSetNum; buffer++)
    {
        vkDestroySemaphore(s_device, _updateFinishedSemaphores[buffer], nullptr);
    }

    // ディスクリプタ関連解放
    vkDestroyDescriptorPool(s_device, _descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(s_device, _descriptorSetLayout, nullptr);

    for (csmUint32 buffer = 0; buffer < s_bufferSetNum; buffer++)
    {
        for (csmInt32 i = 0; i < _descriptorSets[buffer].GetSize(); i++)
        {
            _descriptorSets[buffer][i].uniformBuffer.Destroy(s_device);
        }
    }

    // その他バッファ開放
    for (csmUint32 buffer = 0; buffer < s_bufferSetNum; buffer++)
    {
        for (csmUint32 drawAssign = 0; drawAssign < _vertexBuffers[buffer].GetSize(); drawAssign++)
        {
            _vertexBuffers[buffer][drawAssign].Destroy(s_device);
            _stagingBuffers[buffer][drawAssign].Destroy(s_device);
            _indexBuffers[buffer][drawAssign].Destroy(s_device);
        }
    }
}

void CubismRenderer_Vulkan::DoStaticRelease()
{
    if (s_pipelineManager)
    {
        CSM_DELETE_SELF(CubismPipeline_Vulkan, s_pipelineManager);
        s_pipelineManager = NULL;
    }
}

VkCommandBuffer CubismRenderer_Vulkan::BeginSingleTimeCommands()
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = s_commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(s_device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void CubismRenderer_Vulkan::SubmitCommand(VkCommandBuffer commandBuffer, VkSemaphore signalUpdateFinishedSemaphore, VkSemaphore waitUpdateFinishedSemaphore)
{
    vkEndCommandBuffer(commandBuffer);
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    if(waitUpdateFinishedSemaphore != VK_NULL_HANDLE)
    {
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &waitUpdateFinishedSemaphore;
        submitInfo.pWaitDstStageMask = waitStages;
        vkQueueSubmit(s_queue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(s_queue);
    }
    else if (signalUpdateFinishedSemaphore != VK_NULL_HANDLE)
    {
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &signalUpdateFinishedSemaphore;
        vkQueueSubmit(s_queue, 1, &submitInfo, VK_NULL_HANDLE);
    }
    else
    {
        vkQueueSubmit(s_queue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(s_queue);
        vkFreeCommandBuffers(s_device, s_commandPool, 1, &commandBuffer);
    }

}

void CubismRenderer_Vulkan::InitializeConstantSettings(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue queue,
                                                       csmUint32 swapchainImageCount, VkExtent2D extent, VkImageView imageView, VkFormat imageFormat, VkFormat depthFormat)
{
    s_device = device;
    s_physicalDevice = physicalDevice;
    s_commandPool = commandPool;
    s_queue = queue;
    s_bufferSetNum = swapchainImageCount;

    s_renderExtent = extent;
    s_imageView = imageView;
    s_imageFormat = imageFormat;
    s_depthFormat = depthFormat;
}

void CubismRenderer_Vulkan::EnableChangeRenderTarget()
{
    s_useRenderTarget = true;
}

void CubismRenderer_Vulkan::SetRenderTarget(VkImage image, VkImageView view, VkFormat format, VkExtent2D extent)
{
    s_renderImage = image;
    s_imageView = view;
    s_imageFormat = format;
    s_renderExtent = extent;
}

void CubismRenderer_Vulkan::SetImageExtent(VkExtent2D extent)
{
    s_renderExtent = extent;
}

void CubismRenderer_Vulkan::CreateCommandBuffer()
{
    _updateCommandBuffers.Resize(s_bufferSetNum);
    _drawCommandBuffers.Resize(s_bufferSetNum);
    for (csmUint32 buffer = 0; buffer < s_bufferSetNum; buffer++)
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = s_commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;
        if (vkAllocateCommandBuffers(s_device, &allocInfo, &_updateCommandBuffers[buffer]) != VK_SUCCESS ||
            vkAllocateCommandBuffers(s_device, &allocInfo, &_drawCommandBuffers[buffer]) != VK_SUCCESS)
        {
            CubismLogError("failed to allocate command buffers!");
        }
    }
}

void CubismRenderer_Vulkan::CreateVertexBuffer()
{
    const csmInt32 drawableCount = GetModel()->GetDrawableCount();
    _stagingBuffers.Resize(s_bufferSetNum);
    _vertexBuffers.Resize(s_bufferSetNum);

    for (csmUint32 buffer = 0; buffer < s_bufferSetNum; buffer++)
    {
        _stagingBuffers[buffer].Resize(drawableCount);
        _vertexBuffers[buffer].Resize(drawableCount);
        for (csmInt32 drawAssign = 0; drawAssign < drawableCount; drawAssign++)
        {
            const csmInt32 vcount = GetModel()->GetDrawableVertexCount(drawAssign);
            if (vcount != 0)
            {
                VkDeviceSize bufferSize = sizeof(ModelVertex) * vcount; // 総長 構造体サイズ*個数

                CubismBufferVulkan stagingBuffer;
                stagingBuffer.CreateBuffer(s_device, s_physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
                stagingBuffer.Map(s_device, bufferSize);
                _stagingBuffers[buffer][drawAssign] = stagingBuffer;

                CubismBufferVulkan vertexBuffer;
                vertexBuffer.CreateBuffer(s_device, s_physicalDevice, bufferSize,
                                        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                _vertexBuffers[buffer][drawAssign] = vertexBuffer;
            }
        }
    }
}

void CubismRenderer_Vulkan::CreateIndexBuffer()
{
    const csmInt32 drawableCount = GetModel()->GetDrawableCount();
    _indexBuffers.Resize(s_bufferSetNum);

    for (csmUint32 buffer = 0; buffer < s_bufferSetNum; buffer++)
    {
        _indexBuffers[buffer].Resize(drawableCount);
        for (csmInt32 drawAssign = 0; drawAssign < drawableCount; drawAssign++)
        {
            const csmInt32 icount = GetModel()->GetDrawableVertexIndexCount(drawAssign);
            if (icount != 0)
            {
                VkDeviceSize bufferSize = sizeof(uint16_t) * icount;
                const csmUint16* indices = GetModel()->GetDrawableVertexIndices(drawAssign);

                CubismBufferVulkan stagingBuffer;
                stagingBuffer.CreateBuffer(s_device, s_physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
                stagingBuffer.Map(s_device, bufferSize);
                stagingBuffer.MemCpy(indices, bufferSize);
                stagingBuffer.UnMap(s_device);

                CubismBufferVulkan indexBuffer;
                indexBuffer.CreateBuffer(s_device, s_physicalDevice, bufferSize,
                                        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

                VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

                VkBufferCopy copyRegion{};
                copyRegion.size = bufferSize;
                vkCmdCopyBuffer(commandBuffer, stagingBuffer.GetBuffer(), indexBuffer.GetBuffer(), 1, &copyRegion);
                SubmitCommand(commandBuffer);
                _indexBuffers[buffer][drawAssign] = indexBuffer;
                stagingBuffer.Destroy(s_device);
            }
        }
    }
}

void CubismRenderer_Vulkan::CreateDescriptorSets()
{
    // ディスクリプタプールの作成
    const csmInt32 drawableCount = GetModel()->GetDrawableCount();
    csmInt32 textureCount = 2;
    csmInt32 drawModeCount = 2; // マスクされる描画と通常の描画
    csmInt32 descriptorSetCountPerBuffer = drawableCount * drawModeCount;
    csmInt32 descriptorSetCount = s_bufferSetNum * descriptorSetCountPerBuffer;
    VkDescriptorPoolSize poolSizes[2]{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = descriptorSetCount;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = descriptorSetCount * textureCount; // drawableCount * 描画方法の数 * テクスチャの数

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = sizeof(poolSizes) / sizeof(poolSizes[0]);
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = descriptorSetCount;

    if (vkCreateDescriptorPool(s_device, &poolInfo, nullptr, &_descriptorPool) != VK_SUCCESS)
    {
        CubismLogError("failed to create descriptor pool!");
    }

    // ディスクリプタセットレイアウトの作成
    // ユニフォームバッファ用
    VkDescriptorSetLayoutBinding bindings[3];
    bindings[0].binding = 0;
    bindings[0].descriptorCount = 1;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].pImmutableSamplers = nullptr;
    bindings[0].stageFlags = VK_SHADER_STAGE_ALL;

    // キャラクターテクスチャ
    bindings[1].binding = 1;
    bindings[1].descriptorCount = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].pImmutableSamplers = nullptr;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // マスク用オフスクリーンに使用するテクスチャ
    bindings[2].binding = 2;
    bindings[2].descriptorCount = 1;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[2].pImmutableSamplers = nullptr;
    bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = sizeof(bindings) / sizeof(bindings[0]);
    layoutInfo.pBindings = bindings;

    if (vkCreateDescriptorSetLayout(s_device, &layoutInfo, nullptr, &_descriptorSetLayout) != VK_SUCCESS)
    {
        CubismLogError("failed to create descriptor set layout!");
    }

    // ディスクリプタセットの作成
    _descriptorSets.Resize(s_bufferSetNum);
    for (csmUint32 buffer = 0; buffer < s_bufferSetNum; buffer++)
    {
        _descriptorSets[buffer].Resize(drawableCount);
        for (csmInt32 drawAssign = 0; drawAssign < drawableCount; drawAssign++)
        {
            _descriptorSets[buffer][drawAssign].uniformBuffer.CreateBuffer(s_device, s_physicalDevice, sizeof(ModelUBO),
                                                                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            _descriptorSets[buffer][drawAssign].uniformBuffer.Map(s_device, VK_WHOLE_SIZE);
        }

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = _descriptorPool;
        allocInfo.descriptorSetCount = descriptorSetCountPerBuffer;
        csmVector<VkDescriptorSetLayout> layouts;
        for (csmInt32 i = 0; i < descriptorSetCountPerBuffer; i++)
        {
            layouts.PushBack(_descriptorSetLayout);
        }
        allocInfo.pSetLayouts = layouts.GetPtr();

        csmVector<VkDescriptorSet> descriptorSets;
        descriptorSets.Resize(descriptorSetCountPerBuffer);
        if (vkAllocateDescriptorSets(s_device, &allocInfo, descriptorSets.GetPtr()) != VK_SUCCESS)
        {
            CubismLogError("failed to allocate descriptor sets!");
        }

        for (csmInt32 drawAssign = 0; drawAssign < drawableCount; drawAssign++)
        {
            _descriptorSets[buffer][drawAssign].descriptorSet = descriptorSets[drawAssign * 2];
            _descriptorSets[buffer][drawAssign].descriptorSetMasked = descriptorSets[drawAssign * 2 + 1];
        }
    }
}

void CubismRenderer_Vulkan::CreateDepthBuffer()
{
    _depthImage.CreateImage(s_device, s_physicalDevice, s_renderExtent.width, s_renderExtent.height,
        1, s_depthFormat, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    _depthImage.CreateView(s_device, s_depthFormat,
        VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 1);
}

void CubismRenderer_Vulkan::InitializeRenderer()
{
    vkCmdSetCullModeEXT = reinterpret_cast<PFN_vkCmdSetCullMode>(vkGetDeviceProcAddr(s_device, "vkCmdSetCullModeEXT"));

    _updateFinishedSemaphores.Resize(s_bufferSetNum);
    for (csmUint32 buffer = 0; buffer < s_bufferSetNum; buffer++)
    {
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        vkCreateSemaphore(s_device, &semaphoreInfo, nullptr, &_updateFinishedSemaphores[buffer]);
    }

    CreateCommandBuffer();
    CreateVertexBuffer();
    CreateIndexBuffer();
    CreateDescriptorSets();
    CreateDepthBuffer();
    CubismPipeline_Vulkan::GetInstance()->CreatePipelines(_descriptorSetLayout);
}

void CubismRenderer_Vulkan::Initialize(CubismModel* model)
{
    Initialize(model, 1);
}

void CubismRenderer_Vulkan::Initialize(CubismModel* model, csmInt32 maskBufferCount)
{
    if (s_device == nullptr)
    {
        CubismLogError("Device has not been set.");
        CSM_ASSERT(0);
        return;
    }

    if (model->IsUsingMasking())
    {
        //モデルがマスクを使用している時のみにする
        _clippingManager = CSM_NEW CubismClippingManager_Vulkan();
        _clippingManager->Initialize(
            *model,
            maskBufferCount
        );
    }

    _sortedDrawableIndexList.Resize(model->GetDrawableCount(), 0);

    CubismRenderer::Initialize(model, maskBufferCount); // 親クラスの処理を呼ぶ

    // 1未満は1に補正する
    if (maskBufferCount < 1)
    {
        maskBufferCount = 1;
        CubismLogWarning(
            "The number of render textures must be an integer greater than or equal to 1. Set the number of render textures to 1.");
    }
    if (model->IsUsingMasking())
    {
        const csmInt32 bufferWidth = static_cast<csmInt32>(_clippingManager->GetClippingMaskBufferSize().X);
        const csmInt32 bufferHeight = static_cast<csmInt32>(_clippingManager->GetClippingMaskBufferSize().Y);

        _offscreenFrameBuffers.Clear();

        _offscreenFrameBuffers.Resize(s_bufferSetNum);
        for (csmUint32 buffer = 0; buffer < s_bufferSetNum; buffer++)
        {
            _offscreenFrameBuffers[buffer].Resize(maskBufferCount);
            // バックバッファ分確保
            for (csmInt32 i = 0; i < maskBufferCount; i++)
            {
                _offscreenFrameBuffers[buffer][i].CreateOffscreenSurface(s_device, s_physicalDevice, bufferWidth, bufferHeight,
                    s_imageFormat, s_depthFormat);
            }
        }
    }

    _commandBufferCurrent = 0;

    InitializeRenderer();
}

void CubismRenderer_Vulkan::CopyToBuffer(csmInt32 drawAssign, const csmInt32 vcount, const csmFloat32* varray,
                                         const csmFloat32* uvarray, VkCommandBuffer commandBuffer)
{
    csmVector<ModelVertex> vertices;

    for (csmInt32 ct = 0; ct < vcount * 2; ct += 2)
    {
        ModelVertex vertex;
        // モデルデータからのコピー
        vertex.pos.X = varray[ct + 0];
        vertex.pos.Y = varray[ct + 1];
        vertex.texCoord.X = uvarray[ct + 0];
        vertex.texCoord.Y = uvarray[ct + 1];
        vertices.PushBack(vertex);
    }
    csmUint32 bufferSize = sizeof(ModelVertex) * vertices.GetSize();
    _stagingBuffers[_commandBufferCurrent][drawAssign].MemCpy(vertices.GetPtr(), (size_t)bufferSize);

    VkBufferCopy copyRegion{};
    copyRegion.size = bufferSize;
    vkCmdCopyBuffer(commandBuffer, _stagingBuffers[_commandBufferCurrent][drawAssign].GetBuffer(), _vertexBuffers[_commandBufferCurrent][drawAssign].GetBuffer(), 1,
                    &copyRegion);
}

void CubismRenderer_Vulkan::UpdateMatrix(csmFloat32 vkMat16[16], CubismMatrix44 cubismMat)
{
    for (int i = 0; i < 16; i++)
    {
        vkMat16[i] = cubismMat.GetArray()[i];
    }
}

void CubismRenderer_Vulkan::UpdateColor(csmFloat32 vkVec4[4], csmFloat32 r, csmFloat32 g, csmFloat32 b, csmFloat32 a)
{
    vkVec4[0] = r;
    vkVec4[1] = g;
    vkVec4[2] = b;
    vkVec4[3] = a;
}

void CubismRenderer_Vulkan::UpdateDescriptorSet(Descriptor& descriptor, csmUint32 textureIndex, bool isMasked)
{
    VkBuffer uniformBuffer = descriptor.uniformBuffer.GetBuffer();
    // descriptorが更新されていない最初の1回のみ行う
    VkDescriptorSet descriptorSet;
    if (isMasked && !descriptor.isDescriptorSetMaskedUpdated)
    {
        descriptorSet = descriptor.descriptorSetMasked;
        descriptor.isDescriptorSetMaskedUpdated = true;
    }
    else if (!descriptor.isDescriptorSetUpdated)
    {
        descriptorSet = descriptor.descriptorSet;
        descriptor.isDescriptorSetUpdated = true;
    }
    else
    {
        return;
    }

    csmVector<VkWriteDescriptorSet> descriptorWrites{};

    VkDescriptorBufferInfo uniformBufferInfo{};
    uniformBufferInfo.buffer = uniformBuffer;
    uniformBufferInfo.offset = 0;
    uniformBufferInfo.range = VK_WHOLE_SIZE;
    VkWriteDescriptorSet ubo{};
    ubo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    ubo.dstSet = descriptorSet;
    ubo.dstBinding = 0;
    ubo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo.descriptorCount = 1;
    ubo.pBufferInfo = &uniformBufferInfo;
    descriptorWrites.PushBack(ubo);

    //テクスチャ1はキャラクターのテクスチャ、テクスチャ2はマスク用のオフスクリーンに使用するテクスチャ
    VkDescriptorImageInfo imageInfo1{};
    imageInfo1.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo1.imageView = _textures[textureIndex].GetView();
    imageInfo1.sampler = _textures[textureIndex].GetSampler();
    VkWriteDescriptorSet image1{};
    image1.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    image1.dstSet = descriptorSet;
    image1.dstBinding = 1;
    image1.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    image1.descriptorCount = 1;
    image1.pImageInfo = &imageInfo1;
    descriptorWrites.PushBack(image1);

    if (isMasked)
    {
        VkDescriptorImageInfo imageInfo2{};
        imageInfo2.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo2.imageView = _offscreenFrameBuffers[_commandBufferCurrent][GetClippingContextBufferForDraw()->
            _bufferIndex].GetTextureView();
        imageInfo2.sampler = _offscreenFrameBuffers[_commandBufferCurrent][GetClippingContextBufferForDraw()->
            _bufferIndex].GetTextureSampler();
        VkWriteDescriptorSet image2{};
        image2.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        image2.dstSet = descriptorSet;
        image2.dstBinding = 2;
        image2.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        image2.descriptorCount = 1;
        image2.pImageInfo = &imageInfo2;
        descriptorWrites.PushBack(image2);
    }

    vkUpdateDescriptorSets(s_device, descriptorWrites.GetSize(), descriptorWrites.GetPtr(), 0, NULL);
}

void CubismRenderer_Vulkan::ExecuteDrawForDraw(const CubismModel& model, const csmInt32 index, VkCommandBuffer& cmdBuffer)
{
    // パイプラインレイアウト設定用のインデックスを取得
    csmUint32 blendIndex = 0;
    csmUint32 shaderIndex = 0;
    const csmBool masked = GetClippingContextBufferForDraw() != NULL; // この描画オブジェクトはマスク対象か
    const csmBool invertedMask = model.GetDrawableInvertedMask(index);
    const csmInt32 offset = (masked ? (invertedMask ? 2 : 1) : 0) + (IsPremultipliedAlpha() ? 3 : 0);

    switch (model.GetDrawableBlendMode(index))
    {
    default:
        shaderIndex = ShaderNames_Normal + offset;
        blendIndex = Blend_Normal;
        break;

    case CubismRenderer::CubismBlendMode_Additive:
        shaderIndex = ShaderNames_Add + offset;
        blendIndex = Blend_Add;
        break;

    case CubismRenderer::CubismBlendMode_Multiplicative:
        shaderIndex = ShaderNames_Mult + offset;
        blendIndex = Blend_Mult;
        break;
    }

    VkPipelineLayout pipelineLayout = CubismPipeline_Vulkan::GetInstance()->GetPipelineLayout(shaderIndex, blendIndex);
    if(pipelineLayout == NULL)
    {
        return;
    }

    VkPipeline pipeline = CubismPipeline_Vulkan::GetInstance()->GetPipeline(shaderIndex, blendIndex);
    if(pipeline == NULL)
    {
        return;
    }


    Descriptor &descriptor = _descriptorSets[_commandBufferCurrent][index];
    ModelUBO ubo;
    if (masked)
    {
        // クリッピング用行列の設定
        UpdateMatrix(ubo.clipMatrix, GetClippingContextBufferForDraw()->_matrixForDraw); // テクスチャ座標の変換に使用するのでy軸の向きは反転しない

        // カラーチャンネルの設定
        SetColorChannel(ubo, GetClippingContextBufferForDraw());
    }

    // MVP行列の設定
    UpdateMatrix(ubo.projectionMatrix, GetMvpMatrix());

    // 色定数バッファの設定
    CubismTextureColor baseColor = GetModelColorWithOpacity(model.GetDrawableOpacity(index));
    CubismTextureColor multiplyColor = model.GetMultiplyColor(index);
    CubismTextureColor screenColor = model.GetScreenColor(index);
    SetColorUniformBuffer(ubo, baseColor, multiplyColor, screenColor);

    // ディスクリプタにユニフォームバッファをコピー
    descriptor.uniformBuffer.MemCpy(&ubo, sizeof(ModelUBO));

    // 頂点バッファの設定
    BindVertexAndIndexBuffers(index, cmdBuffer);

    // テクスチャインデックス取得
    csmInt32 textureIndex = model.GetDrawableTextureIndex(index);

    // ディスクリプタセットのバインド
    UpdateDescriptorSet(descriptor, textureIndex, masked);
    VkDescriptorSet* descriptorSet = (masked ? &_descriptorSets[_commandBufferCurrent][index].descriptorSetMasked
                                             : &_descriptorSets[_commandBufferCurrent][index].descriptorSet);
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipelineLayout, 0, 1,
                                descriptorSet, 0, nullptr);

    // パイプラインのバインド
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      pipeline);

    // 描画
    vkCmdDrawIndexed(cmdBuffer, model.GetDrawableVertexIndexCount(index), 1, 0, 0, 0);
}

void CubismRenderer_Vulkan::ExecuteDrawForMask(const CubismModel& model, const csmInt32 index, VkCommandBuffer& cmdBuffer)
{
    csmUint32 shaderIndex = ShaderNames_SetupMask;
    csmUint32 blendIndex = Blend_Mask;

    VkPipelineLayout pipelineLayout = CubismPipeline_Vulkan::GetInstance()->GetPipelineLayout(shaderIndex, blendIndex);
    if(pipelineLayout == NULL)
    {
        return;
    }

    VkPipeline pipeline = CubismPipeline_Vulkan::GetInstance()->GetPipeline(shaderIndex, blendIndex);
    if(pipeline == NULL)
    {
        return;
    }

    Descriptor &descriptor = _descriptorSets[_commandBufferCurrent][index];
    ModelUBO ubo;

    // クリッピング用行列の設定
    UpdateMatrix(ubo.clipMatrix, GetClippingContextBufferForMask()->_matrixForMask);

    // カラーチャンネルの設定
    SetColorChannel(ubo, GetClippingContextBufferForMask());

    // 色定数バッファの設定
    csmRectF *rect = GetClippingContextBufferForMask()->_layoutBounds;
    CubismTextureColor baseColor = {rect->X * 2.0f - 1.0f, rect->Y * 2.0f - 1.0f, rect->GetRight() * 2.0f - 1.0f, rect->GetBottom() * 2.0f - 1.0f};
    CubismTextureColor multiplyColor = model.GetMultiplyColor(index);
    CubismTextureColor screenColor = model.GetScreenColor(index);
    SetColorUniformBuffer(ubo, baseColor, multiplyColor, screenColor);

    // ディスクリプタにユニフォームバッファをコピー
    descriptor.uniformBuffer.MemCpy(&ubo, sizeof(ModelUBO));

    // 頂点バッファの設定
    BindVertexAndIndexBuffers(index, cmdBuffer);

    // テクスチャインデックス取得
    csmInt32 textureIndex = model.GetDrawableTextureIndex(index);

    // ディスクリプタセットのバインド
    UpdateDescriptorSet(descriptor, textureIndex, false);
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout, 0, 1,
                            &_descriptorSets[_commandBufferCurrent][index].descriptorSet, 0, nullptr);

    // パイプラインのバインド
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      pipeline);

    // 描画
    vkCmdDrawIndexed(cmdBuffer, model.GetDrawableVertexIndexCount(index), 1, 0, 0, 0);
}

void CubismRenderer_Vulkan::DrawMeshVulkan(const CubismModel& model, const csmInt32 index,
                                           VkCommandBuffer commandBuffer, VkCommandBuffer updateCommandBuffer)
{
    if (s_device == VK_NULL_HANDLE)
    {
        // デバイス未設定
        return;
    }
    if (model.GetDrawableVertexIndexCount(index) == 0)
    {
        // 描画物無し
        return;
    }
    if (model.GetDrawableOpacity(index) <= 0.0f && GetClippingContextBufferForMask() == NULL)
    {
        // 描画不要なら描画処理をスキップする
        return;
    }

    csmInt32 textureIndex = model.GetDrawableTextureIndices(index);
    if (_textures[textureIndex].GetSampler() == VK_NULL_HANDLE || _textures[textureIndex].GetView() == VK_NULL_HANDLE)
    {
        return;
    }

    // 裏面描画の有効・無効
    if (IsCulling())
    {
        vkCmdSetCullModeEXT(commandBuffer, VK_CULL_MODE_BACK_BIT);
    }
    else
    {
        vkCmdSetCullModeEXT(commandBuffer, VK_CULL_MODE_NONE);
    }

    // 頂点バッファにコピー
    CopyToBuffer(index, model.GetDrawableVertexCount(index),
                 const_cast<csmFloat32*>(model.GetDrawableVertices(index)),
                 reinterpret_cast<csmFloat32*>(const_cast<Core::csmVector2*>(model.GetDrawableVertexUvs(index))),
                 _updateCommandBuffers[_commandBufferCurrent]);

    if (GetClippingContextBufferForMask() != NULL) // マスク生成時
    {
        ExecuteDrawForMask(model, index, commandBuffer);
    }
    else
    {
        ExecuteDrawForDraw(model, index, commandBuffer);
    }

    SetClippingContextBufferForDraw(NULL);
    SetClippingContextBufferForMask(NULL);
}

void CubismRenderer_Vulkan::BeginRendering(VkCommandBuffer drawCommandBuffer, bool isResume)
{
    VkClearValue clearValue[2];
    clearValue[0].color = _clearColor.color;
    clearValue[1].depthStencil = {1.0, 0};

    VkImageMemoryBarrier memoryBarrier{};
    memoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    memoryBarrier.srcAccessMask = VK_ACCESS_NONE;
    memoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    memoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    memoryBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    memoryBarrier.image = s_renderImage;
    memoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    vkCmdPipelineBarrier(drawCommandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &memoryBarrier);

    VkRenderingAttachmentInfoKHR colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
    colorAttachment.imageView = s_imageView;
    colorAttachment.loadOp = ((s_useRenderTarget && !isResume)? VK_ATTACHMENT_LOAD_OP_CLEAR
                                                              : VK_ATTACHMENT_LOAD_OP_LOAD);
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue = *clearValue;

    VkRenderingAttachmentInfoKHR depthStencilAttachment{};
    depthStencilAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
    depthStencilAttachment.imageView = _depthImage.GetView();
    depthStencilAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthStencilAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthStencilAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthStencilAttachment.clearValue = {1.0f, 0};

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea = {{0, 0}, {s_renderExtent}};
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;
    renderingInfo.pDepthAttachment = &depthStencilAttachment;

    vkCmdBeginRendering(drawCommandBuffer, &renderingInfo);
}

void CubismRenderer_Vulkan::EndRendering(VkCommandBuffer drawCommandBuffer)
{
    vkCmdEndRendering(drawCommandBuffer);

    // レイアウト変更
    VkImageMemoryBarrier memoryBarrier{};
    memoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    memoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    memoryBarrier.dstAccessMask = VK_ACCESS_NONE;
    memoryBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    memoryBarrier.newLayout = (s_useRenderTarget ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL);
    memoryBarrier.image = s_renderImage;
    memoryBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    vkCmdPipelineBarrier(drawCommandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &memoryBarrier);
}

void CubismRenderer_Vulkan::DoDrawModel()
{
    VkCommandBuffer updateCommandBuffer = _updateCommandBuffers[_commandBufferCurrent];
    VkCommandBuffer drawCommandBuffer = _drawCommandBuffers[_commandBufferCurrent];

    //------------ クリッピングマスク・バッファ前処理方式の場合 ------------
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(updateCommandBuffer, &beginInfo);
    vkBeginCommandBuffer(drawCommandBuffer, &beginInfo);

    if (_clippingManager != NULL)
    {
        // サイズが違う場合はここで作成しなおし
        for (csmInt32 i = 0; i < _clippingManager->GetRenderTextureCount(); ++i)
        {
            if (_offscreenFrameBuffers[_commandBufferCurrent][i].GetBufferWidth() != static_cast<csmUint32>(
                _clippingManager->
                GetClippingMaskBufferSize().X) ||
                _offscreenFrameBuffers[_commandBufferCurrent][i].GetBufferHeight() != static_cast<csmUint32>(
                    _clippingManager->
                    GetClippingMaskBufferSize().Y))
            {
                _offscreenFrameBuffers[_commandBufferCurrent][i].DestroyOffscreenSurface(s_device);
                _offscreenFrameBuffers[_commandBufferCurrent][i].CreateOffscreenSurface(
                    s_device, s_physicalDevice,
                    static_cast<csmUint32>(_clippingManager->GetClippingMaskBufferSize().X),
                    static_cast<csmUint32>(_clippingManager->GetClippingMaskBufferSize().Y),
                    s_imageFormat, s_depthFormat
                );
            }
        }
        if (IsUsingHighPrecisionMask())
        {
            _clippingManager->SetupMatrixForHighPrecision(*GetModel(), false);
        }
        else
        {
            _clippingManager->SetupClippingContext(*GetModel(), drawCommandBuffer, updateCommandBuffer, this, _commandBufferCurrent);
        }
    }
    SubmitCommand(updateCommandBuffer, _updateFinishedSemaphores[_commandBufferCurrent]);
    SubmitCommand(drawCommandBuffer, VK_NULL_HANDLE, _updateFinishedSemaphores[_commandBufferCurrent]);

    // スワップチェーンを再作成した際に深度バッファのサイズを更新する
    if (_depthImage.GetWidth() != s_renderExtent.width || _depthImage.GetHeight() != s_renderExtent.height)
    {
        _depthImage.Destroy(s_device);
        CreateDepthBuffer();
    }

    const csmInt32 drawableCount = GetModel()->GetDrawableCount();
    const csmInt32* renderOrder = GetModel()->GetDrawableRenderOrders();
    // インデックスを描画順でソート
    for (csmInt32 i = 0; i < drawableCount; ++i)
    {
        const csmInt32 order = renderOrder[i];
        _sortedDrawableIndexList[order] = i;
    }

    //描画
    vkBeginCommandBuffer(updateCommandBuffer, &beginInfo);
    vkBeginCommandBuffer(drawCommandBuffer, &beginInfo);
    BeginRendering(drawCommandBuffer, false);

    for (csmInt32 i = 0; i < drawableCount; ++i)
    {
        const csmInt32 drawableIndex = _sortedDrawableIndexList[i];
        // Drawableが表示状態でなければ処理をパスする
        if (!GetModel()->GetDrawableDynamicFlagIsVisible(drawableIndex))
        {
            continue;
        }

        // クリッピングマスクをセットする
        CubismClippingContext_Vulkan* clipContext = (_clippingManager != NULL)
                                                        ? (*_clippingManager->GetClippingContextListForDraw())[
                                                            drawableIndex]
                                                        : NULL;

        if (clipContext != NULL && IsUsingHighPrecisionMask()) // マスクを書く必要がある
        {
            if (clipContext->_isUsing) // 書くことになっていた
            {
                // 一旦オフスクリーン描画に移る
                EndRendering(drawCommandBuffer);

                // 描画順を考慮して今までに積んだコマンドを実行する
                SubmitCommand(updateCommandBuffer, _updateFinishedSemaphores[_commandBufferCurrent]);
                SubmitCommand(drawCommandBuffer, VK_NULL_HANDLE, _updateFinishedSemaphores[_commandBufferCurrent]);
                vkBeginCommandBuffer(updateCommandBuffer, &beginInfo);
                vkBeginCommandBuffer(drawCommandBuffer, &beginInfo);

                CubismOffscreenSurface_Vulkan* currentHighPrecisionMaskColorBuffer = &_offscreenFrameBuffers[_commandBufferCurrent][clipContext->_bufferIndex];
                currentHighPrecisionMaskColorBuffer->BeginDraw(drawCommandBuffer, 1.0f, 1.0f, 1.0f, 1.0f);

                // 生成したFrameBufferと同じサイズでビューポートを設定
                const VkViewport viewport = GetViewport(
                    static_cast<csmFloat32>(_clippingManager->GetClippingMaskBufferSize().X),
                    static_cast<csmFloat32>(_clippingManager->GetClippingMaskBufferSize().Y),
                    0.0, 1.0
                );
                vkCmdSetViewport(drawCommandBuffer, 0, 1, &viewport);

                const VkRect2D rect = GetScissor(
                    0.0, 0.0,
                    static_cast<csmFloat32>(_clippingManager->GetClippingMaskBufferSize().X),
                    static_cast<csmFloat32>(_clippingManager->GetClippingMaskBufferSize().Y)
                );
                vkCmdSetScissor(drawCommandBuffer, 0, 1, &rect);

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

                    SetClippingContextBufferForMask(clipContext);
                    DrawMeshVulkan(*GetModel(), clipDrawIndex, drawCommandBuffer, updateCommandBuffer);
                }
                // --- 後処理 ---
                currentHighPrecisionMaskColorBuffer->EndDraw(drawCommandBuffer); // オフスクリーン描画終了
                SetClippingContextBufferForMask(NULL);
                SubmitCommand(updateCommandBuffer, _updateFinishedSemaphores[_commandBufferCurrent]);
                SubmitCommand(drawCommandBuffer, VK_NULL_HANDLE, _updateFinishedSemaphores[_commandBufferCurrent]);
                vkBeginCommandBuffer(updateCommandBuffer, &beginInfo);
                vkBeginCommandBuffer(drawCommandBuffer, &beginInfo);

                // 描画再開
                BeginRendering(drawCommandBuffer, true);
            }
        }

        // ビューポートを設定する
        const VkViewport viewport = GetViewport(
            static_cast<csmFloat32>(s_renderExtent.width),
            static_cast<csmFloat32>(s_renderExtent.height),
            0.0, 1.0
        );
        vkCmdSetViewport(drawCommandBuffer, 0, 1, &viewport);

        const VkRect2D rect = GetScissor(
            0.0, 0.0,
            static_cast<csmFloat32>(s_renderExtent.width),
            static_cast<csmFloat32>(s_renderExtent.height)
        );
        vkCmdSetScissor(drawCommandBuffer, 0, 1, &rect);

        // クリッピングマスクをセットする
        SetClippingContextBufferForDraw(clipContext);
        IsCulling(GetModel()->GetDrawableCulling(drawableIndex) != 0);
        DrawMeshVulkan(*GetModel(), drawableIndex, drawCommandBuffer, updateCommandBuffer);
    }

    EndRendering(drawCommandBuffer);
    SubmitCommand(updateCommandBuffer, _updateFinishedSemaphores[_commandBufferCurrent]);
    SubmitCommand(drawCommandBuffer, VK_NULL_HANDLE, _updateFinishedSemaphores[_commandBufferCurrent]);

    PostDraw();
}

void CubismRenderer_Vulkan::PostDraw()
{
    _commandBufferCurrent++;
    if (s_bufferSetNum <= _commandBufferCurrent)
    {
        _commandBufferCurrent = 0;
    }
}

void CubismRenderer_Vulkan::SetClippingContextBufferForMask(CubismClippingContext_Vulkan* clip)
{
    _clippingContextBufferForMask = clip;
}

CubismClippingContext_Vulkan* CubismRenderer_Vulkan::GetClippingContextBufferForMask() const
{
    return _clippingContextBufferForMask;
}

void CubismRenderer_Vulkan::SetClippingContextBufferForDraw(CubismClippingContext_Vulkan* clip)
{
    _clippingContextBufferForDraw = clip;
}

CubismClippingContext_Vulkan* CubismRenderer_Vulkan::GetClippingContextBufferForDraw() const
{
    return _clippingContextBufferForDraw;
}

void CubismRenderer_Vulkan::BindTexture(CubismImageVulkan& image)
{
    _textures.PushBack(image);
}

void CubismRenderer_Vulkan::SetClippingMaskBufferSize(csmFloat32 width, csmFloat32 height)
{
    // インスタンス破棄前にレンダーテクスチャの数を保存
    const csmInt32 renderTextureCount = _clippingManager->GetRenderTextureCount();

    //FrameBufferのサイズを変更するためにインスタンスを破棄・再作成する
    CSM_DELETE_SELF(CubismClippingManager_Vulkan, _clippingManager);

    _clippingManager = CSM_NEW CubismClippingManager_Vulkan();

    _clippingManager->SetClippingMaskBufferSize(width, height);

    _clippingManager->Initialize(
        *GetModel(),
        renderTextureCount
    );
}

CubismVector2 CubismRenderer_Vulkan::GetClippingMaskBufferSize() const
{
    return _clippingManager->GetClippingMaskBufferSize();
}

CubismOffscreenSurface_Vulkan* CubismRenderer_Vulkan::GetMaskBuffer(csmUint32 backbufferNum, csmInt32 offscreenIndex)
{
    return &_offscreenFrameBuffers[backbufferNum][offscreenIndex];
}

void CubismRenderer_Vulkan::SetColorUniformBuffer(ModelUBO& ubo, const CubismTextureColor& baseColor,
                                                  const CubismTextureColor& multiplyColor, const CubismTextureColor& screenColor)
{
    UpdateColor(ubo.baseColor, baseColor.R, baseColor.G, baseColor.B, baseColor.A);
    UpdateColor(ubo.multiplyColor, multiplyColor.R, multiplyColor.G, multiplyColor.B, multiplyColor.A);
    UpdateColor(ubo.screenColor, screenColor.R, screenColor.G, screenColor.B, screenColor.A);
}

void CubismRenderer_Vulkan::BindVertexAndIndexBuffers(const csmInt32 index, VkCommandBuffer& cmdBuffer)
{
    VkBuffer vertexBuffers[] = {_vertexBuffers[_commandBufferCurrent][index].GetBuffer()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(cmdBuffer, _indexBuffers[_commandBufferCurrent][index].GetBuffer(), 0, VK_INDEX_TYPE_UINT16);
}

void CubismRenderer_Vulkan::SetColorChannel(ModelUBO& ubo, CubismClippingContext_Vulkan* contextBuffer)
{
    const csmInt32 channelIndex = contextBuffer->_layoutChannelIndex;
    CubismTextureColor *colorChannel = contextBuffer->GetClippingManager()->GetChannelFlagAsColor(channelIndex);
    UpdateColor(ubo.channelFlag, colorChannel->R, colorChannel->G, colorChannel->B, colorChannel->A);
}

}}}}

//------------ LIVE2D NAMESPACE ------------
