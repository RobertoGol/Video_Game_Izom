#include "main.hpp"
#include "Enemies.hpp"
#include <cmath>
#include <algorithm>

// Генератор бесконечного роя Верминов за границами видимости экрана
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

// Поток логики ИИ Верминов, каскадное разрушение инфраструктуры Убежища 17
void UpdateVerminAI(float deltaTime) {
    for (auto& e : enemies) {
        if (!e.isAlive) continue;
        
        float vx = playerPos.x - e.position.x; 
        float vy = playerPos.y - e.position.y; 
        float dist = std::sqrt(vx * vx + vy * vy);

        float tdx = towerPosition.x - e.position.x; 
        float tdy = towerPosition.y - e.position.y;
        float distToTower = std::sqrt(tdx * tdx + tdy * tdy);

        // Учет каноничной Маскировки или Фазового сдвига Пилота
        bool isPlayerHidden = gameClasses.isTacticalActive && 
            (gameClasses.GetActivePilotClass() == PilotClass::Cloak || 
             gameClasses.GetActivePilotClass() == PilotClass::PhaseShift);

        if (isPlayerHidden && !titan.isPiloted) {
            dist = 999.0f; // Рой полностью теряет Скаута из виду
        }

        // Если Вермины близко к Вышке реле — они грызут её, игнорируя игрока
        if (distToTower < 4.0f && regionalGrid.towerSyncRecovered) {
            if (distToTower > 0.4f) {
                e.position.x += (tdx / distToTower) * e.speed * deltaTime;
                e.position.y += (tdy / distToTower) * e.speed * deltaTime;
            } else {
                regionalGrid.towerHealth -= 15.0f * deltaTime;
                
                // Каскадная нормализация состояний (NormalizeWorldFieldState) из лора
                if (regionalGrid.towerHealth <= 0.0f) {
                    regionalGrid.towerHealth = 0.0f;
                    regionalGrid.towerSyncRecovered = false;  
                    regionalGrid.localRelayAvailable = false; 
                    regionalGrid.feyRingGateUnlocked = false; // Врата FeyRing блокируются
                }
            }
        } else {
            // Преследование Скаута или Танка
            if (dist > 0.35f) {
                e.position.x += (vx / dist) * e.speed * deltaTime;
                e.position.y += (vy / dist) * e.speed * deltaTime;
            } else {
                // Распределение повреждений по 4 внутренним узлам Танка (Ядро, Гусеницы, Турель, Сенсоры)
                if (playerMode == UnitMode::Titan) {
                    int targetComponent = rand() % 4;
                    float damageAmt = 15.0f * deltaTime;
                    
                    if (targetComponent == 0) titan.systems.tracksCondition -= damageAmt;
                    else if (targetComponent == 1) titan.systems.turretStatus -= damageAmt;   
                    else if (targetComponent == 2) titan.systems.sensorLink -= damageAmt;     
                    else titan.systems.coreEnergy -= damageAmt;                               
                    
                    // Пересчет общего здоровья Танка на основе износа его узлов
                    playerHealth = (titan.systems.coreEnergy + titan.systems.tracksCondition + titan.systems.turretStatus + titan.systems.sensorLink) * 1.5f;
                    if (titan.systems.coreEnergy <= 0.0f) playerHealth = 0.0f; 
                } else {
                    playerHealth -= 25.0f * deltaTime;
                }
                if (playerHealth < 0.0f) playerHealth = 0.0f;
            }
        }
    }
    
    // Очистка уничтоженных Верминов из контейнера памяти
    enemies.erase(std::remove_if(enemies.begin(), enemies.end(), [](const Enemy& enemyObj) { 
        return !enemyObj.isAlive; 
    }), enemies.end());
}
