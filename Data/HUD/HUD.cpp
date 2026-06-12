#include "../../main.hpp"
#include "HUD.hpp"

void RenderTacticalHUD(std::vector<Vertex>& vBuffer, float width, float height) {
    float barWidth = 280.0f;
    float barHeight = 12.0f;

    // Шкала Здоровья (В нижнем левом углу)
    float bx1 = 45.0f;
    float by1 = height - 45.0f;
    float healthRatio = playerHealth / playerMaxHealth;

    ScreenPoint hpBg0 = PixelsToNDC(bx1, by1, width, height);
    ScreenPoint hpBg1 = PixelsToNDC(bx1 + barWidth, by1 + barHeight, width, height);
    vBuffer.push_back({ hpBg0.x, hpBg0.y, 0.0f, 0.15f, 0.16f, 0.18f, 1.0f });
    vBuffer.push_back({ hpBg1.x, hpBg0.y, 0.0f, 0.15f, 0.16f, 0.18f, 1.0f });
    vBuffer.push_back({ hpBg0.x, hpBg1.y, 0.0f, 0.15f, 0.16f, 0.18f, 1.0f });
    vBuffer.push_back({ hpBg1.x, hpBg0.y, 0.0f, 0.15f, 0.16f, 0.18f, 1.0f });
    vBuffer.push_back({ hpBg1.x, hpBg1.y, 0.0f, 0.15f, 0.16f, 0.18f, 1.0f });
    vBuffer.push_back({ hpBg0.x, hpBg1.y, 0.0f, 0.15f, 0.16f, 0.18f, 1.0f });

    if (healthRatio > 0.0f) {
        ScreenPoint h1 = PixelsToNDC(bx1 + (barWidth * healthRatio), by1 + barHeight, width, height);
        Vertex hCol = (playerMode == UnitMode::Titan) ? Vertex{ 0.0f, 0.52f, 0.95f, 1.0f } : Vertex{ 0.18f, 0.85f, 0.4f, 1.0f };
        vBuffer.push_back({ hpBg0.x, hpBg0.y, 0.0f, hCol.r, hCol.g, hCol.b, hCol.a });
        vBuffer.push_back({ h1.x, hpBg0.y, 0.0f, hCol.r, hCol.g, hCol.b, hCol.a });
        vBuffer.push_back({ hpBg0.x, h1.y, 0.0f, hCol.r, hCol.g, hCol.b, hCol.a });
        vBuffer.push_back({ h1.x, hpBg0.y, 0.0f, hCol.r, hCol.g, hCol.b, hCol.a });
        vBuffer.push_back({ h1.x, h1.y, 0.0f, hCol.r, hCol.g, hCol.b, hCol.a });
        vBuffer.push_back({ hpBg0.x, h1.y, 0.0f, hCol.r, hCol.g, hCol.b, hCol.a });
    }

    // Инфикаторы состояния повреждения узлов Танка с ИИ на HUD в левом углу
    if (playerMode == UnitMode::Titan) {
        float indicatorY = (height - 45.0f) - 30.0f;
        Vertex tracksCol = (titan.systems.tracksCondition > 40.0f) ? Vertex{0.2f, 0.8f, 0.2f, 1.0f} : Vertex{0.9f, 0.2f, 0.2f, 1.0f};
        PushCircle(vBuffer, 45.0f + 10.0f, indicatorY, 4.0f, width, height, tracksCol, 4);
        
        Vertex turretCol = (titan.systems.turretStatus > 50.0f) ? Vertex{0.2f, 0.8f, 0.2f, 1.0f} : Vertex{0.9f, 0.2f, 0.2f, 1.0f};
        PushCircle(vBuffer, 45.0f + 30.0f, indicatorY, 4.0f, width, height, turretCol, 4);
    }
}
