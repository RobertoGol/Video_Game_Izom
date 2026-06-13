#include "Grid.hpp"
#include <cmath>
#include <algorithm>

namespace bunker {

void SpatialGridManager::ClearDynamicData() 
{
    // Очищаем векторы индексов во всех 400 ячейках карты бункера
    for (int x = 0; x < MAP_WIDTH; ++x) 
    {
        for (int y = 0; y < MAP_HEIGHT; ++y) 
        {
            m_Cells[x][y].enemyIndices.clear();
            m_Cells[x][y].bulletIndices.clear();
        }
    }
}

void SpatialGridManager::InsertEnemies(const std::vector<Enemy>& globalEnemies) 
{
    for (size_t i = 0; i < globalEnemies.size(); ++i) 
    {
        if (!globalEnemies[i].isAlive) continue;

        // Рассчитываем, в какой тайл попал центр хитбокса монстра
        int gx = WorldToGridX(globalEnemies[i].position.x);
        int gy = WorldToGridY(globalEnemies[i].position.y);

        m_Cells[gx][gy].enemyIndices.push_back(i);
    }
}

void SpatialGridManager::InsertBullets(const std::vector<Bullet>& globalBullets) 
{
    for (size_t i = 0; i & outTileCoords) const 
{
    outTileCoords.clear();

    // Вычисляем охватывающую рамку (AABB) объекта в координатах сетки
    int minX = WorldToGridX(entityPos.x - radius);
    int maxX = WorldToGridX(entityPos.x + radius);
    int minY = WorldToGridY(entityPos.y - radius);
    int maxY = WorldToGridY(entityPos.y + radius);

    // Блокируем выход за границы индексов массива 20х20
    minX = std::max(0, minX); maxX = std::min(MAP_WIDTH - 1, maxX);
    minY = std::max(0, minY); maxY = std::min(MAP_HEIGHT - 1, maxY);

    // Собираем координаты только тех тайлов, которые физически перекрываются хитбоксом
    for (int x = minX; x <= maxX; ++x) 
    {
        for (int y = minY; y <= maxY; ++y) 
        {
            outTileCoords.push_back({ x, y });
        }
    }
}

} // namespace bunker
