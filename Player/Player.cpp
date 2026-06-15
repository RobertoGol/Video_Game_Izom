#include "Player.hpp"
#include "../PlayerController.hpp" // Наш созданный ранее PlayerController    //  <-- тут ошибки 
#include "../../main.hpp"          // Доступ к глобальным переменным (bullets, titan, fireCooldown и т.д.)
#include <cmath>
#include <algorithm>

namespace bunker {

// Локальный статический экземпляр контроллера для изоляции стейта стамины и нырков
static PlayerController g_PlayerController;

void InitializePlayer() {
    playerMode = UnitMode::Scout;
    playerPos = { 5.0f, 5.0f, 0.0f };
    playerHealth = 100.0f;
    playerMaxHealth = 100.0f;
    playerSpeed = 5.5f;
    fireCooldown = 0.0f;
    cameraTarget = { 5.0f, 5.0f, 0.0f };
}

bool CheckWorldCollision(float nextX, float nextY, float radius) {
    // Быстрая проверка 4-х точек хитбокса игрока по целочисленной сетке комнат
    int checkPoints[4][2] = {
        { (int)(nextX - radius), (int)(nextY - radius) },
        { (int)(nextX + radius), (int)(nextY - radius) },
        { (int)(nextX - radius), (int)(nextY + radius) },
        { (int)(nextX + radius), (int)(nextY + radius) }
    };

    for (int i = 0; i < 4; ++i) {
        int tx = checkPoints[i][0]; 
        int ty = checkPoints[i][1];
        
        // Защита от выхода за границы массива карты 20х20
        if (tx < 0 || tx >= MAP_WIDTH || ty < 0 || ty >= MAP_HEIGHT) return true;
        if (currentSectorMap[tx][ty] == 1) return true;
    }
    return false;
}

void UpdatePlayerLogic(float deltaTime, float windowWidth, float windowHeight, const ScreenPoint& cachedMouseScreenPos) {
    // 1. Извлекаем радиус в зависимости от текущего режима Пилот/Титан
    float playerRadius = (playerMode == UnitMode::Titan) ? 0.55f : 0.25f;

    // 2. Вызываем наш облегченный PlayerController (WASD, Спринт, AP, Инерция)
    // Метод внутри себя обновит playerPos, fireCooldown и посчитает вектор скорости
    // Передаем HWND как nullptr, так как CaptureWin32Input внутри использует GetAsyncKeyState, что безопасно
    g_PlayerController.UpdateMovement(nullptr, deltaTime, windowWidth, windowHeight, playerRadius);

    // ПОДМЕНА: Принудительно перезаписываем mouseWorldPos на основе переданных кэшированных координат мыши,
    // полностью исключая вызовы GetCursorPos/ScreenToClient в этом кадре!
    currentMouseScreenPos = cachedMouseScreenPos;
    mouseWorldPos = isoCamera.ScreenToWorldGround(currentMouseScreenPos, cameraTarget);

    // 3. БОЕВОЙ АРСЕНАЛ (Логика ведения огня и распределения баллистики)
    float dx = mouseWorldPos.x - playerPos.x; 
    float dy = mouseWorldPos.y - playerPos.y;
    float dLen = std::sqrt(dx * dx + dy * dy);

    if (dLen > 0.05f && fireCooldown <= 0.0f) {
        Vector3D normDir = { dx / dLen, dy / dLen, 0.0f };

        // --- РЕЖИМ ПЕХОТИНЦА (Scout) ---
        if (playerMode == UnitMode::Scout) {
            if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
                if (isAiming) {
                    // Режим дробовика (Pellet дроби): спавним 5 лучей с быстрым случайным разбросом
                    float pelletSpread = 0.04f;
                    for (int i = 0; i < 5; ++i) {
                        Bullet p; 
                        p.start = playerPos; 
                        p.current = playerPos; 
                        p.type = BulletType::Pellet;
                        float randomSpread = ((rand() % 100) / 100.0f - 0.5f) * pelletSpread;
                        p.direction = { normDir.x + randomSpread, normDir.y + randomSpread, 0.0f };
                        bullets.push_back(p);
                    }
                    fireCooldown = 0.55f; // Кулдаун помпы дробовика
                } else {
                    // Стандартный карабин Pip-Pad (Standard Carbine)
                    Bullet b; 
                    b.start = playerPos; 
                    b.current = playerPos; 
                    b.direction = normDir;
                    b.type = BulletType::Standard; 
                    bullets.push_back(b);
                    fireCooldown = 0.22f;
                }
            }
        }
        // --- РЕЖИМ ТИТАНА/ТАНКА (Titan) ---
        else if (playerMode == UnitMode::Titan) {
            // Если турель повреждена Роем Верминов (<50%), пушка начинает люфтить и косить
            if (titan.systems.turretStatus < 50.0f) {
                float brokenFactor = ((rand() % 100) / 100.0f - 0.5f) * 0.25f;
                normDir.x += brokenFactor; 
                normDir.y += brokenFactor;
            }

            // Нажатие СКМ (Колесико) — Запуск ракетного модуля ИИ-Спутника
            if (GetAsyncKeyState(VK_MBUTTON) & 0x8000) {
                if (titan.hasMissileModule) {
                    if (titan.missileMode == MissileStrikeMode::Ballistic) {
                        // Баллистический залп: веер из 7 ракет параллельным курсом
                        for (int i = -3; i <= 3; ++i) {
                            Bullet m; 
                            m.start = playerPos; 
                            m.current = playerPos;
                            m.type = BulletType::BallisticMissile; 
                            m.speed = 15.0f; 
                            m.splashRadius = 1.8f;
                            Vector3D sideVector = { -normDir.y, normDir.x, 0.0f };
                            m.start.x += sideVector.x * (i * 0.4f); 
                            m.start.y += sideVector.y * (i * 0.4f);
                            m.direction = normDir; 
                            bullets.push_back(m);
                        }
                    } else {
                        // Артиллерийский залп: 8 минометных снарядов по площади прицела
                        for (int i = 0; i < 8; ++i) {
                            Bullet m; 
                            m.start = playerPos; 
                            m.current = playerPos;
                            m.type = BulletType::ArtilleryMissile; 
                            m.speed = 10.0f; 
                            m.splashRadius = 2.4f;
                            m.targetPos = mouseWorldPos;
                            m.targetPos.x += ((rand() % 100) / 100.0f - 0.5f) * 2.0f;
                            m.targetPos.y += ((rand() % 100) / 100.0f - 0.5f) * 2.0f;
                            bullets.push_back(m);
                        }
                    }
                    // Переключаем триггер режима стрельбы ракетного модуля
                    titan.missileMode = (titan.missileMode == MissileStrikeMode::Ballistic) ? 
                                        MissileStrikeMode::Artillery : MissileStrikeMode::Ballistic;
                    fireCooldown = 1.8f;
                }
            }

            // Нажатие ЛКМ — Стрельба из пушки Танка (Орудие / Автопушка)
            if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
                if (titan.currentWeapon == TankWeaponMode::Cannon) {
                    // Тяжелое танковое орудие
                    Bullet b; b.start = playerPos; b.current = playerPos; b.direction = normDir;
                    b.type = BulletType::Standard; b.speed = 25.0f; bullets.push_back(b);
                    fireCooldown = 0.4f;
                } else {
                    // Скорострельная автопушка (кулдаун зависит от износа приводов турели)
                    Bullet b; b.start = playerPos; b.current = playerPos; b.direction = normDir;
                    b.type = BulletType::Standard; b.speed = 34.0f; bullets.push_back(b);
                    fireCooldown = (titan.systems.turretStatus < 50.0f) ? 0.18f : 0.07f;
                }
            }
        }
    }
}

} // namespace bunker
