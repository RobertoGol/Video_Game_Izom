#include "UIEffects.hpp"
#include "../../main.hpp"
#include "../Renderer.hpp" // Предоставляет каноничный инлайн-метод PixelsToNDC
#include <cmath>
#include <algorithm>

namespace bunker
{

    // Отрисовка ЭЛТ-эффектов интерлейсинга и тактических шумов Pip-Pad матрицы
    void RenderUIEffects(std::vector<Vertex> &vBuffer, float wWidth, float wHeight)
    {
        // Задаем базовый аналоговый цвет помех экрана (Светло-зеленый люминофор)
        float eR = 0.2f, eG = 0.8f, eB = 0.2f, eA = 0.08f;

        // Слой 1: Построчный рендеринг сетки горизонтальных линий развертки (Scanlines)
        for (float y = 15.0f; y < wHeight - 15.0f; y += 4.0f)
        {
            // Переводим крайние пиксельные координаты линии экрана в формат NDC видеокарты
            ScreenPoint erBg0 = PixelsToNDC(15.0f, y);
            ScreenPoint erBg1 = PixelsToNDC(wWidth - 15.0f, y + 1.5f);

            // Строим плоскую ретро-линию интерлейсинга из двух треугольников (6 вершин)
            vBuffer.push_back({erBg0.x, erBg0.y, 0.0f, eR, eG, eB, eA});
            vBuffer.push_back({erBg1.x, erBg0.y, 0.0f, eR, eG, eB, eA});
            vBuffer.push_back({erBg0.x, erBg1.y, 0.0f, eR, eG, eB, eA});

            vBuffer.push_back({erBg0.x, erBg1.y, 0.0f, eR, eG, eB, eA});
            vBuffer.push_back({erBg1.x, erBg0.y, 0.0f, eR, eG, eB, eA});
            vBuffer.push_back({erBg1.x, erBg1.y, 0.0f, eR, eG, eB, eA});
        }

        // Слой 2: Динамический индикатор тактического сонара или критических предупреждений интерфейса
        if (playerErosionLevel > 70.0f)
        {
            // Если заражение эрозией эфира критическое, выводим мерцающий аварийный маркер в углу экрана
            ScreenPoint e1 = PixelsToNDC(wWidth - 60.0f, 40.0f);

            // Формируем геометрию сигнального пиксель-квадрата предупреждения
            vBuffer.push_back({e1.x - 0.02f, e1.y + 0.02f, 0.0f, 1.0f, 0.2f, 0.2f, 0.4f});
            vBuffer.push_back({e1.x + 0.02f, e1.y + 0.02f, 0.0f, 1.0f, 0.2f, 0.2f, 0.4f});
            vBuffer.push_back({e1.x - 0.02f, e1.y - 0.02f, 0.0f, 1.0f, 0.2f, 0.2f, 0.4f});

            vBuffer.push_back({e1.x - 0.02f, e1.y - 0.02f, 0.0f, 1.0f, 0.2f, 0.2f, 0.4f});
            vBuffer.push_back({e1.x + 0.02f, e1.y + 0.02f, 0.0f, 1.0f, 0.2f, 0.2f, 0.4f});
            vBuffer.push_back({e1.x + 0.02f, e1.y - 0.02f, 0.0f, 1.0f, 0.2f, 0.2f, 0.4f});
        }
    }

} // namespace bunker
