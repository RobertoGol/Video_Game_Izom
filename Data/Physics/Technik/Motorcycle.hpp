#pragma once
#include "../../Types.hpp"
#include <cmath>
#include <algorithm> // Гарантирует работу std::min и std::max

namespace bunker
{

    class MotorcycleSimulation
    {
    private:
        float m_CurrentSpeed = 0.0f;
        const float m_MaxSpeed = 11.5f; // Самый быстрый на ровной поверхности
        float m_LeanAngle = 0.0f;       // Угол наклона люльки/коляски
        Vector3D m_Velocity = {0.0f, 0.0f, 0.0f};

    public:
        MotorcycleSimulation() = default;

        void UpdateMotorcyclePhysics(Vector3D &bikePos, float deltaTime)
        {
            bool moving = false;
            if (GetAsyncKeyState('W') & 0x8000)
            {
                m_CurrentSpeed = (std::min)(m_MaxSpeed, m_CurrentSpeed + 15.0f * deltaTime);
                moving = true;
            }
            if (GetAsyncKeyState('S') & 0x8000)
            {
                m_CurrentSpeed = (std::max)(-3.0f, m_CurrentSpeed - 20.0f * deltaTime);
                moving = true;
            }

            if (GetAsyncKeyState('A') & 0x8000)
            {
                m_LeanAngle -= 3.2f * deltaTime;
            } // Высокая скорость разворота
            if (GetAsyncKeyState('D') & 0x8000)
            {
                m_LeanAngle += 3.2f * deltaTime;
            }

            if (!moving)
                m_CurrentSpeed += (0.0f - m_CurrentSpeed) * 4.0f * deltaTime; // Торможение двигателем

            Vector3D targetVel = {std::cos(m_LeanAngle) * m_CurrentSpeed, std::sin(m_LeanAngle) * m_CurrentSpeed, 0.0f};
            m_Velocity.x += (targetVel.x - m_Velocity.x) * 8.0f * deltaTime; // Мгновенный отклик муфты
            m_Velocity.y += (targetVel.y - m_Velocity.y) * 8.0f * deltaTime;

            float nextX = bikePos.x + m_Velocity.x * deltaTime;
            float nextY = bikePos.y + m_Velocity.y * deltaTime;

            if (!CheckWorldCollision(nextX, bikePos.y, 0.3f))
                bikePos.x = nextX;
            // Исправлено: carPos.x заменено на bikePos.x для устранения ошибки компилятора
            if (!CheckWorldCollision(bikePos.x, nextY, 0.3f))
                bikePos.y = nextY;
        }
    };

} // namespace bunker
