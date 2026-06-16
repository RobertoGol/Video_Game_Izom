#pragma once
#include <algorithm>
#include <vector>
#include "../Types.hpp" // Нативный Types.hpp вашего движка

namespace bunker
{

    // Опережающее объявление типов для разрыва циклических зависимостей
    struct Enemy;
    struct Bullet;

    // Структура ячейки сетки для кэширования динамических объектов (пуль/врагов)
    struct GridCell
    {
        std::vector<size_t> enemyIndices;  // Индексы врагов, находящихся в этом тайле
        std::vector<size_t> bulletIndices; // Индексы летящих снарядов
    };

    class SpatialGridManager
    {
    private:
        // Двухмерный массив ячеек, соответствующий размерам карты Убежища 17 (20х20)
        GridCell m_Cells[MAP_WIDTH][MAP_HEIGHT];
        const float m_TileSize = 1.0f; // Размер одного тайла в мировых координатах

    public:
        SpatialGridManager() = default;

        // ДОБАВЛЕНО: Явное объявление метода очистки ячеек для исправления ошибки C2039/C2556
        void ClearDynamicCells();

        // Быстрая вставка индексов сущностей в соответствующие ячейки на основе их позиций
        void InsertEnemies(const std::vector<Enemy> &globalEnemies);
        void InsertBullets(const std::vector<Bullet> &globalBullets);

        // Метод сбора соседних ячеек (9 тайлов вокруг центра) для локального обсчета коллизий
        void GetNearbyTiles(const Vector3D &entityPos, float radius, std::vector<std::pair<int, int>> &outTileCoords) const;

        // Быстрый перевод мировых координат с плавающей точкой в целочисленные индексы сетки
        inline int WorldToGridX(float worldX) const { return std::clamp(static_cast<int>(worldX / m_TileSize), 0, MAP_WIDTH - 1); }
        inline int WorldToGridY(float worldY) const { return std::clamp(static_cast<int>(worldY / m_TileSize), 0, MAP_HEIGHT - 1); }
    };

} // namespace bunker
