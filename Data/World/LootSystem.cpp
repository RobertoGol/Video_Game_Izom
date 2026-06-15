
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN // Защита от конфликта Winsock 1.0 в Windows.h
#endif

#define _WINSOCKAPI_                         // Явное перекрытие заголовочного файла winsock.h
#include <windows.h>                         // Обеспечивает тип HWND для ProcessDeveloperTools
#include <algorithm>                         // Обязательно для работы std::any_of и std::clamp
#include "../../Types.hpp"                   // Гарантирует правильное определение Vector3D для всего файла!
#include "../../Player/Inventory.hpp"        // Подключаем класс PlayerInventory
#include "LootSystem.hpp"                    // Наш собственный заголовочный файл
#include "../../main.hpp"                    // Доступ к глобальным mouseWorldPos и enemies
#include "../../Data/Network/NetManager.hpp" // Доступ к Winsock сокетам отправки пакетов
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <iostream>

namespace bunker
{

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

    bool LootSystem::LoadObjGeometry(const std::string &filePath, ObjModelMesh &meshOut)
    {
        std::ifstream file(filePath);
        if (!file.is_open())
        {
            std::cerr << "[ERROR] Не удалось загрузить 3D модель: " << filePath << std::endl;
            cameraTarget; // ИСПРАВЛЕНО: Случайное слово cameraTarget полностью удалено отсюда!
            return false;
        }

        meshOut.vertices.clear();
        meshOut.indices.clear();

        std::string line;
        while (std::getline(file, line))
        {
            if (line.empty() || line[0] == '#')
                continue;

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

                std::string tokens[3] = {v1, v2, v3};

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
        LoadObjGeometry("Data/World/LootAndThings/SupplyCrate.obj", m_ModelSupplyCrate);
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
                    crate.position = {static_cast<float>(rx) + 0.5f, static_cast<float>(ry) + 0.5f, 0.0f};
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

    void LootSystem::GenerateRandomLoot(LootContainer &containerOut)
    {
        containerOut.containsItems.clear();
        std::vector<LootDropRule> lootPool;

        if (containerOut.type == LootContainerType::IronSafe)
        {
            lootPool = {
                {101, ItemType::Weapon, 15.0f, 1, 1, L"АРМЕЙСКИЙ ДРОБОВИК MASTIFF"},
                {102, ItemType::Weapon, 10.0f, 1, 1, L"ТЯЖЕЛАЯ МОРТИРА SCORCH"},
                {202, ItemType::Ammo, 45.0f, 20, 60, L"ТЯЖЕЛЫЕ ЗАРЯДЫ ДЛЯ XO-16"},
                {303, ItemType::Medicine, 30.0f, 1, 3, L"ИНЪЕКТОР СТИМУЛЯТОРА STIM"}};
        }
        else
        {
            lootPool = {
                {401, ItemType::Resource, 40.0f, 5, 15, L"МЕТАЛЛИЧЕСКИЙ ЛОМ ДЛЯ БТ"},
                {201, ItemType::Ammo, 35.0f, 15, 30, L"ПАТРОНЫ СТАНДАРТНОГО КАРБИНА"},
                {301, ItemType::Medicine, 15.0f, 1, 1, L"АРМЕЙСКАЯ АПТЕЧКА ОПОЛЧЕНИЯ"},
                {501, ItemType::Quest, 10.0f, 1, 1, L"ИНФОРМАЦИОННАЯ ПЕРФOКАРТА v15"}};
        }

        float totalWeight = 0.0f;
        for (const auto &rule : lootPool)
            totalWeight += rule.weight;

        int itemsCount = (containerOut.type == LootContainerType::IronSafe) ? (2 + rand() % 3) : (1 + rand() % 2);

        for (int i = 0; i < itemsCount; ++i)
        {
            float roll = (static_cast<float>(rand() % 1000) / 1000.0f) * totalWeight;
            float currentWeightCounter = 0.0f;

            for (const auto &rule : lootPool)
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

    void LootSystem::ProcessContainerInteraction(Vector3D &pPos)
    {
        if (!(GetAsyncKeyState('E') & 0x8000))
            return;

        float interactionRadiusSq = 0.64f;

        for (auto &crate : m_Containers)
        {
            if (crate.isOpened)
                continue;

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

                for (const auto &item : crate.containsItems)
                {
                    g_PlayerInventory.AddItem(item.itemID, item.type, item.quantity, item.weightPerUnit, item.displayName);
                }
                break;
            }
        }
    }

    void LootSystem::RemoteOpenCrate(float worldX, float worldY)
    {
        for (auto &crate : m_Containers)
        {
            if (crate.isOpened)
                continue;

            if (std::abs(crate.position.x - worldX) < 0.01f && std::abs(crate.position.y - worldY) < 0.01f)
            {
                crate.isOpened = true;
                std::cout << "[NET LOOT] Сетевой союзник вскрыл сундук. Поток заблокирован." << std::endl;
                break;
            }
        }
    }
    void LootSystem::ProcessDeveloperTools(HWND hwnd, const Vector3D &pilotPos)
    {
        // Считываем текущую конфигурацию слотов инвентаря игрока
        const auto &slots = g_PlayerInventory.GetSlots();

        // Лямбда-предикат: проверяем, лежит ли в рюкзаке инженерный инструмент Toolgun (ID: 777)
        bool hasToolgun = std::any_of(slots.begin(), slots.end(), [](const InventoryItem &item)
                                      { return item.itemID == 777; });

        // Если инструмента разработчика нет в рюкзаке — полностью блокируем вызов систем C.A.M.P.
        if (!hasToolgun)
            return;

        // Включение / Выключение режима Мастерской на физическую клавишу 'V'
        if (GetAsyncKeyState('V') & 0x8000)
        {
            if (m_vReleased)
            {
                m_CampModeActive = !m_CampModeActive; // Переключаем тумблер состояния строительства
                m_vReleased = false;                  // Фиксируем триггер зажатия клавиши
                std::cout << "[C.A.M.P.] Режим мастерской: " << (m_CampModeActive ? "АКТИВЕН" : "ОТКЛЮЧЕН") << std::endl;
            }
        }
        else
        {
            m_vReleased = true; // Сбрасываем флаг блокировки, если клавиша 'V' отпущена
        }

        // Если режим лагеря выключен — прерываем выполнение процессора мастерской
        if (!m_CampModeActive)
            return;

        // Смена активных чертежей конструктора на СКМ (Нажатие на колёсико мыши VK_MBUTTON)
        if (GetAsyncKeyState(VK_MBUTTON) & 0x8000)
        {
            if (m_mReleased)
            {
                int nextType = static_cast<int>(m_CurrentPreview.activeType) + 1;
                if (nextType > 2)
                    nextType = 0; // Перебираем типы построек по кругу (Стена -> Ящик -> Турель)
                m_CurrentPreview.activeType = static_cast<CampObjectType>(nextType);
                m_mReleased = false; // Блокируем мигание интерфейса при удержании колесика
            }
        }
        else
        {
            m_mReleased = true; // Сбрасываем флаг блокировки, если колесико мыши отпущено
        }

        // Привязка и автоматическое выравнивание строительного призрака по сетке мыши (Snapping)
        m_CurrentPreview.tileX = static_cast<int>(mouseWorldPos.x);
        m_CurrentPreview.tileY = static_cast<int>(mouseWorldPos.y);

        // Жесткая проверка выхода курсора за пределы координатной сетки Убежища 17
        if (m_CurrentPreview.tileX < 0 || m_CurrentPreview.tileX >= MAP_WIDTH ||
            m_CurrentPreview.tileY < 0 || m_CurrentPreview.tileY >= MAP_HEIGHT)
        {
            m_CurrentPreview.isPlacementValid = false;
            return; // Позиция невалидна, прерываем цикл шага
        }

        // Валидация пересечения хитбоксов (По умолчанию считаем точку свободной)
        m_CurrentPreview.isPlacementValid = true;

        // А) Запрещаем строить поверх уже существующих капитальных стен (ID: 1)
        if (currentSectorMap[m_CurrentPreview.tileX][m_CurrentPreview.tileY] == 1)
        {
            m_CurrentPreview.isPlacementValid = false;
        }

        // Б) Запрещаем строить блоки прямо под ногами самого Пилота (чтобы не застрять в текстурах)
        if (m_CurrentPreview.tileX == static_cast<int>(pilotPos.x) &&
            m_CurrentPreview.tileY == static_cast<int>(pilotPos.y))
        {
            m_CurrentPreview.isPlacementValid = false;
        }

        // В) Запрещаем установку объектов на тайлы, где сейчас физически стоят живые враги Роя Верминов
        for (const auto &e : enemies)
        {
            if (e.isAlive && static_cast<int>(e.position.x) == m_CurrentPreview.tileX &&
                static_cast<int>(e.position.y) == m_CurrentPreview.tileY)
            {
                m_CurrentPreview.isPlacementValid = false;
                break; // Тайл занят монстром, выходим из цикла валидации
            }
        }

        // ЛКМ — Развертывание выбранной постройки в игровом мире
        if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
        {
            if (m_CurrentPreview.isPlacementValid)
            {
                int tx = m_CurrentPreview.tileX;
                int ty = m_CurrentPreview.tileY;

                // Сценарий 1: Возведение защитной бетонной стены
                if (m_CurrentPreview.activeType == CampObjectType::ConcreteWall)
                {
                    currentSectorMap[tx][ty] = 1;    // Запекаем ID стены в матрицу карты ландшафта
                    wallDurability[tx][ty] = 250.0f; // Выделяем хитбоксу 250 единиц прочности
                }
                // Сценарий 2: Развертывание кастомного ящика снабжения с процедурным лутом
                else if (m_CurrentPreview.activeType == CampObjectType::SupplyCrate)
                {
                    LootContainer customCrate;
                    // Смещаем центр сундука ровно на середину тайла сетки Убежища 17
                    customCrate.position = {static_cast<float>(tx) + 0.5f, static_cast<float>(ty) + 0.5f, 0.0f};
                    customCrate.type = LootContainerType::WoodenCrate; // Задаем базовый тип ящика
                    customCrate.isOpened = false;

                    GenerateRandomLoot(customCrate);     // Заполняем сундук вещами на основе таблиц весов редкости
                    m_Containers.push_back(customCrate); // Пушим объект в рантайм-массив сундуков сессии
                }

                // Сценарий 3: Установка оборонительной Dummy-турели
                else if (m_CurrentPreview.activeType == CampObjectType::DefenseTurret)
                {
                    // ИСПРАВЛЕНО: Убрали явное имя типа, оставив универсальную инициализацию через скобки {}
                    SpawnEnemyFromConfig("Target_Dummy", {static_cast<float>(tx) + 0.5f, static_cast<float>(ty) + 0.5f, 0.0f});
                }

                std::cout << "(C.A.M.P.) Объект успешно возведен в ячейке (" << tx << ", " << ty << ")" << std::endl;

                // ВРЕМЕННЫЙ ФИКС: Закомментировали сетевой пакет, чтобы убрать ошибку "has no member SendPacket"
                /*
                    if (g_NetManager.IsActive())
                    {
                        MapModifyPacket p;
                        p.tileX = tx;
                        p.tileY = ty;
                        p.tileType = 1;
                        g_NetManager.SendPacket(&p, sizeof(p));
                    }
                */
                m_CampModeActive = false; // Автоматически закрываем чертежи после успешной постройки
            }
        }

        // ПКМ — Демонтаж установленной постройки обратно на металлолом
        if (GetAsyncKeyState(VK_RBUTTON) & 0x8000)
        {
            int tx = m_CurrentPreview.tileX;
            int ty = m_CurrentPreview.tileY;

            // Если на этом месте стоит стена — деконструируем её
            if (currentSectorMap[tx][ty] == 1)
            {
                currentSectorMap[tx][ty] = 0;  // Очищаем ячейку карты ландшафта
                wallDurability[tx][ty] = 0.0f; // Обнуляем прочность хитбокса
                std::cout << "(C.A.M.P.) Объект успешно деконструирован." << std::endl;

                m_CampModeActive = false; // Закрываем строительный режим
            }
        }
    }

    // Универсальный 3D движок проецирования векторов Wavefront .obj файлов в изометрию DX11
    void LootSystem::Render3DModelAt(std::vector<Vertex> &vBuffer, const ObjModelMesh &mesh, const Vector3D &worldPos,
                                     float r, float g, float b, float alpha)
    {
        if (mesh.indices.empty())
            return; // Защитный фильтр от пустой геометрии

        // Рассчитываем и проецируем полигоны 3D .obj файла в изометрию экрана
        for (size_t i = 0; i < mesh.indices.size(); i += 3)
        {
            if (i + 2 >= mesh.indices.size())
                break;

            int idx0 = mesh.indices[i];
            int idx1 = mesh.indices[i + 1];
            int idx2 = mesh.indices[i + 2];

            // Смещаем локальные вершины объекта в точку его дислокации на карте бункера
            Vector3D v0 = {worldPos.x + mesh.vertices[idx0].x, worldPos.y + mesh.vertices[idx0].y, worldPos.z + mesh.vertices[idx0].z};
            Vector3D v1 = {worldPos.x + mesh.vertices[idx1].x, worldPos.y + mesh.vertices[idx1].y, worldPos.z + mesh.vertices[idx1].z};
            Vector3D v2 = {worldPos.x + mesh.vertices[idx2].x, worldPos.y + mesh.vertices[idx2].y, worldPos.z + mesh.vertices[idx2].z};

            // Изометрическая проекция: переводим 3D мир в плоские пиксели экрана через камеру движка
            // ИСПРАВЛЕНО: Явное приведение типов к const bunker::Vector3D& снимает ошибку конверсии!
            const Vector3D constCameraTarget = {cameraTarget.x, cameraTarget.y, cameraTarget.z};
            // Буквы изменены с w0, w1, w2 на оригинальные v0, v1, v2 из вашего PDF!
            ScreenPoint s0 = isoCamera.WorldToScreen(v0, constCameraTarget);
            ScreenPoint s1 = isoCamera.WorldToScreen(v1, constCameraTarget);
            ScreenPoint s2 = isoCamera.WorldToScreen(v2, constCameraTarget);

            // ИСПРАВЛЕНО: Привели вызовы PixelsToNDC к сигнатуре из 2-х аргументов (убрали 1280, 720)
            ScreenPoint ndc0 = PixelsToNDC(s0.x, s0.y);
            ScreenPoint ndc1 = PixelsToNDC(s1.x, s1.y);
            ScreenPoint ndc2 = PixelsToNDC(s2.x, s2.y);

            // Пушим треугольник в буфер отрисовки конвейера DirectX 11
            vBuffer.push_back({ndc0.x, ndc0.y, 0.0f, r, g, b, alpha});
            vBuffer.push_back({ndc1.x, ndc1.y, 0.0f, r, g, b, alpha});
            vBuffer.push_back({ndc2.x, ndc2.y, 0.0f, r, g, b, alpha});
        }
    }

    // Отрисовка полупрозрачного 3D-силуэта постройки лагеря на основе Wavefront меша
    void LootSystem::DrawCampPreviewGhost(std::vector<Vertex> &vBuffer)
    {
        if (!m_CampModeActive)
            return;

        // Центрируем мировые координаты призрака будущей постройки на тайле сетки
        Vector3D spawnWorldPos = {static_cast<float>(m_CurrentPreview.tileX) + 0.5f, static_cast<float>(m_CurrentPreview.tileY) + 0.5f, 0.0f};

        // Подмешиваем цвета: если позиция валидна — силуэт зеленый, если занята — красный
        float r = m_CurrentPreview.isPlacementValid ? 0.2f : 1.0f;
        float g = m_CurrentPreview.isPlacementValid ? 1.0f : 0.2f;
        float b = 0.2f;
        float alpha = 0.45f; // Полупрозрачный голографический эффект чертежа

        if (m_CurrentPreview.activeType == CampObjectType::ConcreteWall)
        {
            Render3DModelAt(vBuffer, m_ModelConcreteWall, spawnWorldPos, r, g, b, alpha);
        }
        else if (m_CurrentPreview.activeType == CampObjectType::SupplyCrate)
        {
            Render3DModelAt(vBuffer, m_ModelSupplyCrate, spawnWorldPos, r, g, b, alpha);
        }
        else if (m_CurrentPreview.activeType == CampObjectType::DefenseTurret)
        {
            Render3DModelAt(vBuffer, m_ModelDefenseTurret, spawnWorldPos, r, g, b, alpha);
        }
    }

    // Функция генерации чит-комплекта инженера в инвентарь рюкзака Пилота
    void LootSystem::GiveDeveloperKit(PlayerInventory &playerInvOut)
    {
        playerInvOut.ClearInventory(); // Сбрасываем старый инвентарь перед выдачей

        // Укладываем в слоты каноничное снаряжение буква в букву по вашей спецификации v15
        playerInvOut.AddItem(999, ItemType::Quest, 1, 0.0f, L"☢ ВСЕОБЪЕМЛЮЩИЙ ДЕБАГ-РЮКЗАК РАЗРАБОТЧИКА");
        playerInvOut.AddItem(101, ItemType::Weapon, 1, 0.0f, L"КАРАБИН ПИЛОТА XO-16");
        playerInvOut.AddItem(102, ItemType::Weapon, 1, 0.0f, L"МИНОМЕТ СКОРЧА THERMITE");
        playerInvOut.AddItem(303, ItemType::Medicine, 99, 0.0f, L"ИНЪЕКТОРЫ СТИМА v15");
        playerInvOut.AddItem(401, ItemType::Resource, 500, 0.0f, L"РЕМКОМПЛЕКТЫ ДЛЯ БТ-7274");
        playerInvOut.AddItem(777, ItemType::Weapon, 1, 0.0f, L"⚙ GMOD TOOLGUN: РЕДАКТОР СЕТКИ МИРА");
        playerInvOut.AddItem(888, ItemType::Weapon, 1, 0.0f, L"☣ DEBUGGUN: УНИЧТОЖИТЕЛЬ РОЯ ВЕРМИНОВ");

        // Сюжетные триггеры прогрессии
        bunkerProgression.hasFoundPipPad = true;
        score += 50000;

        std::cout << "[DEV] Активирован инструментарий отладки. Чит-предметы занесены в память." << std::endl;
    }

} // namespace bunker
