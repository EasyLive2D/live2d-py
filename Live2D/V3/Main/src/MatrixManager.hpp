#pragma once

#include <CubismFramework.hpp>

#include "Math/CubismMatrix44.hpp"

class MatrixManager
{
public:
    MatrixManager();
    void SetModelWH(float mw, float mh);
    void UpdateScreenToScene(int ww, int wh);
    void ScreenToScene(float *x, float *y);
    Csm::CubismMatrix44 &GetMvp();
    void SetOffset(float x, float y);
    void SetScaleX(float sx);
    void SetScaleY(float sy);
    void Rotate(float deg);
    void InvertTransform(float* x, float* y);
private:
    Csm::CubismMatrix44 _screenToScene;
    Csm::CubismMatrix44 _p;
    Csm::CubismMatrix44 _m;

    float _rotation[16];

    float _offsetX;
    float _offsetY;
    float _scaleX;
    float _scaleY;
    float _baseScale;
    int _ww;
    int _wh;
    float _mw;
    float _mh;
};