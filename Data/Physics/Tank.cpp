#include "../../main.hpp"
#include "Tank.hpp"
#include "Collisions.hpp"
#include <cmath>

// Глобальные стейты модулей титанов
VortexShieldState ionShield;
std::vector<LockOnTarget> toneLocks;

void UpdateAutonomousTankAI(float deltaTime) {
    if (titan.isPiloted) {
        titan.position = playerPos;
        
        // ОБРАБОТКА АРСЕНАЛА КЛАССОВ ТАНКА (КОГДА ИГРОК ВНУТРИ)
        TitanClass currentClass = gameClasses.GetActiveTitanClass();

        // 1. Класс Ион (Ion) — Вихревой щит на удержание клавиши 'F'
        if (currentClass == TitanClass::Ion) {
            if ((GetAsyncKeyState('F') & 0x8000) && ionShield.energy > 0.0f) {
                ionShield.isActive = true;
                ionShield.energy -= 35.0f * deltaTime; // Тратит батарею ядра
            } else {
                ionShield.isActive = false;
                // Перенаправление пойманного урона обратно в рой
                if (ionShield.caughtBulletsCount > 0) {
                    for (int i = 0; i < ionShield.caughtBulletsCount; ++i) {
                        Bullet b;
                        b.start = titan.position;
                        b.current = titan.position;
                        b.direction = { (mouseWorldPos.x - titan.position.x), (mouseWorldPos.y - titan.position.y), 0.0f };
                        float len = std::sqrt(b.direction.x * b.direction.x + b.direction.y * b.direction.y);
                        if (len > 0.1f) {
                            b.direction.x /= len; b.direction.y /= len;
                            b.type = BulletType::Standard;
                            b.speed = 32.0f;
                            bullets.push_back(b);
                        }
                    }
                    ionShield.caughtBulletsCount = 0;
                }
                if (ionShield.energy < 100.0f) ionShield.energy += 15.0f * deltaTime; // Плавный реген
            }
        }
        return;
    }

    // ЛОГИКА АВТОНОМНОГО ИИ-СПУТНИКА ВНЕ КАБИНЫ
    float tdx_t = playerPos.x - titan.position.x;
    float tdy_t = playerPos.y - titan.position.y;
    float distToPlayer = std::sqrt(tdx_t * tdx_t + tdy_t * tdy_t);

    Enemy* closestTarget = nullptr; 
    float closestDist = 8.0f;
    int enemyIdx = 0;
    int targetFoundIdx = -1;

    for (auto& e : enemies) {
        if (!e.isAlive) { enemyIdx++; continue; }
        float edx = e.position.x - titan.position.x; 
        float edy = e.position.y - titan.position.y;
        float eDist = std::sqrt(edx * edx + edy * edy);
        if (eDist < closestDist) { 
            closestDist = eDist; 
            closestTarget = &e;
            targetFoundIdx = enemyIdx;
        }
        enemyIdx++;
    }

    if (closestTarget != nullptr && titan.fireCooldown <= 0.0f) {
        Bullet tb; 
        tb.start = titan.position; 
        tb.current = titan.position;
        float bdx_t = closestTarget->position.x - titan.position.x; 
        float bdy_t = closestTarget->position.y - titan.position.y;
        float bLen = std::sqrt(bdx_t * bdx_t + bdy_t * bdy_t);
        
        if (bLen > 0.1f) {
            tb.direction = { bdx_t / bLen, bdy_t / bLen, 0.0f };
            
            // Если ИИ прошит под класс Тон (Tone) — вешаем метку захвата при выстреле
            if (gameClasses.GetActiveTitanClass() == TitanClass::Tone && targetFoundIdx != -1) {
                auto it = std::find_if(toneLocks.begin(), toneLocks.end(), [targetFoundIdx](const LockOnTarget& l) { return l.enemyIndex == targetFoundIdx; });
                if (it == toneLocks.end()) {
                    LockOnTarget newLock = { targetFoundIdx, 1, false };
                    toneLocks.push_back(newLock);
                } else {
                    it->locksCount++;
                    if (it->locksCount >= 3) it->isFullyLocked = true; // 3 попадания — цель захвачена
                }
            }

            bullets.push_back(tb); 
            titan.fireCooldown = 0.12f;
        }
    }
    if (titan.fireCooldown > 0.0f) titan.fireCooldown -= deltaTime;

    if (distToPlayer > 2.2f) {
        float nextX = titan.position.x + (tdx_t / distToPlayer) * titan.speed * deltaTime;
        float nextY = titan.position.y + (tdy_t / distToPlayer) * titan.speed * deltaTime;
        if (!CheckWorldCollision(nextX, titan.position.y, 0.55f)) titan.position.x = nextX;
        if (!CheckWorldCollision(titan.position.x, nextY, 0.55f)) titan.position.y = nextY;
    }
}
