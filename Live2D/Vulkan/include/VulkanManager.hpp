/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#pragma once
#include <vulkan/vulkan.h>
#include <cstdint>
#include <vector>
#include "SwapchainManager.hpp"


enum Platform : uint32_t {
    PLATFORM_NONE = 0,
    PLATFORM_WINDOWS = 1,
    PLATFORM_XLIB,
    PLATFORM_XCB,
    PLATFORM_WAYLAND,
};

/**
 * @brief   平台无关的 surface 创建上下文
 *          window  : 原生窗口句柄 (HWND / X11 Window / wl_surface* / NSView*)
 *          display : 显示连接 (X11: Display* / Wayland: wl_display* / 其他: nullptr)
 */
struct SurfaceCreateContext {
    void* window = nullptr;
    void* display = nullptr;
    void* connection = nullptr;
    uint32_t platform = PLATFORM_NONE;
};

/**
  * @brief   vulkanに必要なリソースを扱うクラス
  */
class VulkanManager {
public:
    static const int MAX_FRAME_IN_FLIGHT = 3;
    // キューファミリの情報を格納する構造体
    struct QueueFamilyIndices {
        // 描画用と表示用のキューファミリは必ずしも一致するとは限らないので分ける
        int32_t graphicsFamily = -1;   // 描画コマンドに使用するキューファミリ
        int32_t presentFamily = -1;    // 表示に使用するキューファミリ

        // 対応するキューファミリがあるか
        bool isComplete() { return graphicsFamily != -1 && presentFamily != -1; }
    } indices;

