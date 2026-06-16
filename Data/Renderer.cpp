#include "Renderer.hpp"
#include "../main.hpp"
#include <cmath>
#include <algorithm>

namespace bunker
{
    void InitializeRendererTables()
    {
        // ваш код инициализации таблиц рендеринга
    }

    // --- РЕТРО АНИМАЦИОННЫЙ СТЕК СПРАЙТШИТОВ (DirectX 11 NDC) ---
    struct SpriteAnimFrame
    {
        float uMin, vMin, uMax, vMax;
    };

    // Генерация 4-х вершин текстурированного квада (2 треугольника) с расчетом 8 ракурсов
    void PushAnimatedSpriteSheet(std::vector<Vertex> &buffer, float worldX, float worldY, float spriteWidth, float spriteHeight,
                                 float facingAngleDegrees, int currentFrameIdx, int totalFramesInRow, int textureWidth, int textureHeight)
    {
        // 1. Конвертируем чистый угол 360° в один из 8 изометрических рядов спрайтшита (0 - 7)
        int directionRow = 0;
        float normalizedAngle = fmod(facingAngleDegrees + 22.5f, 360.0f);
        if (normalizedAngle < 0.0f)
            normalizedAngle += 360.0f;
        directionRow = static_cast<int>(normalizedAngle / 45.0f);

        // Маппинг рядов под каноничный разворот изометрической сетки вашего движка
        int isoRowMapping[] = {6, 5, 4, 3, 2, 1, 0, 7};
        int targetRow = isoRowMapping[directionRow];

        // 2. Шаг нарезки сетки атласа по пикселям
        float frameWidthTexel = static_cast<float>(textureWidth) / static_cast<float>(totalFramesInRow);
        float frameHeightTexel = static_cast<float>(textureHeight) / 8.0f; // 8 рядов направлений

        // Расчет UV координат сдвига кадра для шейдера RetroShader.hpp
        float uMin = (static_cast<float>(currentFrameIdx) * frameWidthTexel) / static_cast<float>(textureWidth);
        float uMax = uMin + (frameWidthTexel / static_cast<float>(textureWidth));
        float vMin = (static_cast<float>(targetRow) * frameHeightTexel) / static_cast<float>(textureHeight);
        float vMax = vMin + (frameHeightTexel / static_cast<float>(textureHeight));

        // 3. Перевод мировых координат в экранные изометрические пиксели через камеру
        Vector3D worldPos = {worldX, worldY, 0.0f};
        ScreenPoint screenCenter = isoCamera.WorldToScreen(worldPos, cameraTarget);

        // Строим геометрию пиксель-арт спрайта вокруг центра точки опоры (Pivot Point)
        float halfW = spriteWidth * 0.5f;
        float topY = screenCenter.y - spriteHeight; // Точка опоры спрайта — ноги персонажа

        // ИСПРАВЛЕНО: Теперь все 4 вызова используют правильное имя PixelsToNDC и строго 2 аргумента!
        ScreenPoint topLeft = PixelsToNDC(screenCenter.x - halfW, topY);
        ScreenPoint topRight = PixelsToNDC(screenCenter.x + halfW, topY);
        ScreenPoint botLeft = PixelsToNDC(screenCenter.x - halfW, screenCenter.y);
        ScreenPoint botRight = PixelsToNDC(screenCenter.x + halfW, screenCenter.y);

        // Запись геометрии биллборда в вершинный буфер видеокарты
        buffer.push_back({topLeft.x, topLeft.y, 0.0f, uMin, vMin, 1.0f, 1.0f});
        buffer.push_back({topRight.x, topRight.y, 0.0f, uMax, vMin, 1.0f, 1.0f});
        buffer.push_back({botLeft.x, botLeft.y, 0.0f, uMin, vMax, 1.0f, 1.0f});

        buffer.push_back({botLeft.x, botLeft.y, 0.0f, uMin, vMax, 1.0f, 1.0f});
        buffer.push_back({topRight.x, topRight.y, 0.0f, uMax, vMin, 1.0f, 1.0f});
        buffer.push_back({botRight.x, botRight.y, 0.0f, uMax, vMax, 1.0f, 1.0f});
    }

    // ... функции InitializeRendererTables и PushCircle остаются ниже без изменений ...

} // namespace bunker
