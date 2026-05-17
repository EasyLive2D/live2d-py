#include "MatrixManager.hpp"
#include "LAppModel.hpp"

MatrixManager::MatrixManager(): _offsetX(0.0f), _offsetY(0.0f), _scaleX(1.0f), _scaleY(1.0f),
                                _ww(800),
                                _wh(600)                          
{
    // load identity
    for (int i = 0; i < 16; i++)
    {
        _rotation[i] = 0.0f;
    }
    _rotation[0] = _rotation[5] = _rotation[10] = _rotation[15] = 1.0f;
}

void MatrixManager::SetModelWH(float mw, float mh)
{
    _mw = mw;
    _mh = mh;
}

// called when window is resized
void MatrixManager::UpdateScreenToScene(int width, int height)
{
    _ww = width;
    _wh = height;

    float ratio = static_cast<float>(width) / static_cast<float>(height);
    float left = -ratio;
    float right = ratio;
    float bottom = -1.0f;
    float top = 1.0f;

    _screenToScene.LoadIdentity(); // サイズが変わった際などリセット必須
    if (width > height)
    {
        float sceneW = fabsf(right - left);
        _screenToScene.Scale(sceneW / width, -sceneW / width);
    }
    else
    {
        float sceneH = fabsf(top - bottom);
        _screenToScene.Scale(sceneH / height, -sceneH / height);
    }
    _screenToScene.TranslateRelative(-width * 0.5f, -height * 0.5f);
}

void MatrixManager::ScreenToScene(float* x, float* y)
{
    *x = _screenToScene.TransformX(*x);
    *y = _screenToScene.TransformY(*y);
}

Csm::CubismMatrix44& MatrixManager::GetMvp()
{
    _p.LoadIdentity();
    _m.LoadIdentity();

    // 不清楚为什么是1.0，官方给出示例如此
    if (_mw > 1.0f && _ww < _wh)  // 确保图像不变形
    {
        // 宽较长且窗口宽小于高
        _p.Scale(1.0f, (float)_ww / (float)_wh);
        // 模型宽度固定为 2.0
        _baseScale = 2.0f / _mw;
        _m.Scale(_baseScale, _baseScale);
    }
    else
    {
        _p.Scale((float)_wh / (float)_ww, 1.0f);
        // 模型高度固定为 2.0
        _baseScale = 2.0f / _mh;
        _m.Scale(_baseScale, _baseScale);
    }

    _m.Multiply(_rotation, _m.GetArray(), _m.GetArray());
    _m.ScaleRelative(_scaleX, _scaleY);
    _m.Translate(_offsetX, _offsetY);

    _p.MultiplyByMatrix(&_m);
    return _p;
}

void MatrixManager::SetOffset(float x, float y)
{
    _offsetX = x;
    _offsetY = y;
}

void MatrixManager::SetScaleX(float sx)
{
    _scaleX = sx;
}

void MatrixManager::SetScaleY(float sy)
{
    _scaleY = sy;
}

void MatrixManager::Rotate(float deg)
{
    float r = deg / 180.0f * 3.1415926f;
    _rotation[0] = cosf(r);
    _rotation[1] = sinf(r);
    _rotation[5] = _rotation[0];
    _rotation[4] = -_rotation[1];
}

void MatrixManager::InvertTransform(float* x, float* y)
{
    // 除 projection 以外的变换需要逆变换

    // 逆平移和缩放
    float tx = (*x - _offsetX) / _scaleX;
    float ty = (*y - _offsetY) / _scaleY;

    // 逆旋转
    *x = _rotation[0] * tx - ty * -_rotation[1];
    *y = -_rotation[1] * tx + ty * _rotation[0]; 

    // 逆基础缩放
    *x = *x / _baseScale;
    *y = *y / _baseScale;
}
