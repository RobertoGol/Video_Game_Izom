#pragma once
#include "../Types.hpp"

namespace bunker
{

    class BallisticsEngine
    {
    public:
        BallisticsEngine() = default;

        // 1. Высокоскоростной обсчет летящих пуль, дроби и сплеш-урона ракет без std::sqrt
        void UpdateProjectiles(float deltaTime, std::vector<bunker::Vertex> &vBuffer);

        // 2. Рассредоточенный обсчет спавна и оптимизация ИИ Роя Верминов
        void ProcessSwarmSpawning(float deltaTime);
    };

} // namespace bunker