    const char* deviceExtensions[2] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                                       VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME};

    const char* validationLayers[1] = {"VK_LAYER_KHRONOS_validation"};

    static VulkanManager* GetInstance();

    /**
     * @brief   コンストラクタ
     * @param[in]   wind             ->  ウィンドウ
     */
    VulkanManager(void* wind);

    /**
     * @brief   デストラクタ
     */
    ~VulkanManager();

    /**
     * @brief   インスタンス拡張を外部から注入する（Initialize 前に呼ぶ）
     *          glfwGetRequiredInstanceExtensions 等の結果を渡す
     */
    void SetInstanceExtensions(const char** extensions, uint32_t count);

    /**
     * @brief   surface 创建钩子
     *          可从外部注入（省略时使用平台标准的 vkCreate*Surface）
     */
    typedef VkSurfaceKHR (*CreateSurfaceFunc)(VkInstance instance, const SurfaceCreateContext* ctx);
    void SetCreateSurfaceFunc(CreateSurfaceFunc func) { _createSurfaceFunc = func; }

    /**
     * @brief   批量设置 surface 创建上下文
     */
    void SetSurfaceCreateContext(const SurfaceCreateContext& ctx) {
        _surfaceCtx = ctx;
    }
    void SetDisplay(void* display) { _surfaceCtx.display = display; }

    /**
     * @brief   初期化済みか（論理デバイスが作成されているか）
     */
    bool IsInitialized() const { return _device != VK_NULL_HANDLE; }

    /**
     * @brief   検証レイヤーのサポートを確認する
     */
    bool CheckValidationLayerSupport();

    /**
     * @brief   必要な拡張機能を取得する
     *
     * @return  必要な拡張の配列
     */
    std::vector<const char*> GetRequiredExtensions();

    /**
     * @brief   デバッグメッセージを有効にする
     *
     * @param[in]   createInfo             ->  デバッグメッセンジャーオブジェクトの情報
     */
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    /**
     * @brief   インスタンスを生成する
     */
    void CreateInstance();

    /**
     * @brief   デバッグメッセージを有効にする
     */
    void SetupDebugMessenger();

    /**
     * @brief   デバイスの拡張をチェックする
     *
     * @param[in]   physicalDevice ->  物理デバイス
     */
    bool CheckDeviceExtensionSupport(VkPhysicalDevice physicalDevice);

    /**
     * @brief    サーフェスを作る
     */
    void CreateSurface();

    /**
     * @brief   キューファミリを見つける
     *
     * @param[in]   queueFlags ->  物理デバイス
     */
    void FindQueueFamilies(VkPhysicalDevice device);

    /**
     * @brief   デバイスが使えるか確認する
     *
     * @param[in]   device ->  物理デバイス
     */
    bool IsDeviceSuitable(VkPhysicalDevice device);

    /**
     * @brief   物理デバイスを取得する
     */
    void PickPhysicalDevice();

    /**
     * @brief   論理デバイスを作成する
     */
    void CreateLogicalDevice();

    /**
     * @brief   深度フォーマットを作成する
     */
    void ChooseSupportedDepthFormat();

    /**
     * @brief   コマンドプールを作成する
     */
    void CreateCommandPool();

    /**
     * @brief   同期オブジェクトを作成する
     */
    void CreateSyncObjects();

    /// 按当前 swapchain 图像数重建 per-image 呈现完成信号量（交换链重建后调用）
    void RecreateFinishSemaphores();

    /**
     * @brief   初期化する
     */
    void Initialize();

    /**
     * @brief   コマンドの記録を開始する
     */
    VkCommandBuffer BeginSingleTimeCommands();

    /**
     * @brief   提交单帧命令缓冲区
     *
     * @param[in]   commandBuffer     ->  命令缓冲区
     */
    void SubmitCommand(VkCommandBuffer commandBuffer);

    /**
     * @brief   描画する
     */
    void UpdateDrawFrame();

    /**
     * @brief   描画完了の追加処理
     */
    void PostDraw();

    /**
     * @brief   スワップチェーンを再構成する
     */
    void RecreateSwapchain();

    // ---- 帧解耦接口（由 Python 侧每帧驱动） ----

    /**
     * @brief   开始一帧
     *          获取 swapchain 图像 → 开始录制帧命令缓冲 →
     *          开始主 rendering 区间（loadOp=CLEAR）
     * @return  false: swapchain 失效并已重建，本帧应跳过
     */
    bool BeginFrame();

    /**
     * @brief   结束一帧
     *          结束主 rendering 区间 → present 布局迁移 → 提交 → 呈现
     */
    void EndFrame();

    /**
     * @brief   挂起主 rendering 区间（用于向 mask 等其他目标录制，
     *          在帧命令缓冲上执行 vkCmdEndRendering）
     */
    void SuspendMainRegion();

    /**
     * @brief   恢复主 rendering 区间（loadOp=LOAD，保留已有内容）
     */
    void ResumeMainRegion();

    /**
     * @brief   帧是否正在录制中
     */
    bool IsFrameActive() const { return _frameActive; }

    /**
     * @brief   获取当前帧命令缓冲（纯录制模式用）
     */
    VkCommandBuffer GetFrameCommandBuffer() const { return _frameCmds[_frameIndex]; }

    /**
     * @brief   获取当前帧传输命令缓冲（纯录制模式用）
     *          渲染区间之外才能录制的命令（vkCmdCopyBuffer 等）只能进这个缓冲；
     *          由 EndFrame 在帧命令缓冲之前提交
     */
    VkCommandBuffer GetUploadCommandBuffer() const { return _uploadCmds[_frameIndex]; }

    /**
     * @brief   本帧获取到的 swapchain 图像索引
     */
    uint32_t GetCurrentImageIndex() const { return _imageIndex; }

    /**
     * @brief   可同时飞行的帧数（CPU 提交流水深度，槽位数）
     */
    uint32_t GetFramesInFlight() const;

    /**
     * @brief   リソースを破棄する
     */
    void Destroy();

    /**
     * @brief   デバイスを取得する
     * @return  デバイス
     */
    VkDevice GetDevice() { return _device; }

    /**
     * @brief   物理デバイスを取得する
     * @return  物理デバイス
     */
    VkPhysicalDevice GetPhysicalDevice() { return _physicalDevice; }

    /**
     * @brief   描画キューを取得する
     * @return  描画キュー
     */
    VkQueue GetGraphicQueue() { return _graphicQueue; }

    /**
     * @brief   コマンドプールを取得する
     * @return  コマンドプール
     */
    VkCommandPool GetCommandPool() { return _commandPool; }

    /**
     * @brief   等待 GPU 空闲并重置命令池，清除命令缓冲对各类资源的引用
     *          销毁渲染资源（缓冲/图像）之前调用，否则校验层会报
     *          "can't be called on VkBuffer/VkImage ... in use by VkCommandBuffer"
     */
    void ResetCommandBuffers();

    /**
     * @brief   スワップチェーンマネージャーを取得する
     * @return  スワップチェーンマネージャー
     */
    SwapchainManager* GetSwapchainManager() { return _swapchainManager; }

    /**
     * @brief   ウィンドウサイズが変更されたかのフラグを取得する
     * @return  ウィンドウサイズの変更フラグ
     */
    bool GetIsWindowSizeChanged() { return _isSwapchainInvalid; }

    /**
     * @brief   ウィンドウサイズが変更されたかのフラグをセットする
     * @param[in]   flag     ->  セットするフラグ
     */
    void SetIsWindowSizeChanged(bool flag) { _isSwapchainInvalid = flag; }

    /**
     * @brief    フレームバッファのフラグを更新する
     *
     */
    void SetFrameBufferResized(bool flag) { _framebufferResized = flag; }

    /**
     * @brief   深度フォーマットを取得する
     * @return  深度フォーマット
     */
    VkFormat GetDepthFormat() const { return _depthFormat; }

    /**
     * @brief    サーフェスのフォーマットを取得する
     *
     * @return  フォーマット
     */
    VkFormat GetImageFormat() { return _surfaceFormat; }

    /**
     * @brief    スワップチェーンイメージを取得する
     *
     * @return  スワップチェーンイメージ
     */
    VkImage GetSwapchainImage() { return GetSwapchainManager()->GetImages()[_imageIndex]; }

    /**
     * @brief    スワップチェーンイメージビューを取得する
     *
     * @return  スワップチェーンイメージビュー
     */
    VkImageView GetSwapchainImageView() {
        return GetSwapchainManager()->GetImageViews()[_imageIndex];
    }

    void ChooseSurfaceFormat();

    int GetCurrentFrame() { return _frameIndex; }

    void InvalidateSizeIfNeeded(int width, int height);

