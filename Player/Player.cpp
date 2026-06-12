#include "main.hpp"
#include "Player.hpp"
#include <cmath>

bool CheckWorldCollision(float nextX, float nextY, float radius) {
    int checkPoints[4][2] = {
        { (int)(nextX - radius), (int)(nextY - radius) },
        { (int)(nextX + radius), (int)(nextY - radius) },
        { (int)(nextX - radius), (int)(nextY + radius) },
        { (int)(nextX + radius), (int)(nextY + radius) }
    };
    for (int i = 0; i < 4; ++i) {
        int tx = checkPoints[i][0]; int ty = checkPoints[i][1];
        if (tx < 0 || tx >= MAP_WIDTH || ty < 0 || ty >= MAP_HEIGHT) return true;
        if (currentSectorMap[tx][ty] == 1) return true;
    }
    return false;
}

void HandleInput(HWND hwnd, float deltaTime, float wWidth, float wHeight) {
    Vector3D moveDir = { 0.0f, 0.0f, 0.0f };
    if (GetAsyncKeyState('W') & 0x8000) { moveDir.x += 1.0f; moveDir.y -= 1.0f; }
    if (GetAsyncKeyState('S') & 0x8000) { moveDir.x -= 1.0f; moveDir.y += 1.0f; }
    if (GetAsyncKeyState('A') & 0x8000) { moveDir.x -= 1.0f; moveDir.y -= 1.0f; }
    if (GetAsyncKeyState('D') & 0x8000) { moveDir.x += 1.0f; moveDir.y += 1.0f; }

    isAiming = (GetAsyncKeyState(VK_RBUTTON) & 0x8000);

    if (playerMode == UnitMode::Scout || !isAiming) {
        isoCamera.zoom += (55.0f - isoCamera.zoom) * 5.0f * deltaTime;
    }

    float len = std::sqrt(moveDir.x * moveDir.x + moveDir.y * moveDir.y);
    if (len > 0.0f) {
        float currentMoveSpeed = playerSpeed;
        if (playerMode == UnitMode::Scout && isAiming) currentMoveSpeed *= 0.63f;
        if (playerMode == UnitMode::Titan && titan.systems.tracksCondition < 40.0f) currentMoveSpeed *= 0.3f;

        float nextX = playerPos.x + (moveDir.x / len) * currentMoveSpeed * deltaTime;
        float nextY = playerPos.y + (moveDir.y / len) * currentMoveSpeed * deltaTime;
        float playerRadius = (playerMode == UnitMode::Titan) ? 0.55f : 0.25f;

        if (!CheckWorldCollision(nextX, playerPos.y, playerRadius)) playerPos.x = nextX;
        if (!CheckWorldCollision(playerPos.x, nextY, playerRadius)) playerPos.y = nextY;
    }

    POINT mp; GetCursorPos(&mp); ScreenToClient(hwnd, &mp);
    currentMouseScreenPos = { (float)mp.x, (float)mp.y };
    mouseWorldPos = isoCamera.ScreenToWorldGround(currentMouseScreenPos, cameraTarget);

    if (fireCooldown > 0.0f) fireCooldown -= deltaTime;

    float dx = mouseWorldPos.x - playerPos.x; float dy = mouseWorldPos.y - playerPos.y;
    float dLen = std::sqrt(dx * dx + dy * dy);

    if (dLen > 0.05f && fireCooldown <= 0.0f) {
        Vector3D normDir = { dx / dLen, dy / dLen, 0.0f };

        if (playerMode == UnitMode::Scout) {
            if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
                if (isAiming) {
                    float pelletSpread = 0.04f;
                    for (int i = 0; i < 5; ++i) {
                        Bullet p; p.start = playerPos; p.current = playerPos; p.type = BulletType::Pellet;
                        float randomSpread = ((rand() % 100) / 100.0f - 0.5f) * pelletSpread;
                        p.direction = { normDir.x + randomSpread, normDir.y + randomSpread, 0.0f };
                        bullets.push_back(p);
                    }
                    fireCooldown = 0.55f;
                } else {
                    Bullet b; b.start = playerPos; b.current = playerPos; b.direction = normDir;
                    b.type = BulletType::Standard; bullets.push_back(b);
                    fireCooldown = 0.22f;
                }
            }
        }
        else if (playerMode == UnitMode::Titan) {
            if (titan.systems.turretStatus < 50.0f) {
                float brokenFactor = ((rand() % 100) / 100.0f - 0.5f) * 0.25f;
                normDir.x += brokenFactor; normDir.y += brokenFactor;
            }

            if (GetAsyncKeyState(VK_MBUTTON) & 0x8000) {
                if (titan.hasMissileModule) {
                    if (titan.missileMode == MissileStrikeMode::Ballistic) {
                        for (int i = -3; i <= 3; ++i) {
                            Bullet m; m.start = playerPos; m.current = playerPos;
                            m.type = BulletType::BallisticMissile; m.speed = 15.0f; m.splashRadius = 1.8f;
                            Vector3D sideVector = { -normDir.y, normDir.x, 0.0f };
                            m.start.x += sideVector.x * (i * 0.4f); m.start.y += sideVector.y * (i * 0.4f);
                            m.direction = normDir; bullets.push_back(m);
                        }
                    } else {
                        for (int i = 0; i < 8; ++i) {
                            Bullet m; m.start = playerPos; m.current = playerPos;
                            m.type = BulletType::ArtilleryMissile; m.speed = 10.0f; m.splashRadius = 2.4f;
                            m.targetPos = mouseWorldPos;
                            m.targetPos.x += ((rand() % 100) / 100.0f - 0.5f) * 2.0f;
                            m.targetPos.y += ((rand() % 100) / 100.0f - 0.5f) * 2.0f;
                            bullets.push_back(m);
                        }
                    }
                    titan.missileMode = (titan.missileMode == MissileStrikeMode::Ballistic) ? MissileStrikeMode::Artillery : MissileStrikeMode::Ballistic;
                    fireCooldown = 1.8f;
                }
            }

            if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
                if (titan.currentWeapon == TankWeaponMode::Cannon) {
                    Bullet b; b.start = playerPos; b.current = playerPos; b.direction = normDir;
                    b.type = BulletType::Standard; b.speed = 25.0f; bullets.push_back(b);
                    fireCooldown = 0.4f;
                } else {
                    Bullet b; b.start = playerPos; b.current = playerPos; b.direction = normDir;
                    b.type = BulletType::Standard; b.speed = 34.0f; bullets.push_back(b);
                    fireCooldown = (titan.systems.turretStatus < 50.0f) ? 0.18f : 0.07f;
                }
            }
        }
    }
}
