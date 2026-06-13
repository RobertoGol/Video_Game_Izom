#pragma once
#include <vector>
#include <string>
#include "../../Types.hpp"
#include "../../Player/Inventory.hpp"

namespace bunker {

enum class CampObjectType {
    ConcreteWall,
    DefenseTurret,
    SupplyCrate
};

struct CampPreview {
    int tileX = 0;
    int tileY = 0;
    CampObjectType activeType = CampObjectType::ConcreteWall;
    bool isPlacementValid = false;
};

// Простая структура для хранения геометрии из .obj файла
struct ObjModelMesh {
    std::vector<Vector3D> vertices;
    std::vector<int> indices; // Индексы треугольников для Draw-вызова
};

enum class LootContainerType { WoodenCrate, IronSafe, DevVault };

struct LootContainer {
    Vector3D position;
    LootContainerType type = LootContainerType::WoodenCrate;
    bool isOpened = false;
    float physicsRadius = 0.35f;
    std::vector<InventoryItem> containsItems;
};

class LootSystem {
private:
    std::vector<LootContainer> m_Containers;
    float m_ContainerSpawnTimer = 0.0f;

    bool m_CampModeActive = false;
    CampPreview m_CurrentPreview;

    // --- БЛОК 3D КЭША: Хранилище полигонов для .obj моделей ---
    ObjModelMesh m_ModelConcreteWall;
    ObjModelMesh m_ModelSupplyCrate;
    ObjModelMesh m_ModelDefenseTurret;

public:
    LootSystem() = default;

    // Высокопроизводительный построчный парсер Wavefront .obj файлов
    bool LoadObjGeometry(const std::string& filePath, ObjModelMesh& meshOut);

    // Первичный импорт всей папки LootAndThings при старте сессии
    void Initialize3DModelsRegistry();

    void UpdateLootSpawning(float deltaTime);
    void GenerateRandomLoot(LootContainer& containerOut);
    void ProcessContainerInteraction(Vector3D& pPos);
    void RemoteOpenCrate(float worldX, float worldY);
    void ProcessDeveloperTools(HWND hwnd, const Vector3D& pilotPos);

    // Отрисовка 3D-силуэта постройки на основе загруженного .obj меша!
    void DrawCampPreviewGhost(std::vector<Vertex>& vBuffer);

    // Вспомогательный метод отрисовки 3D моделей в игровом мире
    void Render3DModelAt(std::vector<Vertex>& vBuffer, const ObjModelMesh& mesh, const Vector3D& worldPos, float r, float g, float b, float alpha);

    void GiveDeveloperKit(PlayerInventory& playerInvOut);

    void ClearContainers() { m_Containers.clear(); }
    std::vector<LootContainer>& GetContainers() { return m_Containers; }
};

extern LootSystem g_LootSystem;

} // namespace bunker
