/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#pragma once

#include <CubismFramework.hpp>
#include <string>

/**
* @brief プラットフォーム依存機能を抽象化する Cubism Platform Abstraction Layer.
*
* ファイル読み込みや時刻取得等のプラットフォームに依存する関数をまとめる
*
*/
class LAppPal
{
    static std::string SHADER_DIR;

    static void interceptShaderLoading(std::string& filePath);
public:
    /**
    * @brief ファイルをバイトデータとして読み込む
    *
    * ファイルをバイトデータとして読み込む
    *
    * @param[in]   filePath    読み込み対象ファイルのパス
    * @param[out]  outSize     ファイルサイズ
    * @return                  バイトデータ
    */
    static Csm::csmByte* LoadFileAsBytes(const std::string filePath, Csm::csmSizeInt* outSize);


    /**
    * @brief バイトデータを解放する
    *
    * バイトデータを解放する
    *
    * @param[in]   byteData    解放したいバイトデータ
    */
    static void ReleaseBytes(Csm::csmByte* byteData);

    static void PrintLn(const Csm::csmChar* message);

    static double GetCurrentTimePoint();

    static void InitShaderDir(const std::string& path);
};
