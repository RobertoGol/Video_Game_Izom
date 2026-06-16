#pragma once
#include <string>
#include <vector>
#include "../Types.hpp" // Путь к вашему Types.hpp (содержит Vector3D, Enemy)

namespace bunker
{

    // Расширенный тип врага/мишени, считываемый из файлов .aut
    enum class EnemyClassType
    {
        VerminSwarm, // Стандартная особь Роя Верминов
        TargetDummy, // Ростовая мишень из Target_Dummy.aut
        PunchingBag, // Тяжелый тренировочный мешок из Punching_Bag.aut
        StaticBase   // Неподвижная плита-основание из base.aut
    };

    struct AutEnemyProfile
    {
        EnemyClassType classType = EnemyClassType::VerminSwarm;
        float maxHealth = 35.0f;
        float baseSpeed = 2.4f;
        float physicalRadius = 0.35f;
        float erosionDamage = 0.0f;      // Насколько сильно заражает Эфирной Эрозией при укусе
        int rewardScore = 150;           // Очки за уничтожение
        std::string factionID = "Swarm"; // Идентификатор фракции для логики агро
        std::wstring displayName = L"UNKNOWN ENTITY";
    };

    // Функция инициализации: сканирует папку Type_Enemies и загружает профили
    void InitializeEnemiesRegistry();

    // Высокопроизводительный парсер конкретного .aut файла сценария
    bool LoadEnemyProfileFromAut(const std::string &filePath, AutEnemyProfile &profileOut);

    // Функция спавна конкретного типа врага по его .aut имени конфигурации
    void SpawnEnemyFromConfig(const std::string &autConfigName, const Vector3D &spawnPosition);

    // Функция обновления логики ИИ ИИ-мишеней и Роя Верминов
    void UpdateEnemiesAI(float deltaTime);

} // namespace bunker
