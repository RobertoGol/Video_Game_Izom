$code = @'
#include "Grid.hpp"
#include <cmath>
#include <utility>               // FORCIBLY REQUIRED: Fixes std::make_pair C/C++(757) compiler blocks
#include <algorithm>             // Required for std::clamp and std::max / std::min templates
#include "../bodies/Enemies.hpp" // Чтобы компилятор знал структуру Enemy и поле position
#include "../main.hpp"           // Чтобы были доступны константы MAP_WIDTH и MAP_HEIGHT

    namespace bunker
{

    // 1. Очищаем векторы индексов во всех 400 ячейках карты бункера
    // Clear dynamic entity indices across all 400 bunker grid cells
    void SpatialGridManager::ClearDynamicCells()
    {
        for (int x = 0; x < MAP_WIDTH; ++x)
        {
            for (int y = 0; y < MAP_HEIGHT; ++y)
            {
                m_Cells[x][y].enemyIndices.clear();
                m_Cells[x][y].bulletIndices.clear();
            }
        }
    }

    // 2. Распределяем индексы врагов по тайлам сетки на основе их мировых координат
    // Map living enemy positions onto corresponding spatial grid tiles
    void SpatialGridManager::InsertEnemies(const std::vector<Enemy> &globalEnemies)
    {
        for (size_t i = 0; i < globalEnemies.size(); ++i)
        {
            if (!globalEnemies[i].isAlive)
                continue;
            int gx = WorldToGridX(globalEnemies[i].position.x);
            int gy = WorldToGridY(globalEnemies[i].position.y);
            m_Cells[gx][gy].enemyIndices.push_back(i);
        }
    }

    // 3. ВОССТАНОВЛЕНО: Распределяем индексы пуль по тайлам сетки
    // COMPLETE REPAIR: Map active flying projectiles onto spatial grid tiles
    void SpatialGridManager::InsertBullets(const std::vector<Bullet> &globalBullets)
    {
        for (size_t i = 0; i & outTileCoords)
            const
            {
                outTileCoords.clear();
                int minX = WorldToGridX(entityPos.x - radius);
                int maxX = WorldToGridX(entityPos.x + radius);
                int minY = WorldToGridY(entityPos.y - radius);
                int maxY = WorldToGridY(entityPos.y + radius);

                minX = (std::max)(0, minX);
                maxX = (std::min)(MAP_WIDTH - 1, maxX);
                minY = (std::max)(0, minY);
                maxY = (std::min)(MAP_HEIGHT - 1, maxY);

                for (int x = minX; x <= maxX; ++x)
                {
                    for (int y = minY; y <= maxY; ++y)
                    {
                        outTileCoords.push_back(std::make_pair(x, y));
                    }
                }
            }

        // 5. Transform world-space X vectors into clean integer grid indices
        int SpatialGridManager::WorldToGridX(float worldX) const
        {
            int idx = static_cast<int>(worldX / m_TileSize);
            return std::clamp(idx, 0, MAP_WIDTH - 1);
        }

        // 6. Transform world-space Y vectors into clean integer grid indices
        int SpatialGridManager::WorldToGridY(float worldY) const
        {
            int idx = static_cast<int>(worldY / m_TileSize);
            return std::clamp(idx, 0, MAP_HEIGHT - 1);
        }

    } // namespace bunker
    '@ Set - Content - Path "Data\Physics\Grid.cpp" - Value $code - Encoding UTF8