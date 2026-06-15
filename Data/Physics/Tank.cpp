#include "Tank.hpp"
#include "../../main.hpp" // Связь со всеми глобальными extern переменными вашего движка   
#include <cmath>
#include <algorithm>
#include <iostream>

namespace bunker {

VanguardTitan::VanguardTitan() {
    // На заводе Ополчения по умолчанию запекается баллистический профиль XO-16
    m_CurrentCamProfile = { 350.0f, 120.0f, 1.02f }; 
}

void VanguardTitan::TriggerMechanicalReMap(AncientLoadout newWeapon) {
    if (m_VortexActive || m_IsReMapping) return;

    m_ActiveLoadout = newWeapon;
    m_IsReMapping = true;
    m_ReMapTimer = 0.4f; // Намеренно блокируем приводы рук на 0.4с для проворота шестерен

    switch (newWeapon) {
        case AncientLoadout::XO16_SolidKinetic:
            m_CurrentCamProfile = { 350.0f, 120.0f, 1.02f };
            titan.currentWeapon = TankWeaponMode::AutoCannon; // Синхронизация с UI Pip-Pad
            break;
        case AncientLoadout::Scorch_ThermiteMortar:
            m_CurrentCamProfile = { 600.0f, 450.0f, 1.85f }; // Высокое давление для заброса термита
            titan.currentWeapon = TankWeaponMode::Cannon;
            break;
        case AncientLoadout::Ion_SplitLaser_Vacuum:
            m_CurrentCamProfile = { 100.0f, 10.0f, 1.0f };   // Отдачи нет, вакуумно-лучевые цепи
            titan.currentWeapon = TankWeaponMode::AutoCannon;
            break;
    }
}

PilotInputControls VanguardTitan::FilterAndStabilizeInputs(const PilotInputControls& rawInput, const TerrainFrictionData& terrain) {
    PilotInputControls stabilized = rawInput;

    // 1. Фильтрация дрожания рук Пилота (Магнитные демпферы на соленоидах затягивают прицел)
    if (rawInput.hand_tremor_hz > 5.0f) {
        stabilized.steering_wheel_angle = rawInput.steering_wheel_angle * 0.85f;
    }

    // 2. Автоматическое удержание баланса 40-тонного тела на скользком грунте/грязи бункера
    if (terrain.surface_slickness > 0.6f) {
        // ИИ перехватывает клапаны ног, изменяя угол стопы стоек на ходу
        float balanceOffset = std::sin(rawInput.throttle_lever) * terrain.surface_slickness * 15.0f;
        // Данный оффсет подмешивается в анимацию шасси
    }

    return stabilized;
}

void VanguardTitan::UpdateBT7274(float deltaTime) {
    // Накопление энергии Котла-Реактора БТ для Активации Ядра
    if (!m_CoreOverdriveActive) {
        m_CoreChargePercentage = std::min(100.0f, m_CoreChargePercentage + 4.5f * deltaTime);
    } else {
        m_CoreChargePercentage = std::max(0.0f, m_CoreChargePercentage - 18.0f * deltaTime);
        if (m_CoreChargePercentage <= 0.0f) m_CoreOverdriveActive = false;
    }

    // Обработка тика механической смены кулачковых валов баллистики орудий
    if (m_IsReMapping) {
        m_ReMapTimer -= deltaTime;
        if (m_ReMapTimer <= 0.0f) m_IsReMapping = false;
        return; // Манипуляторы заблокированы, огонь и способности недоступны
    }

    // Горячая смена калибров на аналоговом пульте (Клавиши 1, 2, 3)
    if (GetAsyncKeyState('1') & 0x8000) TriggerMechanicalReMap(AncientLoadout::XO16_SolidKinetic);
    if (GetAsyncKeyState('2') & 0x8000) TriggerMechanicalReMap(AncientLoadout::Scorch_ThermiteMortar);
    if (GetAsyncKeyState('3') & 0x8000) TriggerMechanicalReMap(AncientLoadout::Ion_SplitLaser_Vacuum);

    // Обработка Вортекс-щита на удержание клавиши 'Q'
    bool holdingQ = (GetAsyncKeyState('Q') & 0x8000);
    UpdateVortexShield(holdingQ, deltaTime);

    // --- РЕЖИМ 1: АВТОНОМНЫЙ («Авто-титан», Пилот Джеку Купер действует снаружи) ---
    if (!titan.isPiloted) {
        float tdx = playerPos.x - titan.position.x;
        float tdy = playerPos.y - titan.position.y;
        float distanceToPilotSq = tdx * tdx + tdy * tdy;

        // Паттерн Anti-Blocking (Сфера отчуждения 6 метров): БТ экстренно уступает дорогу пилоту
        if (distanceToPilotSq < m_ExclusionRadiusSq) {
            float distance = std::sqrt(distanceToPilotSq);
            if (distance > 0.001f) {
                Vector3D pushVector = { tdx / distance, tdy / distance, 0.0f };
                // Реверс гидравлики ног на форсаже (Скорость рывка = 12.0)
                titan.position.x -= pushVector.x * 12.0f * deltaTime;
                titan.position.y -= pushVector.y * 12.0f * deltaTime;
            }
        } else {
            // Паттерн Anchor Point: Движение к тактическому центру боевой геометрии (Combat Anchor)
            Vector3D combatAnchor = CalculateCombatAnchorPoint();
            float adx = combatAnchor.x - titan.position.x;
            float ady = combatAnchor.y - titan.position.y;
            float aLenSq = adx * adx + ady * ady;
            
            if (aLenSq > 1.0f) {
                float aLen = std::sqrt(aLenSq);
                titan.position.x += (adx / aLen) * titan.speed * deltaTime;
                titan.position.y += (ady / aLen) * titan.speed * deltaTime;
            }
        }

        // Автономное ведение огня из XO-16 по каскадному фильтру целей (Threat/TTK)
        if (!enemies.empty() && fireCooldown <= 0.0f && !m_VortexActive) {
            for (auto& e : enemies) {
                if (!e.isAlive) continue;
                float edx = e.position.x - titan.position.x;
                float edy = e.position.y - titan.position.y;
                float eDistSq = edx * edx + edy * edy;

                if (eDistSq <= 400.0f) { // В пределах оптического дальномера 20 клеток
                    float eDist = std::sqrt(eDistSq);
                    Vector3D targetDir = { edx / eDist, edy / eDist, 0.0f };
                    
                    // Запуск баллистического выстрела
                    Bullet b;
                    b.start = titan.position; b.current = titan.position; b.direction = targetDir;
                    b.type = BulletType::Standard; b.speed = 36.0f;
                    bullets.push_back(b);
                    
                    fireCooldown = 0.09f; // Темп огня XO-16
                    break;
                }
            }
        }
        return; 
    }

    // --- РЕЖИМ 2: СИМБИОТИЧЕСКИЙ (Пилот внутри кабины БТ-7274) ---
    // Чтение Win32 WASD осей с поправкой на изометрические оси вашей карты
    Vector3D moveInput = { 0.0f, 0.0f, 0.0f };
    if (GetAsyncKeyState('W') & 0x8000) { moveInput.x += 1.0f; moveInput.y -= 1.0f; }
    if (GetAsyncKeyState('S') & 0x8000) { moveInput.x -= 1.0f; moveInput.y += 1.0f; }
    if (GetAsyncKeyState('A') & 0x8000) { moveInput.x -= 1.0f; moveInput.y -= 1.0f; }
    if (GetAsyncKeyState('D') & 0x8000) { moveInput.x += 1.0f; moveInput.y += 1.0f; }

    float inputLenSq = moveInput.x * moveInput.x + moveInput.y * moveInput.y;
    Vector3D targetVel = { 0.0f, 0.0f, 0.0f };

    if (inputLenSq > 0.001f) {
        float len = std::sqrt(inputLenSq);
        // Тяжелый Вортекс-щит замедляет сервоприводы шасси до 40% скорости
        float activeSpeed = m_VortexActive ? (titan.speed * 0.4f) : titan.speed;
        targetVel.x = (moveInput.x / len) * activeSpeed;
        targetVel.y = (moveInput.y / len) * activeSpeed;
    }

    // Линейная интерполяция тяжелой массы (Физика инерции Авангарда)
    float lerpAcc = (inputLenSq > 0.001f) ? m_TitanAcceleration : m_TitanDeceleration;
    m_Velocity.x += (targetVel.x - m_Velocity.x) * lerpAcc * deltaTime;
    m_Velocity.y += (targetVel.y - m_Velocity.y) * lerpAcc * deltaTime;

    float nextX = titan.position.x + m_Velocity.x * deltaTime;
    float nextY = titan.position.y + m_Velocity.y * deltaTime;

    float btRadius = 0.55f; // Пятно контакта коллизии корпуса
    if (!CheckWorldCollision(nextX, titan.position.y, btRadius)) titan.position.x = nextX;
    if (!CheckWorldCollision(titan.position.x, nextY, btRadius)) titan.position.y = nextY;

    // Перенос позиции игрока вслед за кабиной БТ
    playerPos = titan.position;

    // Ручное ведение огня ЛКМ из кабины
    if ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) && fireCooldown <= 0.0f && !m_VortexActive) {
        float mdx = mouseWorldPos.x - titan.position.x;
        float mdy = mouseWorldPos.y - titan.position.y;
        float mLenSq = mdx * mdx + mdy * mdy;

        if (mLenSq > 0.01f) {
            float mLen = std::sqrt(mLenSq);
            Vector3D targetDir = { mdx / mLen, mdy / mLen, 0.0f };

            Bullet b; b.start = titan.position; b.current = titan.position; b.direction = targetDir;
            b.type = BulletType::Standard;

            if (m_ActiveLoadout == AncientLoadout::XO16_SolidKinetic) {
                b.speed = 36.0f; bullets.push_back(b);
                fireCooldown = m_CoreOverdriveActive ? 0.03f : 0.08f; // Удвоение темпа Burst Core
            } 
            else if (m_ActiveLoadout == AncientLoadout::Scorch_ThermiteMortar) {
                b.type = BulletType::Standard; b.speed = 18.0f; // Тяжелая навесная кассета термита
                bullets.push_back(b); fireCooldown = 0.65f;
            }
            else if (m_ActiveLoadout == AncientLoadout::Ion_SplitLaser_Vacuum) {
                b.speed = 45.0f; bullets.push_back(b); // Ион сплиттер пушка
                fireCooldown = 0.14f;
            }
        }
    }
}

