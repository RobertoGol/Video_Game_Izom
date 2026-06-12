#include "../../main.hpp"
#include "Tank.hpp"
#include "Collisions.hpp"
#include <cmath>

void UpdateAutonomousTankAI(float deltaTime) {
    if (titan.isPiloted) {
        titan.position = playerPos; // Если пилотируется — позиция намертво завязана на игрока
        return;
    }
    
    float tdx_t = playerPos.x - titan.position.x;
    float tdy_t = playerPos.y - titan.position.y;
    float distToPlayer = std::sqrt(tdx_t * tdx_t + tdy_t * tdy_t);

    // Поиск ближайшего Вермина для автопушки Танка
    Enemy* closestTarget = nullptr; 
    float closestDist = 8.0f;
    for (auto& e : enemies) {
        if (!e.isAlive) continue;
        float edx = e.position.x - titan.position.x; 
        float edy = e.position.y - titan.position.y;
        float eDist = std::sqrt(edx * edx + edy * edy);
        if (eDist < closestDist) { 
            closestDist = eDist; 
            closestTarget = &e; 
        }
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
            bullets.push_back(tb); 
            titan.fireCooldown = 0.12f; // Высокая скорострельность автомата
        }
    }
    if (titan.fireCooldown > 0.0f) titan.fireCooldown -= deltaTime;

    // Следование за пилотом с учётом габаритов шасси Танка и стен
    if (distToPlayer > 2.2f) {
        float nextX = titan.position.x + (tdx_t / distToPlayer) * titan.speed * deltaTime;
        float nextY = titan.position.y + (tdy_t / distToPlayer) * titan.speed * deltaTime;
        if (!CheckWorldCollision(nextX, titan.position.y, 0.55f)) titan.position.x = nextX;
        if (!CheckWorldCollision(titan.position.x, nextY, 0.55f)) titan.position.y = nextY;
    }
}
