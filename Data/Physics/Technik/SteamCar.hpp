#pragma once
#include "../../../Data/Types.hpp"
#include "../Mechanics.hpp" // ОБЯЗАТЕЛЬНО: Дает коду ниже увидеть функцию CheckWorldCollision
#include <cmath>            // Проверь, есть ли эта строка!

namespace bunker
{
    // ГАРАНТИРОВАННЫЙ ПРОБРОС ДЛЯ КОМПИЛЯТОРА:
    bool CheckWorldCollision(float x, float y, float radius);

    class SteamCarSimulation
    {
    private:
        float m_SteamPressure = 0.0f;
        const float m_MaxPressure = 240.0f;
        float m_HullAngle = 0.0f;
        Vector3D m_Velocity = {0.0f, 0.0f, 0.0f};

    public:
        SteamCarSimulation() = default;

        void UpdateCarPhysics(Vector3D &carPos, float deltaTime)
        {
            Vector3D inputDir = {0.0f, 0.0f, 0.0f};
            if (GetAsyncKeyState('W') & 0x8000)
            {
                inputDir.x += 1.0f;
                inputDir.y -= 1.0f;
                m_SteamPressure = (std::min)(m_MaxPressure, m_SteamPressure + 40.0f * deltaTime);
            }
            else
            {
                m_SteamPressure = (std::max)(0.0f, m_SteamPressure - 60.0f * deltaTime);
            }

            if (GetAsyncKeyState('S') & 0x8000)
            {
                inputDir.x -= 1.0f;
                inputDir.y += 1.0f;
            }
            if (GetAsyncKeyState('A') & 0x8000)
            {
                m_HullAngle -= 1.5f * deltaTime;
            }
            if (GetAsyncKeyState('D') & 0x8000)
            {
                m_HullAngle += 1.5f * deltaTime;
            }

            float targetSpeed = (m_SteamPressure / m_MaxPressure) * 6.5f; // Зависимость от пара
            Vector3D targetVel = {std::cos(m_HullAngle) * targetSpeed, std::sin(m_HullAngle) * targetSpeed, 0.0f};

            m_Velocity.x += (targetVel.x - m_Velocity.x) * 2.0f * deltaTime; // Тяжелый разгон
            m_Velocity.y += (targetVel.y - m_Velocity.y) * 2.0f * deltaTime;

            float nextX = carPos.x + m_Velocity.x * deltaTime;
            float nextY = carPos.y + m_Velocity.y * deltaTime;

            if (!CheckWorldCollision(nextX, carPos.y, 0.5f))
                carPos.x = nextX;
            if (!CheckWorldCollision(carPos.x, nextY, 0.5f))
                carPos.y = nextY;
        }

        float GetPressurePercent() const { return m_SteamPressure / m_MaxPressure; }
    };

} // namespace bunker