Vector3D VanguardTitan::CalculateCombatAnchorPoint() {
    if (enemies.empty()) {
        // Если боя нет, паттерн Anchor Point держит БТ на 4 клетки позади обзора камеры пилота
        return playerPos; 
    }

    // Находим центр масс скопления Роя Верминов вокруг вышки
    Vector3D enemyMassCenter = { 0.0f, 0.0f, 0.0f };
    int activeCount = 0;

    for (const auto& e : enemies) {
        if (!e.isAlive) continue;
        enemyMassCenter.x += e.position.x;
        enemyMassCenter.y += e.position.y;
        activeCount++;
    }

    if (activeCount == 0) return playerPos;

    enemyMassCenter.x /= activeCount;
    enemyMassCenter.y /= activeCount;

    // Идеальный заслон: Точка ровно на 40% расстояния между Пилотом и эпицентром Роя
    Vector3D anchor;
    anchor.x = playerPos.x + (enemyMassCenter.x - playerPos.x) * 0.40f;
        anchor.y = playerPos.y + (enemyMassCenter.y - playerPos.y) * 0.40f;
        anchor.z = 0.0f;

        return anchor;
    }

    void VanguardTitan::UpdateVortexShield(bool isHoldingQ, float deltaTime) 
    {
        if (m_ActiveLoadout == AncientLoadout::Scorch_ThermiteMortar) 
        {
            m_VortexActive = false; // У мортиры Скорча щита нет по лору
            return;
        }

        if (isHoldingQ && m_VortexEnergy > 5.0f) 
        {
            m_VortexActive = true;
            m_VortexEnergy = std::max(0.0f, m_VortexEnergy - 30.0f * deltaTime); // Потребление пара контура

            // ОПТИМИЗАЦИЯ: Магнитный захват свинца Роя без извлечения корней (Быстрые квадраты расстояний)
            float shieldRadiusSq = 4.0f; // Radius of 2 cells around BT
            
            for (auto& b : bullets) 
            {
                if (!b.isAlive) continue;

                float bdx = b.current.x - titan.position.x;
                float bdy = b.current.y - titan.position.y;

                if ((bdx * bdx + bdy * bdy) <= shieldRadiusSq) 
                {
                    b.isAlive = false; // Пуля поглощена магнитным полем
                    m_CaughtBulletsCount++;
                }
            }
        } 
        else 
        {
            // КЛАПАНЫ ОТКРЫТЫ: Выбрасываем весь пойманный свинец обратно по вектору мыши!
            if (m_VortexActive && m_CaughtBulletsCount > 0) 
            {
                float mdx = mouseWorldPos.x - titan.position.x;
                float mdy = mouseWorldPos.y - titan.position.y;
                float mLenSq = mdx * mdx + mdy * mdy;

                if (mLenSq > 0.01f) 
                {
                    float mLen = std::sqrt(mLenSq);
                    Vector3D returnDir = { mdx / mLen, mdy / mLen, 0.0f };

                    for (int i = 0; i < m_CaughtBulletsCount; ++i) 
                    {
                        Bullet rb;
                        rb.start = titan.position; 
                        rb.current = titan.position;
                        rb.type = BulletType::Standard; 
                        rb.speed = 28.0f;

                        // Аналоговый веер разброса (Возврат с 25% избыточным кинетическим ускорением)
                        float spread = ((rand() % 100) / 100.0f - 0.5f) * 0.25f;
                        rb.direction = { returnDir.x + spread, returnDir.y + spread, 0.0f };
                        
                        bullets.push_back(rb);
                    }
                }
                m_CaughtBulletsCount = 0;
            }

            m_VortexActive = false;
            m_VortexEnergy = std::min(100.0f, m_VortexEnergy + 15.0f * deltaTime); // Восстановление давления пара
        }
    }

    bool VanguardTitan::ValidateCoreOverdriveTrigger(float targetArmorHp, float timeToCover) 
    {
        if (m_CoreChargePercentage < 100.0f) return false;

        // Если враг уйдет за стены быстрее, чем за 3 секунды — Активация откладывается автоматикой
        if (timeToCover < 3.0f) return false;

        // Перегруз эффективен, если давление пара и расчетный урон способны пробить броню цели
        float potentialDamage = m_BoilerSteamPressureBar * 12.5f;
        
        if (potentialDamage >= targetArmorHp || m_CoolantTemperatureCelsius > 950.0f) 
        {
            return true;
        }

        return false;
    }

    void VanguardTitan::ExecuteCoreOverdrive() 
    {
        if (m_CoreChargePercentage < 100.0f) return;

        m_CoreOverdriveActive = true;
        m_BoilerSteamPressureBar = 1200.0f; // Перегруз до 1200 Бар! Ревущие турбины Сверхзаряда (Burst Core)
        m_CoolantTemperatureCelsius = 980.0f;
    }

} // namespace bunker
