#include "Map.hpp"
#include "../../../main.hpp"             // Доступ ко всей extern-шине глобальной памяти
#include "../../bodies/Enemies.hpp"     // Допуск к спавну тренировочных манекенов .aut

#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>

namespace bunker {

bool SessionWorldManager::LoadWorldSessionFromWld(const std::string& filePath) 
{
    std::string targetPath = filePath.empty() ? m_DefaultWldPath : filePath;
    std::ifstream wldFile(targetPath, std::ios::binary);

    // Если файла сессии .wld еще нет (первый запуск игры) — генерируем чистый мир с нуля
    if (!wldFile.is_open()) 
    {
        std::cout << "[WORLD] Файл сессии .wld не найден. Запуск первичного запекания мира..." << std::endl;
        
        // Зануляем матрицы
        for (int x = 0; x < MAP_WIDTH; ++x) {
            for (int y = 0; y < MAP_HEIGHT; ++y) {
                currentSectorMap[x][y] = 0;
                wallDurability[x][y] = 0.0f;
                etherErosionMap[x][y] = 0.0f;
            }
        }

        // Строим стены Командного Пункта Убежища 17 по умолчанию
        for (int x = 5; x < 15; ++x) {
            for (int y = 5; y < 15; ++y) {
                if (x == 5 || x == 14 || y == 5 || y == 14) {
                    currentSectorMap[x][y] = 1;
                    wallDurability[x][y] = 100.0f;
                }
            }
        }
        // Пробиваем герметичные шлюзы для БТ-7274
        currentSectorMap[9][5] = 0;
        currentSectorMap[9][14] = 0;

        // Расставляем базовые точки интереса и тренировочные .aut манекены
        PopulateInitialTrainingZone();

        // Сразу создаем файл сессии base.wld на диске
        SaveWorldSessionToWld(targetPath);
        return true;
    }

    // Чтение метаданных живой сессии (State of Decay style)
    wldFile.read(reinterpret_cast<char*>(&m_SessionMeta), sizeof(SessionMapMetaData));

    // Мгновенный бинарный блочный стриминг матриц карты в ОЗУ за один такт процессора
    wldFile.read(reinterpret_cast<char*>(currentSectorMap), sizeof(currentSectorMap));
    wldFile.read(reinterpret_cast<char*>(wallDurability), sizeof(wallDurability));
    wldFile.read(reinterpret_cast<char*>(etherErosionMap), sizeof(etherErosionMap));

    wldFile.close();
    std::cout << "[WORLD] Живая карта сессии '" << m_SessionMeta.currentMapName << ".wld' успешно загружена в память." << std::endl;
    return true;
}

bool SessionWorldManager::SaveWorldSessionToWld(const std::string& filePath) 
{
    std::string targetPath = filePath.empty() ? m_DefaultWldPath : filePath;
    std::ofstream wldFile(targetPath, std::ios::binary | std::ios::trunc);

    if (!wldFile.is_open()) 
    {
        std::cerr << "[ERROR] Не удалось сохранить файл сессии мира: " << targetPath << std::endl;
        return false;
    }

    // Сохраняем метаданные зачистки и ресурсов
    wldFile.write(reinterpret_cast<const char*>(&m_SessionMeta), sizeof(SessionMapMetaData));

    // Сериализуем текущее состояние разрушений стен и очагов заражения
    wldFile.write(reinterpret_cast<const char*>(currentSectorMap), sizeof(currentSectorMap));
    wldFile.write(reinterpret_cast<const char*>(wallDurability), sizeof(wallDurability));
    wldFile.write(reinterpret_cast<const char*>(etherErosionMap), sizeof(etherErosionMap));

    wldFile.close();
    return true;
}

void SessionWorldManager::PopulateInitialTrainingZone() 
{
    // Настраиваем лорные координаты вышки реле связи Убежища 17
    towerPosition.x = 9.5f;  //  <-- тут ошибки 
    towerPosition.y = 9.5f;  //  <-- тут ошибки 
    towerPosition.z = 0.0f;  //  <-- тут ошибки 
    regionalGrid.towerHealth = 200.0f;  //  <-- тут ошибки 

    // Закладываем стартовые аномалии Эрозии
    etherErosionMap[2][2] = 15.0f;
    etherErosionMap[17][17] = 30.0f;
    m_SessionMeta.activeVerminNests = 2;

    // Спавним тренировочные манекены из кэшированных .aut профилей
    SpawnEnemyFromConfig("base", Vector3D{ 14.0f, 14.0f, 0.0f });  //  <-- тут ошибки 
    SpawnEnemyFromConfig("Punching_Bag", Vector3D{ 14.0f, 15.0f, 0.0f });  //  <-- тут ошибки 
    SpawnEnemyFromConfig("Target_Dummy", Vector3D{ 15.0f, 14.0f, 0.0f });  //  <-- тут ошибки 
}

void SessionWorldManager::UpdateLiveWorldGrid(float deltaTime) 
{
    // Симуляция динамической экосистемы (State of Decay механика)
    for (int x = 0; x < MAP_WIDTH; ++x) 
    {
        for (int y = 0; y < MAP_HEIGHT; ++y) 
        {
            // 1. Динамическое разрастание очагов Эфирной Эрозии со временем сессии
            if (etherErosionMap[x][y] > 0.01f) 
            {
                etherErosionMap[x][y] = std::min(100.0f, etherErosionMap[x][y] + 0.35f * deltaTime);

                // Если Пилот наступает на зараженный тайл — накапливаем уровень Эрозии костюма
                float dx = playerPos.x - (static_cast<float>(x) + 0.5f);  //  <-- тут ошибки 
                float dy = playerPos.y - (static_cast<float>(y) + 0.5f);  //  <-- тут ошибки 
                
                if ((dx * dx + dy * dy) < 0.25f) 
                {
                    playerErosionLevel = std::min(100.0f, playerErosionLevel + 2.2f * deltaTime);
                }
            }
        }
    }

    // 2. Механика осады Убежища 17 (Если база не зачищена, уровень ресурсов падает от набегов Роя)
    if (!m_SessionMeta.isBaseCleared && regionalGrid.towerHealth > 0.0f)   //  <-- тут ошибки 
    {
        // Реле связи медленно разряжается, если вокруг бушуют гнезда аномалий
        m_SessionMeta.baseSuppliesLevel = std::max(0.0f, m_SessionMeta.baseSuppliesLevel - 0.02f * m_SessionMeta.activeVerminNests * deltaTime);
    }
}

} // namespace bunker