private:
    SurfaceCreateContext _surfaceCtx;                 // surface 创建上下文
    std::vector<const char*> _instanceExtensions;     // 外部注入的实例扩展
    CreateSurfaceFunc _createSurfaceFunc = nullptr;   // surface 创建钩子（省略时用内部实现）
    VkInstance _instance;                             // ライブラリを初期化するインスタンス
    VkSurfaceKHR _surface;              // ウィンドウシステムに描画ために必要なサーフェス
    VkPhysicalDevice _physicalDevice;   // 物理デバイス
    VkDevice _device;                   // 論理デバイス
    VkQueue _graphicQueue;              // 描画コマンドを積むキュー
    VkQueue _presentQueue;              // 表示コマンドに使用するキュー
    VkCommandPool _commandPool;         // コマンドバッファの作成に必要なコマンドプール
    std::vector<VkSemaphore> _imageAvailableSemaphores;
    std::vector<VkSemaphore> _imageFinishDrawSemaphores;
    int _frameIndex = 0;
    SwapchainManager*
        _swapchainManager;              // スワップチェーンの管理を行うスワップチェーンマネージャー
    bool _isSwapchainInvalid = false;   // ウィンドウサイズが変更されたかのフラグ
    VkDebugUtilsMessengerEXT _debugMessenger;   // デバッグメッセージを出力するオブジェクト
    uint32_t _imageIndex = 0;                   // 現在のイメージインデックス
    VkFormat _depthFormat;                      // 深度フォーマット
    VkFormat _surfaceFormat = VK_FORMAT_R8G8B8A8_UNORM;   // イメージフォーマット
    bool _framebufferResized = false;   // フレームバッファのサイズが変わったか
    std::vector<VkFence> _frameFences;
    // per-slot 常驻命令缓冲：槽位由 fence 串行化，等待该槽 fence 完成后才允许
    // reset/re-record 该槽缓冲，避免重置仍在 GPU 上执行的命令缓冲（UB）
    std::vector<VkCommandBuffer> _frameCmds;    // 帧解耦模式的常驻命令缓冲（每槽一份）
    std::vector<VkCommandBuffer> _uploadCmds;   // 常驻传输命令缓冲（渲染区间外命令，每槽一份）
    bool _frameActive = false;                  // 是否处于 BeginFrame~EndFrame 之间
#if defined(LIVE2D_VK_VALIDATION)
    bool _enableValidationLayers = true;   // 検証レイヤーを有効にするか
#else
    bool _enableValidationLayers = false;
#endif   // LIVE2D_VK_VALIDATION
    uint32_t _frameWidth;
    uint32_t _frameHeight;
};
