#pragma once
#include "Types.hpp"

namespace bunker {

// Чистая inline функция преобразования координат пикселей экрана в NDC для DirectX 11
inline ScreenPoint PixelsToNDC(float px, float py) 
{
    ScreenPoint ndc;
    ndc.x = (px / 1280.0f) * 2.0f - 1.0f;
    ndc.y = 1.0f - (py / 720.0f) * 2.0f; // Инвертируем Y для DX
    return ndc;
}

} // namespace bunker

