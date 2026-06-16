#include "Mechanics.hpp"
#include "../../main.hpp" // Прямой линк ко всем extern массивам движка
#include "Collisions.hpp" // Интеграция нашей новой системы масс
#include <cmath>
#include <algorithm>

namespace bunker
{

    void BallisticsEngine::UpdateProjectiles(float deltaTime, std::vector<Vertex> &vBuffer)
    {
        if (bullets.empty())
            return;

        // Проходим по всему массиву активного свинца на карте
        for (size_t i = 0; i < bullets.size();)
        {
            Bullet &b = bullets[i];

            // Сдвигаем пулю по вектору направления без тригонометрических функций
            b.current.x += b.direction.x * b.speed * deltaTime;
            b.current.y += b.direction.y * b.speed * deltaTime;

            // Расчет пройденной дистанции на квадратах дельт
            float tdx = b.current.x - b.start.x;
            float tdy = b.current.y - b.start.y;

            if ((tdx * tdx + tdy * tdy) >= (b.maxDistance * b.maxDistance))
            {
                b.isAlive = false;
            }

            // Защита от вылета пули за границы ячеек .map
            int tx = static_cast<int>(b.current.x);
            int ty = static_cast<int>(b.current.y);

            if (tx < 0 || tx >= MAP_WIDTH || ty < 0 || ty >= MAP_HEIGHT || currentSectorMap[tx][ty] == 1)
            {
                b.isAlive = false;

                // Если стена разрушаемая, уменьшаем ее прочность
                if (tx >= 0 && tx < MAP_WIDTH && ty >= 0 && ty < MAP_HEIGHT)
                {
                    if (wallDurability[tx][ty] > 0)
                    {
                        wallDurability[tx][ty] -= (b.type == BulletType::BallisticMissile) ? 45.0f : 10.0f;
                        if (wallDurability[tx][ty] <= 0)
                            currentSectorMap[tx][ty] = 0; // Стена пробита!
                    }
                }
            }

            // ОБСЧЕТ ПОПАДАНИЯ ВО ВРАГОВ (Квадратичный фильтр пересечений)
            if (b.isAlive)
            {
                for (auto &e : enemies)
                {
                    if (!e.isAlive)
                        continue;

                    float edx = e.position.x - b.current.x;
                    float edy = e.position.y - b.current.y;
                    float distSq = edx * edx + edy * edy;
                    float hitRadiusSq = e.radius * e.radius;

                    if (distSq <= hitRadiusSq)
                    {
                        b.isAlive = false;

                        // --- КАНOНИЧНЫЙ ТРИГГЕР ЦЕПНЫХ МOЛНИЙ DEBUGGUN (ID: 888) ---
                        if (GetAsyncKeyState(VK_F10) & 0x8000)
                        {
                            Vector3D lastLightningSource = e.position;
                            e.health = 0.0f;
                            e.isAlive = false;
                            score += 150;

                            int chainCount = 0;
                            float chainRadiusSq = 16.0f; // Радиус прыжка дуги — 4 клетки (16 в квадрате)

                            for (auto &nextTarget : enemies)
                            {
                                if (!nextTarget.isAlive)
                                    continue;
                                if (chainCount >= 5)
                                    break;

                                float ldx = nextTarget.position.x - lastLightningSource.x;
                                float ldy = nextTarget.position.y - lastLightningSource.y;

                                if ((ldx * ldx + ldy * ldy) <= chainRadiusSq)
                                {
                                    nextTarget.health = 0.0f;
                                    nextTarget.isAlive = false;
                                    score += 150;
                                    chainCount++;

                                    // Проекция 3D мировых точек молнии на экран
                                    ScreenPoint pStart = isoCamera.WorldToScreen(lastLightningSource, cameraTarget);
                                    ScreenPoint pEnd = isoCamera.WorldToScreen(nextTarget.position, cameraTarget);

                                    // ИСПРАВЛЕНО: Убрали лишние параметры разрешения экрана, оставив строго 2 аргумента
                                    ScreenPoint ndcStart = PixelsToNDC(pStart.x, pStart.y);
                                    ScreenPoint ndcEnd = PixelsToNDC(pEnd.x, pEnd.y);

                                    // Запись ярко-голубых неоновых полигонов разряда в vBuffer
                                    vBuffer.push_back({ndcStart.x, ndcStart.y, 0.0f, 0.3f, 0.7f, 1.0f, 1.0f});
                                    vBuffer.push_back({ndcEnd.x, ndcEnd.y, 0.0f, 0.3f, 0.7f, 1.0f, 1.0f});
                                    vBuffer.push_back({ndcStart.x + 0.004f, ndcStart.y, 0.0f, 0.3f, 0.7f, 1.0f, 1.0f});

                                    lastLightningSource = nextTarget.position; // Ток перескакивает дальше
                                }
                            }
                        }
                        // --- ЛОГИКА ТЯЖЕЛОГО СПЛЕШ-ВЗРЫВА РАКЕТ КЛАССА АВАНГАРД ---
                        else if (b.type == BulletType::BallisticMissile || b.type == BulletType::ArtilleryMissile)
                        {
                            float splashRadiusSq = b.splashRadius * b.splashRadius;

                            for (auto &splashEnemy : enemies)
                            {
                                if (!splashEnemy.isAlive)
                                    continue;

                                float sdx = splashEnemy.position.x - b.current.x;
                                float sdy = splashEnemy.position.y - b.current.y;

                                // Наносим урон по площади без извлечения корней
                                if ((sdx * sdx + sdy * sdy) <= splashRadiusSq)
                                {
                                    splashEnemy.health -= 85.0f;
                                    if (splashEnemy.health <= 0.0f)
                                    {
                                        splashEnemy.isAlive = false;
                                        score += 150;
                                    }
                                }
                            }
                        }
                        else
                        {
                            // Точечный урон (Дробь Pellet или карабин Standard)
                            e.health -= (b.type == BulletType::Pellet) ? 14.0f : 25.0f;
                            if (e.health <= 0.0f)
                            {
                                e.isAlive = false;
                                score += 150;
                            }
                        }
                        break; // Пуля уничтожена, выходим к следующему снаряду
                    }
                }
            }

            // Стандартный быстрый Swap-and-Pop для очистки вектора без дефрагментации памяти
            if (!b.isAlive)
            {
                if (i != bullets.size() - 1)
                {
                    bullets[i] = std::move(bullets.back());
                }
                bullets.pop_back();
            }
            else
            {
                ++i;
            }
        }

        // СИСТЕМА ВЗАИМНЫХ КОЛЛИЗИЙ СУЩЕСТВ (Выталкивание тел после баллистики)
        if (enemies.size() > 1)
        {
            for (size_t i = 0; i < enemies.size(); ++i)
            {
                if (!enemies[i].isAlive)
                    continue;

                // 1. Столкновение Монстров между собой, чтобы они не слипались в одну точку
                for (size_t j = i + 1; j < enemies.size(); ++j)
                {
                    if (!enemies[j].isAlive)
                        continue;

                    CollisionBody bodyA = {&enemies[i].position, enemies[i].radius, PhysicalMassClass::VerminSwarm};
                    CollisionBody bodyB = {&enemies[j].position, enemies[j].radius, PhysicalMassClass::VerminSwarm};
                    Collisions::ResolveCircleVsCircle(bodyA, bodyB);
                }

                // 2. Столкновение Монстра с Игроком/Титаном (с учетом колоссальной массы БТ-7274)
                CollisionBody enemyBody = {&enemies[i].position, enemies[i].radius, PhysicalMassClass::VerminSwarm};
                PhysicalMassClass playerMass = (playerMode == UnitMode::Titan) ? PhysicalMassClass::VanguardTitan : PhysicalMassClass::HumanPilot;
                float pRadius = (playerMode == UnitMode::Titan) ? 0.55f : 0.25f;
                CollisionBody playerBody = {&playerPos, pRadius, playerMass};

                // Если сталкиваются с БТ — Вермины отлетают, не замедляя ход шасси Авангарда
                if (Collisions::ResolveCircleVsCircle(playerBody, enemyBody))
                {
                    if (playerMode == UnitMode::Scout && !bunkerProgression.hasFoundPipPad)
                    {
                        playerHealth -= 2.0f; // Укус Вермина ранит пехотинца
                    }
                }

                // 3. Выталкивание Монстров из стен карты
                int ex = static_cast<int>(enemies[i].position.x);
                int ey = static_cast<int>(enemies[i].position.y);

                for (int nx = ex - 1; nx <= ex + 1; ++nx)
                {
                    for (int ny = ey - 1; ny <= ey + 1; ++ny)
                    {
                        if (nx >= 0 && nx < MAP_WIDTH && ny >= 0 && ny < MAP_HEIGHT)
                        {
                            if (currentSectorMap[nx][ny] == 1)
                            {
                                Collisions::ResolveCircleVsTileWall(enemies[i].position, enemies[i].radius, nx, ny);
                            }
                        }
                    }
                }
            }
        }
    }

