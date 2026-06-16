#pragma once
#include <vector>
#include <utility> // ОБЯЗАТЕЛЬНО: Дает классу понимание std::pair
#include <algorithm>
#include "../Types.hpp" // Путь к твоим структурам Vector3D и ScreenPoint
// #include <SFML/System/Vector2.hpp>  // & outTileCoords) const;
namespace bunker
{

    // Опережающие объявления, чтобы разорвать циклическую зависимость инклудов
    struct Enemy;
    struct Bullet;

    struct GridCell
    {
        std::vector<size_t> enemyIndices;
        std::vector<size_t> bulletIndices;
    };

    class SpatialGridManager
    {
    private:
        GridCell m_Cells[MAP_WIDTH][MAP_HEIGHT];
        const float m_TileSize = 1.0f;

    public:
        SpatialGridManager() = default;
        ~SpatialGridManager() = default;

        // Объявляем метод очистки ячеек кадра
        void ClearDynamicCells();

        // Быстрая вставка индексов сущностей в ячейки сетки
        void InsertEnemies(const std::vector<Enemy> &globalEnemies);
        void InsertBullets(const std::vector<Bullet> &globalBullets);

        // Метод сбора соседних ячеек (9 тайлов вокруг центра) для локального обсчета коллизий
        // ВАЖНО: outTileCoords теперь жестко прописан как аргумент-ссылка
        void GetNearbyTiles(const Vector3D &entityPos, float radius, std::vector<std::pair<int, int>> &outTileCoords) const;

        // Быстрый перевод мировых координат в целочисленные индексы сетки
        int WorldToGridX(float worldX) const;
        int WorldToGridY(float worldY) const;
    };

} // namespace bunker
