#include "../../main.hpp"  //  <-- тут ошибки 
#include "UIEffects.hpp"

void RenderUIEffects(std::vector<Vertex>& vBuffer, float width, float height) {
    float barWidth = 280.0f;
    float barHeight = 12.0f;

    // Шкала Эфирной Эрозии (В нижнем правом углу)
    float bx2 = width - barWidth - 45.0f;
    float by2 = height - 45.0f;
    float erosionRatio = playerErosionLevel / 100.0f;

    ScreenPoint erBg0 = PixelsToNDC(bx2, by2, width, height);
    ScreenPoint erBg1 = PixelsToNDC(bx2 + barWidth, by2 + barHeight, width, height);
    vBuffer.push_back({ erBg0.x, erBg0.y, 0.0f, 0.15f, 0.16f, 0.18f, 1.0f });
    vBuffer.push_back({ erBg1.x, erBg0.y, 0.0f, 0.15f, 0.16f, 0.18f, 1.0f });
    vBuffer.push_back({ erBg0.x, erBg1.y, 0.0f, 0.15f, 0.16f, 0.18f, 1.0f });
    vBuffer.push_back({ erBg1.x, erBg0.y, 0.0f, 0.15f, 0.16f, 0.18f, 1.0f });
    vBuffer.push_back({ erBg1.x, erBg1.y, 0.0f, 0.15f, 0.16f, 0.18f, 1.0f });
    vBuffer.push_back({ erBg0.x, erBg1.y, 0.0f, 0.15f, 0.16f, 0.18f, 1.0f });

    if (erosionRatio > 0.0f) {
        ScreenPoint e1 = PixelsToNDC(bx2 + (barWidth * erosionRatio), by2 + barHeight, width, height);
        Vertex eCol = { 0.85f, 0.5f, 0.05f, 1.0f }; // Предупреждающее оранжевое свечение эфира
        vBuffer.push_back({ erBg0.x, erBg0.y, 0.0f, eCol.r, eCol.g, eCol.b, eCol.a });
        vBuffer.push_back({ e1.x, erBg0.y, 0.0f, eCol.r, eCol.g, eCol.b, eCol.a });
        vBuffer.push_back({ erBg0.x, e1.y, 0.0f, eCol.r, eCol.g, eCol.b, eCol.a });
        vBuffer.push_back({ e1.x, erBg0.y, 0.0f, eCol.r, eCol.g, eCol.b, eCol.a });
        vBuffer.push_back({ e1.x, e1.y, 0.0f, eCol.r, eCol.g, eCol.b, eCol.a });
        vBuffer.push_back({ erBg0.x, e1.y, 0.0f, eCol.r, eCol.g, eCol.b, eCol.a });
    }
}
