/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "CubismShader_D3D9.hpp"

#include "CubismRenderer_D3D9.hpp"

//------------ LIVE2D NAMESPACE ------------
namespace Live2D { namespace Cubism { namespace Framework { namespace Rendering {

void CubismShader_D3D9::ReleaseShaderProgram()
{
    _shaderSrc.Clear();

    if(_vertexFormat)
    {
        _vertexFormat->Release();
        _vertexFormat = NULL;
    }
    if(_shaderEffect)
    {
        _shaderEffect->Release();
        _shaderEffect = NULL;
    }
}

CubismShader_D3D9::CubismShader_D3D9()
    : _shaderEffect(NULL)
    , _vertexFormat(NULL)
{
}

CubismShader_D3D9::~CubismShader_D3D9()
{
    ReleaseShaderProgram();
}

void CubismShader_D3D9::GenerateShaders(LPDIRECT3DDEVICE9 device)
{
    if(_shaderEffect || _vertexFormat)
    {// 既に有る
        return;
    }

    if(!device)
    {
        return;
    }

    // シェーダーソースコードが記述されているファイルを読み込み
    const csmChar* frameworkShaderPath = "FrameworkShaders/CubismEffect.fx";
    csmLoadFileFunction fileLoader = Csm::CubismFramework::GetLoadFileFunction();
    csmReleaseBytesFunction bytesReleaser = Csm::CubismFramework::GetReleaseBytesFunction();

    if (!fileLoader)
    {
        CubismLogError("File loader is not set.");
        return;
    }

    if (!bytesReleaser)
    {
        CubismLogError("Byte releaser is not set.");
        return;
    }

    csmSizeInt shaderSize;
    csmByte* shaderSrc = fileLoader(frameworkShaderPath, &shaderSize);
    if (shaderSrc == NULL)
    {
        CubismLogError("Failed to load shader");
        return;
    }
    csmString shaderString = csmString(reinterpret_cast<const csmChar*>(shaderSrc), shaderSize);

    _shaderSrc.Clear();
    for (csmInt32 i = 0; i < shaderString.GetLength(); i++)
    {
        _shaderSrc.PushBack(shaderString.GetRawString()[i]);
    }

    // ファイル読み込みで確保したバイト列を開放
    bytesReleaser(shaderSrc);

    //
    if(!LoadShaderProgram(device))
    {
        CubismLogError("CreateShader failed");
        CSM_ASSERT(0);
    }


    // この描画で使用する頂点フォーマット
    D3DVERTEXELEMENT9 elems[] = {
        { 0, 0, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 0, sizeof(float) * 2, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
        D3DDECL_END()
    };
    if (device->CreateVertexDeclaration(elems, &_vertexFormat))
    {
        CubismLogError("CreateVertexDeclaration failed");
        CSM_ASSERT(0);
    }
}

Csm::csmBool CubismShader_D3D9::LoadShaderProgram(LPDIRECT3DDEVICE9 device)
{
    if (!device) return false;

    ID3DXBuffer* error = NULL;
    if (FAILED(D3DXCreateEffect(device, _shaderSrc.GetPtr(), _shaderSrc.GetSize(), 0, 0, 0, 0, &_shaderEffect, &error)))
    {
        return false;
    }

    return true;
}

ID3DXEffect* CubismShader_D3D9::GetShaderEffect() const
{
    return _shaderEffect;
}

void CubismShader_D3D9::SetupShader(LPDIRECT3DDEVICE9 device)
{
    // まだシェーダ・頂点宣言未作成ならば作成する
    GenerateShaders(device);

    if (!device || !_vertexFormat) return;

    device->SetVertexDeclaration(_vertexFormat);
}

}}}}

//------------ LIVE2D NAMESPACE ------------

