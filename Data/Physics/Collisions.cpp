#include "Collisions.hpp"
#include <cmath>
#include <algorithm>

namespace bunker
{

    // Вспомогательная функция получения числового веса для распределения сил
    static float GetMassWeight(PhysicalMassClass massClass)
    {
        switch (massClass)
        {
        case PhysicalMassClass::StaticWall:
            return 1000000.0f; // Условная бесконечность
        case PhysicalMassClass::VanguardTitan:
            return 40000.0f; // 40 тонн
        case PhysicalMassClass::HumanPilot:
            return 100.0f; // 100 кг
        case PhysicalMassClass::VerminSwarm:
            return 15.0f; // 15 кг
        default:
            return 1.0f;
        }
    }

    bool Collisions::ResolveCircleVsCircle(CollisionBody &bodyA, CollisionBody &bodyB)
    {
        float dx = bodyB.position->x - bodyA.position->x;
        float dy = bodyB.position->y - bodyA.position->y;

        // ОПТИМИЗАЦИЯ: Работаем на квадратах расстояний до фиксации пересечения
        float distanceSq = dx * dx + dy * dy;
        float minDist = bodyA.radius + bodyB.radius;
        float minDistSq = minDist * minDist;

        if (distanceSq >= minDistSq || distanceSq < 0.0001f)
        {
            return false; // Столкновения нет
        }

        // Извлекаем корень ТОЛЬКО при подтвержденном контакте (один раз на коллизию)
        float distance = std::sqrt(distanceSq);
        float overlap = minDist - distance;

        // Нормализованный вектор направления столкновения
        float nx = dx / distance;
        float ny = dy / distance;

        // Расчет долей выталкивания на основе физических масс объектов
        float massA = GetMassWeight(bodyA.massClass);
        float massB = GetMassWeight(bodyB.massClass);
        float totalMass = massA + massB;

        // Чем тяжелее объект, тем меньше он сдвигается при ударе
        float pushFactorA = massB / totalMass;
        float pushFactorB = massA / totalMass;

        // Сдвигаем позиции векторов в мире
        bodyA.position->x -= nx * overlap * pushFactorA;
        bodyA.position->y -= ny * overlap * pushFactorA;

        bodyB.position->x += nx * overlap * pushFactorB;
        bodyB.position->y += ny * overlap * pushFactorB;

        return true;
    }

    bool Collisions::ResolveCircleVsTileWall(Vector3D &objPos, float radius, int tileX, int tileY)
    {
        // Границы тайла стены размером 1.0 х 1.0 единиц в мире
        float minX = static_cast<float>(tileX);
        float maxX = static_cast<float>(tileX) + 1.0f;
        float minY = static_cast<float>(tileY);
        float maxY = static_cast<float>(tileY) + 1.0f;

        // Находим ближайшую точку геометрии стены к центру хитбокса
        float closestX = std::clamp(objPos.x, minX, maxX);
        float closestY = std::clamp(objPos.y, minY, maxY);

        float dx = objPos.x - closestX;
        float dy = objPos.y - closestY;
        float distanceSq = dx * dx + dy * dy;

        if (distanceSq >= radius * radius || distanceSq < 0.0001f)
        {
            return false; // Объект не касается стены
        }

        float distance = std::sqrt(distanceSq);
        float overlap = radius - distance;

        // Жесткое выталкивание на 100% — стена имеет бесконечную массу
        float nx = dx / distance;
        float ny = dy / distance;

        objPos.x += nx * overlap;
        objPos.y += ny * overlap;

        return true;
    }

} // namespace bunker
