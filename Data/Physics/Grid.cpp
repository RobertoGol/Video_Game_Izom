#include "../../main.hpp"
#include "Grid.hpp"
#include <cmath>

void ProcessVault17GridCascade(float deltaTime) {
    // 1. ЕСЛИ ВЫШКА СВЯЗИ УНИЧТОЖЕНА ВЕРМИНАМИ
    if (regionalGrid.towerHealth <= 0.0f) {
        regionalGrid.towerHealth = 0.0f;
        regionalGrid.towerSyncRecovered = false;  // Вышка падает
        regionalGrid.localRelayAvailable = false; // Каскад 1: Локальное реле Pip-Pad тухнет
        regionalGrid.feyRingGateUnlocked = false; // Каскад 2: Врата эвакуации FeyRing блокируются
    }

    // 2. МЕХАНИКА ВОССТАНОВЛЕНИЯ (Из твоих лорных файлов)
    // Если Скаут-оперативник (или Танк) находится в радиусе 1.5 метров от вышки, идет ручная синхронизация
    float dx = playerPos.x - towerPosition.x;
    float dy = playerPos.y - towerPosition.y;
    float distToTower = std::sqrt(dx * dx + dy * dy);

    if (distToTower <= 1.5f && !regionalGrid.towerSyncRecovered) {
        // Запускается протокол восстановления вольт-интерфейса Pip-Pad
        regionalGrid.towerHealth += 25.0f * deltaTime; // Медленный подъем прочности
        if (regionalGrid.towerHealth >= 200.0f) {
            regionalGrid.towerHealth = 200.0f;
            regionalGrid.towerSyncRecovered = true;   // Вышка снова в сети!
            regionalGrid.localRelayAvailable = true;  // Каскад оживает: реле Pip-Pad доступно
            regionalGrid.feyRingGateUnlocked = true;  // Врата FeyRing разблокированы
        }
    }
}
