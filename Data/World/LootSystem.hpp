#pragma once
#include <vector>
#include <string>
#include "../../Types.hpp"
#include "../../../Player/Inventory.hpp"

namespace bunker {

// Перечисление объектов строительства в стиле C.A.M.P. из Fallout 76
enum class CampObjectType 
{
    ConcreteWall,  // Железобетонная стена
    DefenseTurret, // Автоматическая защитная турель реле связи
    SupplyCrate    // Кастомный ящик для лута базы
};

// Структура текущего строительного силуэта (Ghost Preview)
struct CampPreview 
{
    int tileX = 0;
    int tileY = 0;
    CampObjectType activeType = CampObjectType::ConcreteWall;
    bool isPlacementValid = false; // Зеленый силуэт (true) / Красный силуэт (false)
};

// Структура для хранения сырой 3D-геометрии из .obj файлов папки LootAndThings
struct ObjModelMesh 
{
    std::vector<Vector3D> vertices;
    std::vector<int> indices; // Индексы треугольников для конвейера Draw
};

enum class LootContainerType 
{ 
    WoodenCrate, 
    IronSafe, 
    DevVault 
};

struct LootContainer 
{
    Vector3D position;
    LootContainerType type = LootContainerType::WoodenCrate;
    bool isOpened = false;
    float physicsRadius = 0.35f;
    std::vector<InventoryItem> containsItems;
};

class LootSystem 
{
private:
    std::vector<LootContainer> m_Containers;
    float m_ContainerSpawnTimer = 0.0f;

    // Параметры Мастерской C.A.M.P.
    bool m_CampModeActive = false;
    CampPreview m_CurrentPreview;

    // Переменные триггеров кнопок перенесены в поля класса для фикса областей видимости
    bool m_vReleased = true;
    bool m_mReleased = true;

    // Хранилище полигонов для кэшированных 3D моделей
    ObjModelMesh m_ModelConcreteWall;
    ObjModelMesh m_ModelSupplyCrate;
    ObjModelMesh m_ModelDefenseTurret;

public:
    LootSystem() = default;
    ~LootSystem() = default;

    // Высокопроизводительный построчный парсер Wavefront .obj файлов
    bool LoadObjGeometry(const std::string& filePath, ObjModelMesh& meshOut);

    // Первичный импорт всей папки LootAndThings при старте сессии
    void Initialize3DModelsRegistry();

    // Динамический спавн сундуков на свободных тайлах сессии
    void UpdateLootSpawning(float deltaTime);

    // Процедурный рандомайзер содержимого сундука на основе весов редкости
    void GenerateRandomLoot(LootContainer& containerOut);

    // Механика взаимодействия Пилота с сундуком (Клавиша 'E') с сетевой синхронизацией
    void ProcessContainerInteraction(Vector3D& pPos);

    // Сетевой хэндлер: Вскрытие сундука на клиентах по сигналу из сети
    void RemoteOpenCrate(float worldX, float worldY);

    // Модернизированный процессор Мастерской C.A.M.P. в стиле Fallout 4/76 и Garry's Mod
    void ProcessDeveloperTools(HWND hwnd, const Vector3D& pilotPos);

    // Отрисовка 3D-силуэта постройки на основе загруженного .obj меша
    void DrawCampPreviewGhost(std::vector<Vertex>& vBuffer);

    // Универсальный метод проецирования 3D моделей в изометрический мир
    void Render3DModelAt(std::vector<Vertex>& vBuffer, const ObjModelMesh& mesh, const Vector3D& worldPos, float r, float g, float b, float alpha);

    // Generation Чит-Предметов и Оружия Разработчика
    void GiveDeveloperKit(PlayerInventory& playerInvOut);

    // Сброс и кэширование при смене ячейки карты
    void ClearContainers() { m_Containers.clear(); }
    std::vector<LootContainer>& GetContainers() { return m_Containers; }
};

extern LootSystem g_LootSystem;

} // namespace bunker
