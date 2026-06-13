#include "HUD.hpp"
#include "../../main.hpp"  //  <-- тут ошибки 
#include "../PlayerController.hpp"
#include <algorithm>

namespace bunker {

// Вспомогательная функция для генерации плоского 2D прямоугольника из двух треугольников (6 вершин)
static void PushFlatBar(std::vector<Vertex>& vBuffer, float x, float y, float width, float height, float r, float g, float b, float a) 
{
    // Перевод из экранных пикселей (1280х720) в NDC координаты DirectX 11 (-1.0 до 1.0)
    auto ToNDC = [](float px, float py) -> ScreenPoint {
        ScreenPoint ndc;
        ndc.x = (px / 1280.0f) * 2.0f - 1.0f;
        ndc.y = 1.0f - (py / 720.0f) * 2.0f; // Инвертируем Y для DX
        return ndc;
    };

    ScreenPoint topLeft  = ToNDC(x, y);
    ScreenPoint topRight = ToNDC(x + width, y);
    ScreenPoint botLeft  = ToNDC(x, y + height);
    ScreenPoint botRight = ToNDC(x + width, y + height);

    // Треугольник 1
    vBuffer.push_back({ topLeft.x,  topLeft.y,  0.0f, r, g, b, a });
    vBuffer.push_back({ topRight.x, topRight.y, 0.0f, r, g, b, a });
    vBuffer.push_back({ botLeft.x,  botLeft.y,  0.0f, r, g, b, a });

    // Треугольник 2
    vBuffer.push_back({ botLeft.x,  botLeft.y,  0.0f, r, g, b, a });
    vBuffer.push_back({ topRight.x, topRight.y, 0.0f, r, g, b, a });
    vBuffer.push_back({ botRight.x, botRight.y, 0.0f, r, g, b, a });
}

void RenderTacticalHUD(std::vector<Vertex>& vBuffer, float wWidth, float wHeight) 
{
    // Экземпляр контроллера для считывания текущего запаса стамины
    static PlayerController hudController;

    // Слой 0: Общая рамка Pip-Pad по периметру экрана (Фирменный аналоговый зеленый люминофор)
    float activeR = 0.2f, activeG = 1.0f, activeB = 0.2f;

    // --- РЕНДЕРИНГ HUD В ЗАВИСИМОСТИ ОТ УПРАВЛЯЕМОГО ТРАНСПОРТА / СУЩНОСТИ ---
    if (playerMode == UnitMode::Scout) 
    {
        // ==========================================
        // ИНТЕРФЕЙС ПЕХОТИНЦА (Scout Mode HUD)
        // ==========================================
        
        // 1. Шкала Здоровья (Красный неон)
        float hpPercent = std::clamp(playerHealth / playerMaxHealth, 0.0f, 1.0f);
        PushFlatBar(vBuffer, 40.0f, 640.0f, 250.0f, 14.0f, 0.2f, 0.2f, 0.2f, 0.5f); // Фон
        PushFlatBar(vBuffer, 40.0f, 640.0f, 250.0f * hpPercent, 14.0f, 1.0f, 0.2f, 0.2f, 0.8f5); // Индикатор

        // 2. Шкала Очков Действия / Выносливости (Желто-зеленый неон Helldivers)
        float apPercent = hudController.GetStaminaPercent();
        PushFlatBar(vBuffer, 40.0f, 660.0f, 200.0f, 8.0f, 0.2f, 0.2f, 0.2f, 0.5f);
        PushFlatBar(vBuffer, 40.0f, 660.0f, 200.0f * apPercent, 8.0f, 0.8f, 0.9f, 0.2f, 0.8f);

        // 3. Индикатор Эфирной Эрозии (Фиолетовое свечение v15)
        float erosionPercent = std::clamp(playerErosionLevel / 100.0f, 0.0f, 1.0f);
        PushFlatBar(vBuffer, 40.0f, 675.0f, 150.0f, 6.0f, 0.1f, 0.1f, 0.1f, 0.6f);
        PushFlatBar(vBuffer, 40.0f, 675.0f, 150.0f * erosionPercent, 6.0f, 0.6f, 0.2f, 0.8f, 0.7f);
    } 
    else if (playerMode == UnitMode::Titan) 
    {
        // ==========================================
        // ИНТЕРФЕЙС БТ-7274 (Vanguard-AI Heavy HUD)
        // ==========================================
        
        // Массивные оранжево-зеленые шкалы индустриального робота
        activeR = 0.9f; activeG = 0.45f; activeB = 0.1f;

        // 1. Индикатор Энергии Вортекс-Щита (Левый нижний угол)
        // Завязано на внутреннее состояние титана, передаваемое из Tank.cpp
        float vortexPercent = titan.systems.coreEnergy / 100.0f; // Переиспользуем coreEnergy под буфер щита
        PushFlatBar(vBuffer, 50.0f, 610.0f, 300.0f, 18.0f, 0.1f, 0.2f, 0.1f, 0.4f);
        PushFlatBar(vBuffer, 50.0f, 610.0f, 300.0f * vortexPercent, 18.0f, 0.3f, 0.6f, 1.0f, 0.8f5); // Синее вихревое поле

        // 2. Давление пара в котле реактора (Винтажный датчик)
        float tracksConditionPercent = titan.systems.tracksCondition / 100.0f;
        PushFlatBar(vBuffer, 50.0f, 635.0f, 300.0f, 10.0f, 0.2f, 0.1f, 0.0f, 0.4f);
        PushFlatBar(vBuffer, 50.0f, 635.0f, 300.0f * tracksConditionPercent, 10.0f, activeR, activeG, activeB, 0.8f);

        // 3. Шкала заряда Ультимативного Ядра (Burst Core)
        float corePercent = std::fmod(gameState.sessionTime * 0.05f, 1.0f); // Заглушка накопления до интеграции полной переменной
        PushFlatBar(vBuffer, 50.0f, 655.0f, 300.0f, 25.0f, 0.2f, 0.2f, 0.2f, 0.5f);
        PushFlatBar(vBuffer, 50.0f, 655.0f, 300.0f * corePercent, 25.0f, 1.0f, 0.7f, 0.0f, 0.9f); // Золотой перегруз котла
    }
    else 
    {
        // ==========================================
        // ЗАДЕЛ: HUD ДЛЯ РЕТРО-АВТОМОБИЛЕЙ И МОТОЦИКЛОВ
        // ==========================================
        
        // Монохромный янтарный интерфейс приборов ДВС
        activeR = 1.0f; activeG = 0.7f; activeB = 0.0f;

        // 1. Аналоговый Спидометр / Тахометр (Горизонтальная пиксельная лента)
        float speedPercent = std::abs(titan.speed / 10.0f); // Условный расчет скорости
        PushFlatBar(vBuffer, 440.0f, 650.0f, 400.0f, 20.0f, 0.15f, 0.1f, 0.0f, 0.5f);
        PushFlatBar(vBuffer, 440.0f, 650.0f, 400.0f * speedPercent, 20.0f, activeR, activeG, activeB, 0.85f);

        // 2. Датчик температуры масла / Давления пара в поршнях
        PushFlatBar(vBuffer, 440.0f, 675.0f, 180.0f, 8.0f, 0.2f, 0.2f, 0.2f, 0.5f);
        PushFlatBar(vBuffer, 440.0f, 675.0f, 130.0f, 8.0f, 0.9f, 0.6f, 0.0f, 0.8f);
    }
}

} // namespace bunker
