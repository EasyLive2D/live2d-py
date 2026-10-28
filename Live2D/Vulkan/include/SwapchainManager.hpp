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

/**
 * @brief   スワップチェーンを扱うクラス
 */
class SwapchainManager
{
public:
    struct SwapchainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities;           //基本的な機能
        std::vector<VkSurfaceFormatKHR> formats;         //利用可能なフォーマット
        std::vector<VkPresentModeKHR> presentModes;      //利用可能な表示モード
    };

    /**
     * @brief   コンストラクタ
     * @param[in]   physicalDevice      ->  物理デバイス
     * @param[in]   device              ->  デバイス
     * @param[in]   surface             ->  サーフェス
     * @param[in]   graphicsFamily      ->  描画コマンドに使うキューファミリ
     * @param[in]   presentFamily       ->  表示に使うキューファミリ
     * @param[in]   extent              ->  init frame size
     */
    SwapchainManager(VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, VkFormat format, uint32_t graphicsFamily, uint32_t presentFamily,
        VkExtent2D extent);

    /**
     * @brief    物理デバイスのサポートを確認する
     *
     * @param[in]   physicalDevice      ->  物理デバイス
     * @param[in]   surface             ->  スワップチェーンサーフェス
     * @return  物理デバイスのサポート情報
     */
    static SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice& physicalDevice, VkSurfaceKHR surface);

    /**
     * @brief    サーフェスフォーマットを選択する
     * @param[in]   availableFormats    ->  使えるサーフェスフォーマット
     * @return  選択するサーフェスフォーマット
     */
    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

    /**
     * @brief    表示モードを選択する
     * @param[in]   availableFormats    ->  使えるフォーマット
     */
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

    /**
     * @brief   スワップチェーンイメージの解像度を選択する
     *
     * @param[in]   capabilities        ->  サーフェスの機能を保持する構造体
     * @return  スワップチェーンイメージの解像度
     */
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, VkExtent2D requested);

    /**
     * @brief   スワップチェーンを作成する
     *
     * @param[in]   physicalDevice      ->  物理デバイス
     * @param[in]   device              ->  デバイス
     * @param[in]   surface             ->  サーフェス
     * @param[in]   graphicsFamily      ->  描画コマンドに使うキューファミリ
     * @param[in]   presentFamily       ->  表示に使うキューファミリ
     */
    void CreateSwapchain(VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, uint32_t graphicsFamily, uint32_t presentFamily, 
                        VkExtent2D requested);

    /**
     * @brief    表示コマンドをキューに積む
     *
     * @param[in]   queue          ->  キュー
     * @param[in]   imageIndex     ->  イメージのインデックス
     * @param[in]   finishSemaphore ->  表示コマンドが完了したときに通知するsemaphore
     * @return  成功したか
     */
    VkResult QueuePresent(VkQueue& queue, uint32_t imageIndex, VkSemaphore finishSemaphore);

    /**
     * @brief    スワップチェーンのレイアウトの変更
     *
     * @param[in]   device         ->  論理デバイス
     * @param[in]   commandPool    ->  コマンドプール
     * @param[in]   queue          ->  キュー
     */
    void ChangeLayout(VkDevice device, VkCommandPool commandPool, VkQueue queue);

    /**
     * @brief    リソースを破棄＆再作成する
     *
     * @param[in]   device              ->  デバイス
     */
    void Cleanup(VkDevice device);

    /**
     * @brief    extentを取得する
     *
     * @return  extent
     */
    VkExtent2D GetExtent() { return extent; }

    /**
     * @brief    イメージ数を取得する
     *
     * @return  イメージ数
     */
    int32_t GetImageCount() { return imageCount; }

    /**
     * @brief    スワップチェーンを取得する
     *
     * @return  スワップチェーン
     */
    VkSwapchainKHR GetSwapchain() { return swapchain; }

    /**
     * @brief    スワップチェーンのフォーマットを取得する
     *
     * @return  フォーマット
     */
    VkFormat GetSwapchainImageFormat() { return swapchainFormat; }

    /**
     * @brief    スワップチェーンのイメージを取得する
     *
     * @return  スワップチェーンイメージ
     */
    std::vector<VkImage> GetImages() { return images; }

    /**
     * @brief    スワップチェーンのイメージビューを取得する
     *
     * @return  スワップチェーンイメージビュー
     */
    std::vector<VkImageView> GetImageViews() { return imageViews; }

private:
    VkExtent2D extent = {0, 0};                  ///< イメージの解像度
    uint32_t imageCount = 0;                     ///< イメージ数
    std::vector<VkImage> images;                 ///< スワップチェーンのイメージ
    std::vector<VkImageView> imageViews;         ///< スワップチェーンのイメージビュー
    VkSwapchainKHR swapchain;                    ///< スワップチェーン
    const VkFormat swapchainFormat = VK_FORMAT_R8G8B8A8_UNORM; ///< スワップチェーンのフォーマット
};
