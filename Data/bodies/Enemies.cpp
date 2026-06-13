#include "Enemies.hpp"
#include "../../main.hpp" // Доступ к глобальным массивам enemies, playerPos, towerPosition
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>

namespace bunker {

// Статический кэш загруженных профилей .aut, чтобы не читать диск во время боя!
static std::vector<std::pair<std::string, AutEnemyProfile>> g_AutProfilesRegistry;

bool LoadEnemyProfileFromAut(const std::string& filePath, AutEnemyProfile& profileOut) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "[ERROR] Не удалось открыть файл автоматизации: " << filePath << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        // Убираем пробелы и пропускаем строки комментариев (в стиле // или #)
        if (line.empty() || line[0] == '/' || line[0] == '#') continue;

        std::stringstream ss(line);
        std::string key;
        std::string value;

        if (std::getline(ss, key, '=') && std::getline(ss, value)) {
            // Удаляем возможные символы возврата каретки \r из Windows-файлов
            if (!value.empty() && value.back() == '\r') value.pop_back();

            if (key == "type") {
                if (value == "VerminSwarm")  profileOut.classType = EnemyClassType::VerminSwarm;
                if (value == "TargetDummy")  profileOut.classType = EnemyClassType::TargetDummy;
                if (value == "PunchingBag")  profileOut.classType = EnemyClassType::PunchingBag;
                if (value == "StaticBase")   profileOut.classType = EnemyClassType::StaticBase;
            }
            else if (key == "health")    { profileOut.maxHealth = std::stof(value); }
            else if (key == "speed")     { profileOut.baseSpeed = std::stof(value); }
            else if (key == "radius")    { profileOut.physicalRadius = std::stof(value); }
            else if (key == "erosion")   { profileOut.erosionDamage = std::stof(value); }
            else if (key == "score")     { profileOut.rewardScore = std::stoi(value); }
            else if (key == "faction")   { profileOut.factionID = value; }
            else if (key == "name") {
                // Быстрая конвертация строки в широкую строку wstring для рендеринга надписей HUD
                profileOut.displayName = std::wstring(value.begin(), value.end());
            }
        }
    }

    file.close();
    return true;
}

void InitializeEnemiesRegistry() {
    g_AutProfilesRegistry.clear();

    // Список базовых конфигурационных файлов автоматизации из вашей папки Type_Enemies
    std::vector<std::string> autFiles = {
        "Data/bodies/Type_Enemies/base.aut",
        "Data/bodies/Type_Enemies/Punching_Bag.aut",
        "Data/bodies/Type_Enemies/Target_Dummy.aut"
    };

    for (const auto& path : autFiles) {
        AutEnemyProfile profile;
        if (LoadEnemyProfileFromAut(path, profile)) {
            // Извлекаем чистое имя файла (например, "Punching_Bag") в качестве ключа
            size_t lastSlash = path.find_last_of("/\\");
            size_t lastDot = path.find_last_of(".");
            std::string keyName = path.substr(lastSlash + 1, lastDot - lastSlash - 1);

            g_AutProfilesRegistry.push_back({ keyName, profile });
            std::cout << "[REGISTRY] Успешно кэширован профиль врага из файла: " << keyName << ".aut" << std::endl;
        }
    }
}

void SpawnEnemyFromConfig(const std::string& autConfigName, const Vector3D& spawnPosition) {
    // Высокоскоростной поиск профиля в кэшированном массиве памяти (без дисковых операций!)
    auto it = std::find_if(g_AutProfilesRegistry.begin(), g_AutProfilesRegistry.end(),
        [&autConfigName](const std::pair<std::string, AutEnemyProfile>& item) {
            return item.first == autConfigName;
        });

    if (it != g_AutProfilesRegistry.end()) {
        const AutEnemyProfile& p = it->second;

        // Конструируем стандартную сущность Enemy из вашего Types.hpp
        Enemy e;
        e.position = spawnPosition;
        e.health = p.maxHealth;
        e.speed = p.baseSpeed;
        e.radius = p.physicalRadius;
        e.isAlive = true;

        // Пушим в глобальный массив симуляции Mechanics.cpp
        enemies.push_back(e);
    } else {
        std::cerr << "[WARNING] Попытка спавна ненайденной конфигурации: " << autConfigName << std::endl;
    }
}

void UpdateEnemiesAI(float deltaTime) {
    // Логика пошагового перемещения ИИ-монстров (вызывается из ProcessGameMechanics кадра)
    for (auto& e : enemies) {
        if (!e.isAlive) continue;

        // Если у врага скорость равна 0 (например, статичная плита base.aut или манекен), ИИ для него отключается
        if (e.speed < 0.01f) continue;

        // Стандартный паттерн штурма: бег к вышке связи или переключение на игрока при сближении
        float dx = playerPos.x - e.position.x;
        float dy = playerPos.y - e.position.y;
        float distToPlayerSq = dx * dx + dy * dy;

        float targetX = towerPosition.x;
        float targetY = towerPosition.y;

        if (distToPlayerSq < 49.0f) { // Радиус обнаружения игрока 7 клеток (49 в квадрате)
            targetX = playerPos.x;
            targetY = playerPos.y;
        }

        float tdx = targetX - e.position.x;
        float tdy = targetY - e.position.y;
        float len = std::sqrt(tdx * tdx + tdy * tdy);

        if (len > 0.05f) {
            e.position.x += (tdx / len) * e.speed * deltaTime;
            e.position.y += (tdy / len) * e.speed * deltaTime;
        }
    }
}

} // namespace bunker
