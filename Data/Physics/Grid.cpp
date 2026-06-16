#include "Grid.hpp"
#include <cmath>
#include <utility>   // ОБЯЗАТЕЛЬНО: Дает компилятору шаблоны std::pair и std::make_pair
#include <algorithm> // ОБЯЗАТЕЛЬНО: Дает шаблоны std::max, std::min и std::clamp

// Подключаем внешние зависимости для структур сущностей твоего движка v15
#include "../bodies/Enemies.hpp" // Предоставляет структуру Enemy и позицию position
#include "../../main.hpp"        // Предоставляет константы MAP_WIDTH, MAP_HEIGHT и тип Bullet

namespace bunker
{

    // 1. Очищаем векторы индексов во всех 400 ячейках карты бункера (20х20)
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

    // 3. ИСПРАВЛЕНО И ВОССТАНОВЛЕНО: Распределяем индексы пуль по тайлам сетки без синтаксических разрывов!
    // Используем имя GetNearbyTiles и тип Vector3D строго как в вашем Grid.hpp
    void SpatialGridManager::GetNearbyTiles(const Vector3D &entityPos, float radius, std::vector<std::pair<int, int>> &outTileCoords) const
    {
        outTileCoords.clear();

        // Теперь компилятор поймет эти строки, так как метод официально принадлежит классу!
        int minX = WorldToGridX(entityPos.x - radius);
        int maxX = WorldToGridX(entityPos.x + radius);
        int minY = WorldToGridY(entityPos.y - radius);
        int maxY = WorldToGridY(entityPos.y + radius);

        // Резервируем память для ускорения работы
        size_t expectedSize = static_cast<size_t>((maxX - minX + 1) * (maxY - minY + 1));
        outTileCoords.reserve(expectedSize);

        // Собираем координаты ячеек
        for (int x = minX; x <= maxX; ++x)
        {
            for (int y = minY; y <= maxY; ++y)
            {
                outTileCoords.emplace_back(x, y);
            }
        }
    }

    // 5. Перевод мировых координат в индексы сетки X
    int SpatialGridManager::WorldToGridX(float worldX) const
    {
        int idx = static_cast<int>(worldX / m_TileSize);
        return std::clamp(idx, 0, MAP_WIDTH - 1);
    }

    // 6. Перевод мировых координат в индексы сетки Y
    int SpatialGridManager::WorldToGridY(float worldY) const
    {
        int idx = static_cast<int>(worldY / m_TileSize);
        return std::clamp(idx, 0, MAP_HEIGHT - 1);
    }

} // namespace bunker
