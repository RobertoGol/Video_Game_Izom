#pragma once
#include <vector>
#include "../Types.hpp" // Путь к вашему Types.hpp для структур Vertex и ScreenPoint

namespace bunker {

// Глобальная инлайновая функция перевода пикселей в NDC пространство DirectX 11
inline ScreenPoint PixelsToNDC(float x, float y, float width, float height)   //  <-- тут ошибки [PixelsToNDC]
{
    return { (x / width) * 2.0f - 1.0f, 1.0f - (y / height) * 2.0f };  //  <-- тут ошибки [ { ]
}

// Инициализация статических таблиц тригонометрии (вызывается при старте в WinMain)
void InitializeRendererTables();

// Оптимизированная отрисовка кругов/сфер для хитбоксов, прицелов и спецэффектов
void PushCircle(std::vector<Vertex>& buffer, float cx, float cy, float r, float width, float height, Vertex col, int segments = 16);

} // namespace bunker
