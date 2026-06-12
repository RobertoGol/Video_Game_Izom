#include "main.hpp"
#include "Enemies.hpp"
#include "Player.hpp"
#include <cmath>
#include <algorithm>

void SpawnVerminGrid(float deltaTime) {
    enemySpawnTimer += deltaTime;
    if (enemySpawnTimer >= 1.2f) {
        Enemy e; 
        float angle = (rand() % 360) * 0.0174533f;
        e.position.x = playerPos.x + std::cos(angle) * 11.0f;
        e.position.y = playerPos.y + std::sin(angle) * 11.0f;
        enemies.push_back(e); 
        enemySpawnTimer = 0.0f;
    }
}

void UpdateVerminAI(float deltaTime) {
    for (auto& e : enemies) {
        if (!e.isAlive) continue;
        
        float vx = playerPos.x - e.position.x; 
        float vy = playerPos.y - e.position.y; 
        float dist = std::sqrt(vx * vx + vy * vy);

        float tdx = towerPosition.x - e.position.x; 
        float tdy = towerPosition.y - e.position.y;
        float distToTower = std::sqrt(tdx * tdx + tdy * tdy);

        // Проверяем тактическую маскировку или фазовый сдвиг Пилота
        bool isPlayerHidden = gameClasses.isTacticalActive && 
            (gameClasses.GetActivePilotClass() == PilotClass::Cloak || 
             gameClasses.GetActivePilotClass() == PilotClass::PhaseShift);

        // Если пилот скрылся в невидимости или фазе — Вермины теряют его из вида
        if (isPlayerHidden && !titan.isPiloted) {
            dist = 999.0f; 
        }

        // Агро-логика Верминов: приоритет вышки реле, если они в радиусе 4 метров
        if (distToTower < 4.0f && regionalGrid.towerSyncRecovered) {
            if (distToTower > 0.4f) {
                e.position.x += (tdx / distToTower) * e.speed * deltaTime;
                e.position.y += (tdy / distToTower) * e.speed * deltaTime;
            } else {
                // Вермины наносят урон инфраструктуре Убежища 17
                regionalGrid.towerHealth -= 15.0f * deltaTime;
                
                // Каскадная нормализация состояний (NormalizeWorldFieldState)
                if (regionalGrid.towerHealth <= 0.0f) {
                    regionalGrid.towerHealth = 0.0f;
                    regionalGrid.towerSyncRecovered = false;  // Вышка связи уничтожена!
                    regionalGrid.localRelayAvailable = false; // Локальное реле Pip-Pad падает
                    regionalGrid.feyRingGateUnlocked = false; // Врата эвакуации FeyRing блокируются
                }
            }
        } else {
            // Преследование пилота или Танка с ИИ
            if (dist > 0.35f) {
                e.position.x += (vx / dist) * e.speed * deltaTime;
                e.position.y += (vy / dist) * e.speed * deltaTime;
            } else {
                // Распределение повреждений по 4 внутренним узлам Танка
                if (playerMode == UnitMode::Titan) {
                    int targetComponent = rand() % 4;
                    float damageAmt = 15.0f * deltaTime;
                    
                    if (targetComponent == 0) titan.systems.tracksCondition -= damageAmt; // Поломка ходовой
                    else if (targetComponent == 1) titan.systems.turretStatus -= damageAmt;   // Поломка турели пушки
                    else if (targetComponent == 2) titan.systems.sensorLink -= damageAmt;     // Вывод из строя реле
                    else titan.systems.coreEnergy -= damageAmt;                               // Повреждение ядра
                    
                    // Каскадный пересчет общего здоровья Танка на основе износа его узлов
                    playerHealth = (titan.systems.coreEnergy + titan.systems.tracksCondition + titan.systems.turretStatus + titan.systems.sensorLink) * 1.5f;
                } else {
                    // Обычный укус Скаута вне брони Танка
                    playerHealth -= 25.0f * deltaTime;
                }
                if (playerHealth < 0.0f) playerHealth = 0.0f;
            }
        }
    }
    enemies.erase(std::remove_if(enemies.begin(), enemies.end(), [](const Enemy& enemyObj) { 
        return !enemyObj.isAlive; 
    }), enemies.end());
}
