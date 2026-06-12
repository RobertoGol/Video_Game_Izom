#include "main.hpp"
#include "Mechanics.hpp"
#include "Player.hpp"
#include <cmath>
#include <algorithm>

void ProcessGameMechanics(float deltaTime) {
    // 1. СИМУЛЯЦИЯ БАЛЛИСТИКИ И АРТИЛЛЕРИИ ПУЛЬ
    for (auto& b : bullets) {
        float moveStep = b.speed * deltaTime;

        if (b.type == BulletType::ArtilleryMissile) {
            float bdx = b.targetPos.x - b.current.x; float bdy = b.targetPos.y - b.current.y;
            float distToTarget = std::sqrt(bdx * bdx + bdy * bdy);
            if (distToTarget > 0.2f) {
                b.current.x += (bdx / distToTarget) * moveStep; b.current.y += (bdy / distToTarget) * moveStep;
                b.distanceTraveled += moveStep;
                float progress = b.distanceTraveled / 12.0f;
                if (progress > 1.0f) progress = 1.0f;
                b.current.z = std::sin(progress * 3.1415926f) * 4.0f;
            } else { b.isAlive = false; }
        } else {
            b.current.x += b.direction.x * moveStep; b.current.y += b.direction.y * moveStep;
            b.distanceTraveled += moveStep;
        }

        if (b.distanceTraveled >= b.maxDistance) b.isAlive = false;

        // Разрушаемость стен завалов снарядами
        int mX = (int)b.current.x; int mY = (int)b.current.y;
        if (mX >= 0 && mX < MAP_WIDTH && mY >= 0 && mY < MAP_HEIGHT) {
            if (currentSectorMap[mX][mY] == 1) {
                b.isAlive = false;
                int damageToWall = (b.type == BulletType::BallisticMissile || b.type == BulletType::ArtilleryMissile) ? 50 : 8;
                wallDurability[mX][mY] -= damageToWall;
                if (wallDurability[mX][mY] <= 0) { currentSectorMap[mX][mY] = 0; score += 50; }
                continue;
            }
        }

        // Попадания во врагов
        for (auto& e : enemies) {
            if (!e.isAlive) continue;
            float edx = e.position.x - b.current.x; float edy = e.position.y - b.current.y;
            if (std::sqrt(edx * edx + edy * edy) <= e.radius) {
                b.isAlive = false;
                if (b.type == BulletType::BallisticMissile || b.type == BulletType::ArtilleryMissile) {
                    for (auto& splashEnemy : enemies) {
                        if (!splashEnemy.isAlive) continue;
                        float sdx = splashEnemy.position.x - b.current.x; float sdy = splashEnemy.position.y - b.current.y;
                        if (std::sqrt(sdx * sdx + sdy * sdy) <= b.splashRadius) {
                            splashEnemy.health -= 85.0f;
                            if (splashEnemy.health <= 0.0f) { splashEnemy.isAlive = false; score += 150; }
                        }
                    }
                } else {
                    e.health -= (b.type == BulletType::Pellet) ? 14.0f : 20.0f;
                    if (e.health <= 0.0f) { e.isAlive = false; score += 150; }
                }
                break;
            }
        }
    }

    // 2. ПОТОК ЛОГИКИ АВТОНОМНОГО СОЮЗНИКА ТАНКА С ИИ
    if (!titan.isPiloted) {
        float tdx_t = playerPos.x - titan.position.x; float tdy_t = playerPos.y - titan.position.y;
        float distToPlayer = std::sqrt(tdx_t * tdx_t + tdy_t * tdy_t);

        Enemy* closestTarget = nullptr; float closestDist = 8.0f;
        for (auto& e : enemies) {
            if (!e.isAlive) continue;
            float edx = e.position.x - titan.position.x; float edy = e.position.y - titan.position.y;
            float eDist = std::sqrt(edx * edx + edy * edy);
            if (eDist < closestDist) { closestDist = eDist; closestTarget = &e; }
        }

        if (closestTarget != nullptr && titan.fireCooldown <= 0.0f) {
            Bullet tb; tb.start = titan.position; tb.current = titan.position;
            float bdx_t = closestTarget->position.x - titan.position.x; float bdy_t = closestTarget->position.y - titan.position.y;
            float bLen = std::sqrt(bdx_t * bdx_t + bdy_t * bdy_t);
            if (bLen > 0.1f) {
                tb.direction = { bdx_t / bLen, bdy_t / bLen, 0.0f };
                bullets.push_back(tb); titan.fireCooldown = 0.12f;
            }
        }
        if (titan.fireCooldown > 0.0f) titan.fireCooldown -= deltaTime;

        if (distToPlayer > 2.2f) {
            float nextX = titan.position.x + (tdx_t / distToPlayer) * titan.speed * deltaTime;
            float nextY = titan.position.y + (tdy_t / distToPlayer) * titan.speed * deltaTime;
            if (!CheckWorldCollision(nextX, titan.position.y, 0.55f)) titan.position.x = nextX;
            if (!CheckWorldCollision(titan.position.x, nextY, 0.55f)) titan.position.y = nextY;
        }
    } else {
        titan.position = playerPos;
    }

    // 3. РАСЧЕТ НАКОПЛЕНИЯ ЭФИРНОЙ ЭРОЗИИ (ETHER EROSION) СКАУТА
    int currentTileX = (int)playerPos.x; int currentTileY = (int)playerPos.y;
    if (currentTileX >= 0 && currentTileX < MAP_WIDTH && currentTileY >= 0 && currentTileY < MAP_HEIGHT) {
        float currentZoneErosion = etherErosionMap[currentTileX][currentTileY];
        if (titan.isPiloted) {
            playerErosionLevel -= 12.0f * deltaTime;
        } else {
            playerErosionLevel += (currentZoneErosion > 0.0f) ? (currentZoneErosion * 18.0f * deltaTime) : (-4.0f * deltaTime);
        }
        if (playerErosionLevel > 100.0f) playerErosionLevel = 100.0f;
        if (playerErosionLevel < 0.0f) playerErosionLevel = 0.0f;

        if (playerErosionLevel > EROSION_DAMAGE_THRESHOLD && !titan.isPiloted) {
            playerHealth -= 10.0f * deltaTime;
            if (playerHealth < 0.0f) playerHealth = 0.0f;
        }
    }
}
