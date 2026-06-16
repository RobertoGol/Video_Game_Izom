#pragma once
#include "../Types.hpp"

namespace bunker
{

    // Физические категории масс объектов для распределения импульса выталкивания
    enum class PhysicalMassClass
    {
        StaticWall,    // Бесконечная масса (стены бункера, колонны)
        VanguardTitan, // Тяжелая масса (40 тонн, БТ-7274)
        HumanPilot,    // Средняя масса (Пилот, тренировочные мешки)
        VerminSwarm    // Легкая масса (отдельные особи Роя)
    };

    struct CollisionBody
    {
        Vector3D *position;
        float radius;
        PhysicalMassClass massClass;
    };

    class Collisions
    {
    public:
        // 1. Честное физическое разрешение столкновения двух кругов с учетом соотношения их масс
        static bool ResolveCircleVsCircle(CollisionBody &bodyA, CollisionBody &bodyB);

        // 2. Выталкивание объекта из жесткой стены (AABB) по алгоритму кратчайшего вектора
        static bool ResolveCircleVsTileWall(Vector3D &objPos, float radius, int tileX, int tileY);
    };

} // namespace bunker
