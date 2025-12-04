/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "CubismShader_D3D11.hpp"

#include "CubismRenderer_D3D11.hpp"
#include <d3dcompiler.h>

//------------ LIVE2D NAMESPACE ------------
namespace Live2D { namespace Cubism { namespace Framework { namespace Rendering {

void CubismShader_D3D11::ReleaseShaderProgram()
{
    // シェーダーソースコード開放
    _shaderSrc.Clear();

    // 頂点レイアウト開放
    if (_vertexFormat)
    {
        _vertexFormat->Release();
        _vertexFormat = NULL;
    }

    // 器はそのまま
    for (csmInt32 i = 0; i < ShaderNames_Max; i++)
    {
        if(_shaderSetsVS[i])
        {
            _shaderSetsVS[i]->Release();
            _shaderSetsVS[i] = NULL;
        }

        if (_shaderSetsPS[i])
        {
            _shaderSetsPS[i]->Release();
            _shaderSetsPS[i] = NULL;
        }
    }
}

CubismShader_D3D11::CubismShader_D3D11()
    : _vertexFormat(NULL)
{
    // 器作成
    for (csmInt32 i = 0; i < ShaderNames_Max; i++)
    {
        _shaderSetsVS.PushBack(NULL);
        _shaderSetsPS.PushBack(NULL);
    }
}

CubismShader_D3D11::~CubismShader_D3D11()
{
    ReleaseShaderProgram();

    // 器の削除
    _shaderSetsVS.Clear();
    _shaderSetsPS.Clear();
}

void CubismShader_D3D11::GenerateShaders(ID3D11Device* device)
{
    // 既に有るかどうかをこれで判断している
    if(_vertexFormat!=NULL)
    {
        return;
    }

    // 一旦開放
    ReleaseShaderProgram();

    csmBool isSuccess = false;
    do
    {
        // シェーダーソースコードが記述されているファイルを読み込み
        const csmChar* frameworkShaderPath = "FrameworkShaders/CubismEffect.fx";
        csmLoadFileFunction fileLoader = Csm::CubismFramework::GetLoadFileFunction();
        csmReleaseBytesFunction bytesReleaser = Csm::CubismFramework::GetReleaseBytesFunction();

        if (!fileLoader)
        {
            CubismLogError("File loader is not set.");
            break;
        }

        if (!bytesReleaser)
        {
            CubismLogError("Byte releaser is not set.");
            break;
        }

        csmSizeInt shaderSize;
        csmByte* shaderSrc = fileLoader(frameworkShaderPath, &shaderSize);
        if (shaderSrc == NULL)
        {
            CubismLogError("Failed to load shader");
            break;
        }
        csmString shaderString = csmString(reinterpret_cast<const csmChar*>(shaderSrc), shaderSize);

        _shaderSrc.Clear();
        for (csmInt32 i = 0; i < shaderString.GetLength(); i++)
        {
            _shaderSrc.PushBack(shaderString.GetRawString()[i]);
        }

        // ファイル読み込みで確保したバイト列を開放
        bytesReleaser(shaderSrc);

        // マスク
        if(!LoadShaderProgram(device, false, ShaderNames_SetupMask, static_cast<const csmChar*>("VertSetupMask")))
        {
            break;
        }
        if (!LoadShaderProgram(device, true, ShaderNames_SetupMask, static_cast<const csmChar*>("PixelSetupMask")))
        {
            break;
        }

        // 頂点シェーダ
        if (!LoadShaderProgram(device, false, ShaderNames_Normal, static_cast<const csmChar*>("VertNormal")))
        {
            break;
        }
        if (!LoadShaderProgram(device, false, ShaderNames_NormalMasked, static_cast<const csmChar*>("VertMasked")))
        {
            break;
        }

        // ピクセルシェーダ
        if (!LoadShaderProgram(device, true, ShaderNames_Normal, static_cast<const csmChar*>("PixelNormal")))
        {
            break;
        }
        if (!LoadShaderProgram(device, true, ShaderNames_NormalMasked, static_cast<const csmChar*>("PixelMasked")))
        {
            break;
        }
        if (!LoadShaderProgram(device, true, ShaderNames_NormalMaskedInverted, static_cast<csmChar*>("PixelMaskedInverted")))
        {
            break;
        }
        if (!LoadShaderProgram(device, true, ShaderNames_NormalPremultipliedAlpha, static_cast<csmChar*>("PixelNormalPremult")))
        {
            break;
        }
        if (!LoadShaderProgram(device, true, ShaderNames_NormalMaskedPremultipliedAlpha, static_cast<const csmChar*>("PixelMaskedPremult")))
        {
            break;
        }
        if (!LoadShaderProgram(device, true, ShaderNames_NormalMaskedInvertedPremultipliedAlpha, static_cast<csmChar*>("PixelMaskedInvertedPremult")))
        {
            break;
        }

        // 成功
        isSuccess = true;
    } while (0);

    if(!isSuccess)
    {
        CubismLogError("Fail Compile shader");
        CSM_ASSERT(0);
        return;
    }


// シェーダーコードからレイアウト取得

    UINT compileFlag = 0;
#ifdef CSM_DEBUG
    // デバッグならば
    compileFlag |= D3DCOMPILE_DEBUG;
#endif

    ID3DBlob* layoutError = NULL;
    ID3DBlob* layoutBlobr = NULL;
    HRESULT hr = D3DCompile(
        _shaderSrc.GetPtr(),                // メモリー内のシェーダーへのポインターです。
        _shaderSrc.GetSize(),               // メモリー内のシェーダーのサイズです。
        NULL,                               // シェーダー コードが格納されているファイルの名前です。
        NULL,                               // マクロ定義の配列へのポインターです
        NULL,                               // インクルード ファイルを扱うインターフェイスへのポインターです
        "VertNormal",                       // シェーダーの実行が開始されるシェーダー エントリポイント関数の名前です。
        "vs_4_0",                           // シェーダー モデルを指定する文字列。
        compileFlag,                        // シェーダーコンパイルフラグ
        0,                                  // シェーダーエフェクトのフラグ
        &layoutBlobr,                       // 結果
        &layoutError);                     // エラーが出る場合はここで
    if (FAILED(hr))
    {
        CubismLogError("Fail create input layout");
        CSM_ASSERT(0);
    }
    else
    {
        // この描画で使用する頂点フォーマット
        D3D11_INPUT_ELEMENT_DESC elems[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };
        hr = device->CreateInputLayout(elems, ARRAYSIZE(elems), layoutBlobr->GetBufferPointer(), layoutBlobr->GetBufferSize(), &_vertexFormat);

        if (FAILED(hr))
        {
            CubismLogWarning("CreateVertexDeclaration failed");
        }
    }

    // blobの開放
    if (layoutError)
    {
        layoutError->Release();
    }
    if (layoutBlobr)
    {
        layoutBlobr->Release();
    }
}

Csm::csmBool CubismShader_D3D11::LoadShaderProgram(ID3D11Device* device, bool isPs, csmInt32 assign, const csmChar* entryPoint)
{
    csmBool bRet = false;
    if (!device) return false;

    ID3DBlob* errorBlob = NULL;
    ID3DBlob* vertexBlob = NULL;

    ID3D11VertexShader*     vertexShader = NULL;
    ID3D11PixelShader*      pixelShader = NULL;

    HRESULT hr = S_OK;
    do
    {
        UINT compileFlag = 0;
#ifdef CSM_DEBUG
        // デバッグならば
        compileFlag |= D3DCOMPILE_DEBUG;
#endif

        hr = D3DCompile(
            _shaderSrc.GetPtr(),                // メモリー内のシェーダーへのポインターです。
            _shaderSrc.GetSize(),               // メモリー内のシェーダーのサイズです。
            NULL,                               // シェーダー コードが格納されているファイルの名前です。
            NULL,                               // マクロ定義の配列へのポインターです
            NULL,                               // インクルード ファイルを扱うインターフェイスへのポインターです
            entryPoint,                         // シェーダーの実行が開始されるシェーダー エントリポイント関数の名前です。
            isPs ? "ps_4_0" :"vs_4_0",          // シェーダー モデルを指定する文字列。
            compileFlag,                          // シェーダーコンパイルフラグ
            0,                                  // シェーダーエフェクトのフラグ
            &vertexBlob,
            &errorBlob);                              // エラーが出る場合はここで
        if (FAILED(hr))
        {
            CubismLogWarning("Fail Compile Shader : %s", entryPoint==NULL ? "" : entryPoint);
            break;
        }
        hr = isPs ? device->CreatePixelShader(vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(), NULL, &pixelShader) :
            device->CreateVertexShader(vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(), NULL, &vertexShader);
        if (FAILED(hr))
        {
            CubismLogWarning("Fail Create Shader");
            break;
        }

        if(isPs)
        {
            _shaderSetsPS[assign] = pixelShader;
        }
        else
        {
            _shaderSetsVS[assign] = vertexShader;
        }
        // 成功
        bRet = true;
    } while (0);

    if(errorBlob)
    {
        errorBlob->Release();
    }
    if (vertexBlob)
    {
        vertexBlob->Release();
    }

    return bRet;
}

ID3D11VertexShader* CubismShader_D3D11::GetVertexShader(csmUint32 assign)
{
    if(assign<ShaderNames_Max)
    {
        return _shaderSetsVS[assign];
    }

    return NULL;
}

ID3D11PixelShader* CubismShader_D3D11::GetPixelShader(csmUint32 assign)
{
    if (assign<ShaderNames_Max)
    {
        return _shaderSetsPS[assign];
    }

    return NULL;
}

void CubismShader_D3D11::SetupShader(ID3D11Device* device, ID3D11DeviceContext* renderContext)
{
    // まだシェーダ・頂点宣言未作成ならば作成する
    GenerateShaders(device);
    if (!renderContext || !_vertexFormat)
    {
        return;
    }

    renderContext->IASetInputLayout(_vertexFormat);
}

}}}}

//------------ LIVE2D NAMESPACE ------------