    void BallisticsEngine::ProcessSwarmSpawning(float deltaTime)
    {
        enemySpawnTimer += deltaTime;

        // Рассредоточенный спавн: рой лезет из вентиляций каждые 3.5 секунды, если лимит < 60 особей
        if (enemySpawnTimer >= 3.5f)
        {
            enemySpawnTimer = 0.0f;

            if (enemies.size() < 60)
            {
                // Точки спавна по углам карты Убежища 17
                Vector3D spawnPoints[4] = {
                    {1.0f, 1.0f, 0.0f}, {18.0f, 1.0f, 0.0f}, {1.0f, 18.0f, 0.0f}, {18.0f, 18.0f, 0.0f}};

                for (int i = 0; i < 4; ++i)
                {
                    Enemy newVermin;
                    newVermin.position = spawnPoints[i];
                    newVermin.health = 35.0f;
                    newVermin.speed = 2.2f + static_cast<float>(rand() % 100) / 500.0f; // Разброс скоростей особей
                    newVermin.isAlive = true;
                    newVermin.radius = 0.32f;
                    enemies.push_back(newVermin);
                }
            }
        }

        // ИИ Монстров: Плавное накопление векторов шага в сторону вышки связи или игрока
        for (auto &e : enemies)
        {
            if (!e.isAlive)
            {
                continue;
            }

            // По умолчанию Рой штурмует вышку связи Убежища 17, но если игрок подошел близко — агрится на него
            float dx = playerPos.x - e.position.x;
            float dy = playerPos.y - e.position.y;
            float distToPlayerSq = dx * dx + dy * dy;

            float targetX = towerPosition.x;
            float targetY = towerPosition.y;

            if (distToPlayerSq < 49.0f)
            {
                // Радиус агрессии 7 клеток (49 в квадрате)
                targetX = playerPos.x;
                targetY = playerPos.y;
            }

            float tdx_target = targetX - e.position.x;
            float tdy_target = targetY - e.position.y;
            float len = std::sqrt(tdx_target * tdx_target + tdy_target * tdy_target);

            if (len > 0.05f)
            {
                e.position.x += (tdx_target / len) * e.speed * deltaTime;
                e.position.y += (tdy_target / len) * e.speed * deltaTime;
            }
        }
    }

} // namespace bunker
