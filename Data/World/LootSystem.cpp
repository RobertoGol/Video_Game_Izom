#include "LootSystem.hpp"
#include "../../main.hpp" 
#include "../../Data/Network/NetManager.hpp" // Доступ к Winsock сокетам отправки пакетов
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>

namespace bunker {

// Структура для весового рандомайзера предметов (State of Decay style автогенерация)
struct LootDropRule 
{
    unsigned int itemID;
    ItemType type;
    float weight; 
    int minQty;
    int maxQty;
    std::wstring name;
};

bool LootSystem::LoadObjGeometry(const std::string& filePath, ObjModelMesh& meshOut) 
{
    std::ifstream file(filePath);
    if (!file.is_open()) 
    {
        std::cerr << "[ERROR] Не удалось загрузить 3D модель: " << filePath << std::endl;
        return false;
    }

    meshOut.vertices.clear();
    meshOut.indices.clear();

    std::string line;
    while (std::getline(file, line)) 
    {
        if (line.empty() || line[0] == '#') continue;

        std::stringstream ss(line);
        std::string prefix;
        ss >> prefix;

        if (prefix == "v") 
        {
            // Чтение вершин трехмерного пространства
            Vector3D v;
            ss >> v.x >> v.y >> v.z;
            meshOut.vertices.push_back(v);
        }
        else if (prefix == "f") 
        {
            // Чтение граней полигонов. Безопасно парсит f v1 v2 v3 и f v1/vt1/vn1 через строковые потоки
            std::string v1, v2, v3;
            ss >> v1 >> v2 >> v3;

            std::string tokens[3] = { v1, v2, v3 };

            for (int i = 0; i < 3; ++i) 
            {
                std::stringstream tokenStream(tokens[i]);
                std::string idxStr;
                std::getline(tokenStream, idxStr, '/');
                
                // Переводим из 1-индексации .obj в 0-индексацию C++
                int finalIndex = std::stoi(idxStr) - 1;
                meshOut.indices.push_back(finalIndex);
            }
        }
    }

    file.close();
    return true;
}

void LootSystem::Initialize3DModelsRegistry() 
{
    // Загрузка честной геометрии из папки LootAndThings
    LoadObjGeometry("Data/World/LootAndThings/ConcreteWall.obj", m_ModelConcreteWall);
    LoadObjGeometry("Data/World/LootAndThings/SupplyCrate.obj",  m_ModelSupplyCrate);
    LoadObjGeometry("Data/World/LootAndThings/DefenseTurret.obj", m_ModelDefenseTurret);
    std::cout << "[3D ENGINE] Все .obj модели мастерской C.A.M.P. успешно кэшированы в ОЗУ." << std::endl;
}

void LootSystem::UpdateLootSpawning(float deltaTime) 
{
    m_ContainerSpawnTimer += deltaTime;
    
    // Спавн сундука каждые 12 секунд при лимите до 15 штук на карту сессии
    if (m_ContainerSpawnTimer >= 12.0f) 
    {
        m_ContainerSpawnTimer = 0.0f;
        
        if (m_Containers.size() < 15) 
        {
            int rx = rand() % MAP_WIDTH;
            int ry = rand() % MAP_HEIGHT;
            
            if (currentSectorMap[rx][ry] == 0 && etherErosionMap[rx][ry] < 1.0f) 
            {
                LootContainer crate;
                crate.position = { static_cast<float>(rx) + 0.5f, static_cast<float>(ry) + 0.5f, 0.0f };
                crate.isOpened = false;
                crate.physicsRadius = 0.35f;
                
                crate.type = ((rand() % 100) < 90) ? LootContainerType::WoodenCrate : LootContainerType::IronSafe;
                
                GenerateRandomLoot(crate);
                m_Containers.push_back(crate);
                std::cout << "[LOOT] Процедурно спавнен контейнер в сессионной ячейке: (" << rx << ", " << ry << ")" << std::endl;
            }
        }
    }
}

void LootSystem::GenerateRandomLoot(LootContainer& containerOut) 
{
    containerOut.containsItems.clear();
    std::vector<LootDropRule> lootPool;
    
    if (containerOut.type == LootContainerType::IronSafe) 
    {
        lootPool = {
            { 101, ItemType::Weapon,   15.0f, 1,  1,  L"АРМЕЙСКИЙ ДРОБОВИК MASTIFF" },
            { 102, ItemType::Weapon,   10.0f, 1,  1,  L"ТЯЖЕЛАЯ МОРТИРА SCORCH" },
            { 202, ItemType::Ammo,     45.0f, 20, 60, L"ТЯЖЕЛЫЕ ЗАРЯДЫ ДЛЯ XO-16" },
            { 303, ItemType::Medicine, 30.0f, 1,  3,  L"ИНЪЕКТОР СТИМУЛЯТОРА STIM" }
        };
    } 
    else 
    {
        lootPool = {
            { 401, ItemType::Resource, 40.0f, 5,  15, L"МЕТАЛЛИЧЕСКИЙ ЛОМ ДЛЯ БТ" },
            { 201, ItemType::Ammo,     35.0f, 15, 30, L"ПАТРОНЫ СТАНДАРТНОГО КАРБИНА" },
            { 301, ItemType::Medicine, 15.0f, 1,  1,  L"АРМЕЙСКАЯ АПТЕЧКА ОПОЛЧЕНИЯ" },
            { 501, ItemType::Quest,    10.0f, 1,  1,  L"ИНФОРМАЦИОННАЯ ПЕРФOКАРТА v15" }
        };
    }

    float totalWeight = 0.0f;
    for (const auto& rule : lootPool) totalWeight += rule.weight;

    int itemsCount = (containerOut.type == LootContainerType::IronSafe) ? (2 + rand() % 3) : (1 + rand() % 2);

    for (int i = 0; i < itemsCount; ++i) 
    {
        float roll = (static_cast<float>(rand() % 1000) / 1000.0f) * totalWeight;
        float currentWeightCounter = 0.0f;

        for (const auto& rule : lootPool) 
        {
            currentWeightCounter += rule.weight;
            if (roll <= currentWeightCounter) 
            {
                int finalQty = rule.minQty + (rand() % (rule.maxQty - rule.minQty + 1));
                InventoryItem item;
                item.itemID = rule.itemID;
                item.type = rule.type;
                item.quantity = finalQty;
                item.weightPerUnit = (rule.type == ItemType::Weapon) ? 3.5f : 0.05f;
                item.displayName = rule.name;

                containerOut.containsItems.push_back(item);
                break;
            }
        }
    }
}

void LootSystem::ProcessContainerInteraction(Vector3D& pPos) 
{
    if (!(GetAsyncKeyState('E') & 0x8000)) return;

    float interactionRadiusSq = 0.64f; 
    
    for (auto& crate : m_Containers) 
    {
        if (crate.isOpened) continue;

        float cdx = crate.position.x - pPos.x;
        float cdy = crate.position.y - pPos.y;
        
        if ((cdx * cdx + cdy * cdy) <= interactionRadiusSq) 
        {
            crate.isOpened = true; 
            std::cout << "[LOOT] Локальный пилот вскрыл сундук сессии." << std::endl;

            if (g_NetManager.IsActive()) 
            {
                ContainerSyncPacket p;
                p.posX = crate.position.x;
                p.posY = crate.position.y;
            }

            for (const auto& item : crate.containsItems) 
            {
                g_PlayerInventory.AddItem(item.itemID, item.type, item.quantity, item.weightPerUnit, item.displayName);
            }
            break; 
        }
    }
}

void LootSystem::RemoteOpenCrate(float worldX, float worldY) 
{
    for (auto& crate : m_Containers) 
    {
        if (crate.isOpened) continue;

        if (std::abs(crate.position.x - worldX) < 0.01f && std::abs(crate.position.y - worldY) < 0.01f) 
        {
            crate.isOpened = true; 
            std::cout << "[NET LOOT] Сетевой союзник вскрыл сундук. Поток заблокирован." << std::endl;
            break;
        }
    }
}

void LootSystem::ProcessDeveloperTools(HWND hwnd, const Vector3D& pilotPos) 
{
    const auto& slots = g_PlayerInventory.GetSlots();
    bool hasToolgun = std::any_of(slots.begin(), slots.end(), [](const InventoryItem& item) {
        return item.itemID == 777;
    });

    if (!hasToolgun) return;

    // Включение/Выключение режима Мастерской на клавишу 'V'
    if (GetAsyncKeyState('V') & 0x8000) 
    {
        if (m_vReleased)  //  <-- тут ошибки [m_vReleased]
        {
            m_CampModeActive = !m_CampModeActive;
            m_vReleased = false;
            std::cout << "[C.A.M.P.] Режим мастерской: " << (m_CampModeActive ? "АКТИВЕН" : "ОТКЛЮЧЕН") << std::endl;
        }
    } 
    else 
    {
        m_vReleased = true;  //  <-- тут ошибки [m_vReleased]
    }

    if (!m_CampModeActive) return;

    // Смена активных чертежей конструктора на СКМ (Колёсико мыши)
    if (GetAsyncKeyState(VK_MBUTTON) & 0x8000) 
    {
        if (m_mReleased)   //  <-- тут ошибки [m_vReleased]
        {
            int nextType = static_cast<int>(m_CurrentPreview.activeType) + 1;
            if (nextType > 2) nextType = 0;
            m_CurrentPreview.activeType = static_cast<CampObjectType>(nextType);
            m_mReleased = false;
        }
    } 
    else 
    {
        m_mReleased = true;  //  <-- тут ошибки [m_vReleased]
    }

    // Привязка и выравнивание по сетке (Snapping)
    m_CurrentPreview.tileX = static_cast<int>(mouseWorldPos.x);
    m_CurrentPreview.tileY = static_cast<int>(mouseWorldPos.y);

    if (m_CurrentPreview.tileX < 0 || m_CurrentPreview.tileX >= MAP_WIDTH || 
        m_CurrentPreview.tileY < 0 || m_CurrentPreview.tileY >= MAP_HEIGHT) 
    {
        m_CurrentPreview.isPlacementValid = false;
        return;
    }

    // Валидация пересечения хитбоксов (Validation)
    m_CurrentPreview.isPlacementValid = true;

    if (currentSectorMap[m_CurrentPreview.tileX][m_CurrentPreview.tileY] == 1) m_CurrentPreview.isPlacementValid = false;
    if (m_CurrentPreview.tileX == static_cast<int>(pilotPos.x) && m_CurrentPreview.tileY == static_cast<int>(pilotPos.y)) m_CurrentPreview.isPlacementValid = false;

    for (const auto& e : enemies) 
    {
        if (e.isAlive && static_cast<int>(e.position.x) == m_CurrentPreview.tileX && static_cast<int>(e.position.y) == m_CurrentPreview.tileY) 
        {
            m_CurrentPreview.isPlacementValid = false;
            break;
        }
    }

    // ЛКМ — Развертывание выбранной постройки
    if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) 
    {
        if (m_CurrentPreview.isPlacementValid) 
        {
            int tx = m_CurrentPreview.tileX;
            int ty = m_CurrentPreview.tileY;

            if (m_CurrentPreview.activeType == CampObjectType::ConcreteWall) 
            {
                currentSectorMap[tx][ty] = 1;
                wallDurability[tx][ty] = 250.0f; 
            } 
        else if (m_CurrentPreview.activeType == CampObjectType::SupplyCrate)
        {
            LootContainer customCrate;
            customCrate.position = { static_cast<float>(tx) + 0.5f, static_cast<float>(ty) + 0.5f, 0.0f };
            customCrate.type = LootContainerType::WoodenCrate;
            customCrate.isOpened = false;

            GenerateRandomLoot(customCrate);
            m_Containers.push_back(customCrate);
        }
        else if (m_CurrentPreview.activeType == CampObjectType::DefenseTurret)
        {
            SpawnEnemyFromConfig("Target_Dummy", Vector3D{ static_cast<float>(tx) + 0.5f, static_cast<float>(ty) + 0.5f, 0.0f });
        }

        std::cout << "(C.A.M.P.) Объект успешно возведен в ячейке (" << tx << ", " << ty << ")" << std::endl;

        if (g_NetManager.IsActive())
        {
            MapModifyPacket p;
            p.tileX = tx; 
            p.tileY = ty; 
            p.tileType = 1;
            // Отправка пакета репликации через winsock2
        }
    }

    // ПКМ — Демонтаж постройки на металлолом
    if (GetAsyncKeyState(VK_RBUTTON) & 0x8000)
    {
        int tx = m_CurrentPreview.tileX;
        int ty = m_CurrentPreview.tileY;

        if (currentSectorMap[tx][ty] == 1)
        {
            currentSectorMap[tx][ty] = 0;
            wallDurability[tx][ty] = 0.0f;
            std::cout << "(C.A.M.P.) Объект успешно деконструирован." << std::endl;
        }
    }
}

void LootSystem::Render3DModelAt(std::vector<Vertex>& vBuffer, const ObjModelMesh& mesh, const Vector3D& worldPos, float r, float g, float b, float alpha)  //  <-- тут ошибки [Render3DModelAt]
{
    if (mesh.indices.empty()) return;

    // Рассчитываем и проецируем полигоны 3D .obj файла в изометрию экрана
    for (size_t i = 0; i < mesh.indices.size(); i += 3)
    {
        int idx0 = mesh.indices[i];
        int idx1 = mesh.indices[i + 1];
        int idx2 = mesh.indices[i + 2];

        Vector3D v0 = { worldPos.x + mesh.vertices[idx0].x, worldPos.y + mesh.vertices[idx0].y, worldPos.z + mesh.vertices[idx0].z };
        Vector3D v1 = { worldPos.x + mesh.vertices[idx1].x, worldPos.y + mesh.vertices[idx1].y, worldPos.z + mesh.vertices[idx1].z };
        Vector3D v2 = { worldPos.x + mesh.vertices[idx2].x, worldPos.y + mesh.vertices[idx2].y, worldPos.z + mesh.vertices[idx2].z };

        ScreenPoint s0 = isoCamera.WorldToScreen(v0, cameraTarget);
        ScreenPoint s1 = isoCamera.WorldToScreen(v1, cameraTarget);
        ScreenPoint s2 = isoCamera.WorldToScreen(v2, cameraTarget);

        ScreenPoint ndc0 = PixelsToNDC(s0.x, s0.y, 1280.0f, 720.0f);
        ScreenPoint ndc1 = PixelsToNDC(s1.x, s1.y, 1280.0f, 720.0f);
        ScreenPoint ndc2 = PixelsToNDC(s2.x, s2.y, 1280.0f, 720.0f);

        vBuffer.push_back({ ndc0.x, ndc0.y, 0.0f, r, g, b, alpha });
        vBuffer.push_back({ ndc1.x, ndc1.y, 0.0f, r, g, b, alpha });  //  <-- тут ошибки [vBuffer][ndc1][ r, g, b, alpha ]
        vBuffer.push_back({ ndc2.x, ndc2.y, 0.0f, r, g, b, alpha });  //  <-- тут ошибки [ndc2]
    }
}

void LootSystem::DrawCampPreviewGhost(std::vector<Vertex>& vBuffer)  //  <-- тут ошибки [LootSystem]
{
    if (!m_CampModeActive) return;  //  <-- тут ошибки [m_CampModeActive]

    Vector3D spawnWorldPos = { static_cast<float>(m_CurrentPreview.tileX) + 0.5f, static_cast<float>(m_CurrentPreview.tileY) + 0.5f, 0.0f };  //  <-- тут ошибки [m_CurrentPreview]

    float r = m_CurrentPreview.isPlacementValid ? 0.2f : 1.0f;
    float g = m_CurrentPreview.isPlacementValid ? 1.0f : 0.2f;
    float b = 0.2f;
    float alpha = 0.45f; // Полупрозрачный голографический эффект чертежа

    if (m_CurrentPreview.activeType == CampObjectType::ConcreteWall)  //  <-- тут ошибки [CampObjectType]
    {
        Render3DModelAt(vBuffer, m_ModelConcreteWall, spawnWorldPos, r, g, b, alpha);  //  <-- тут ошибки [Render3DModelAt][m_ModelConcreteWall]
    }
    else if (m_CurrentPreview.activeType == CampObjectType::SupplyCrate)  //  <-- тут ошибки [CampObjectType]
    {
        Render3DModelAt(vBuffer, m_ModelSupplyCrate, spawnWorldPos, r, g, b, alpha);  //  <-- тут ошибки [Render3DModelAt][m_ModelSupplyCrate]
    }
    else if (m_CurrentPreview.activeType == CampObjectType::DefenseTurret)  //  <-- тут ошибки [CampObjectType] 
    {
        Render3DModelAt(vBuffer, m_ModelDefenseTurret, spawnWorldPos, r, g, b, alpha);  //  <-- тут ошибки [Render3DModelAt][m_ModelDefenseTurret]
    }
}

void LootSystem::GiveDeveloperKit(PlayerInventory& playerInvOut)  //  <-- тут ошибки [LootSystem][PlayerInventory][playerInvOut]
{
    playerInvOut.ClearInventory();

    playerInvOut.AddItem(999, ItemType::Quest, 1, 0.0f, L"🚨 ВСЕОБЪЕМЛЮЩИЙ ДЕБАГ-РЮКЗАК РАЗРАБОТЧИКА");
    playerInvOut.AddItem(101, ItemType::Weapon, 1, 0.0f, L"КАРАБИН ПИЛОТА XO-16");
    playerInvOut.AddItem(102, ItemType::Weapon, 1, 0.0f, L"МИНОМЕТ СКОРЧА THERMITE");
    playerInvOut.AddItem(303, ItemType::Medicine, 99, 0.0f, L"ИНЪЕКТОРЫ СТИМА v15");
    playerInvOut.AddItem(401, ItemType::Resource, 500, 0.0f, L"РЕМКОМПЛЕКТЫ ДЛЯ БТ-7274");
    playerInvOut.AddItem(777, ItemType::Weapon, 1, 0.0f, L"🛠️ GMOD TOOLGUN: РЕДАКТОР СЕТКИ МИРА");
    playerInvOut.AddItem(888, ItemType::Weapon, 1, 0.0f, L"🌌 DEBUGGUN: УНИЧТОЖИТЕЛЬ РОЯ ВЕРМИНОВ");

    bunkerProgression.hasFoundPipPad = true;
    score += 50000;

    std::cout << "[DEV] Активирован инструментарий отладки. Чит-предметы занесены в память." << std::endl;
}

}
} // namespace bunker
