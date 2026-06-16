#include "Tactics.hpp"
#include "../../main.hpp" // Допуск к глобальным playerHealth, bunkerProgression, CheckWorldCollision
#include <cmath>
#include <algorithm>

namespace bunker
{

    void Vault17ClassManager::ChangePilotClass(PilotClass newClass)
    {
        if (!isInsideVehicle)
        {
            activePilotClass = newClass;
            isTacticalActive = false;
            tacticalActiveTimer = 0.0f;
            grapple.isAttached = false;
            aWallShield.isDeployed = false;
            isPulseBladeActive = false;
            isPhaseDimensionActive = false;
            UpdateActiveStats();
        }
    }

    void Vault17ClassManager::ChangeTitanFirmware(TitanClass newClass)
    {
        activeTitanClass = newClass;
        if (isInsideVehicle)
            UpdateActiveStats();
    }

    void Vault17ClassManager::EnterVehicle()
    {
        isInsideVehicle = true;
        isTacticalActive = false;
        grapple.isAttached = false;
        aWallShield.isDeployed = false;
        isPhaseDimensionActive = false;
        UpdateActiveStats();
    }

    void Vault17ClassManager::ExitVehicle()
    {
        isInsideVehicle = false;
        UpdateActiveStats();
    }

    PilotClass Vault17ClassManager::GetActivePilotClass() const { return activePilotClass; }
    TitanClass Vault17ClassManager::GetActiveTitanClass() const { return activeTitanClass; }

    void Vault17ClassManager::UpdateCooldowns(float deltaTime)
    {
        // Однократная инициализация модификаторов характеристик при самом первом вызове
        static bool firstFrame = true;
        if (firstFrame)
        {
            UpdateActiveStats(); // Загружаем базовые тактические модификаторы v15
            firstFrame = false;
        } // Скобка закрывается ЗДЕСЬ! Теперь инициализация не блокирует остальной код кадра.

        // 1. Ежекадровое уменьшение таймера перезарядки способности
        if (tacticalCooldown > 0.0f)
        {
            tacticalCooldown -= deltaTime;
        }

        // 2. Обработка времени действия активной тактической способности
        if (isTacticalActive)
        {
            tacticalActiveTimer -= deltaTime;
            if (tacticalActiveTimer <= 0.0f)
            {
                isTacticalActive = false;
                aWallShield.isDeployed = false;
                isPulseBladeActive = false;
                isPhaseDimensionActive = false; // Возвращаемся из Изнанки (Фазовый сдвиг)
                UpdateActiveStats();            // Сбрасываем баффы скорости/защиты экзокостюма
            }
        }

        // 3. Регенерация здоровья инъекцией тактического Стима (Канон Titanfall 2)
        if (isTacticalActive && activePilotClass == PilotClass::Stim && !isInsideVehicle)
        {
            // Обернули std::min в скобки (std::min), чтобы макросы windows.h не ломали синтаксис компилятора
            playerHealth = (std::min)(playerMaxHealth, playerHealth + 15.0f * deltaTime);
        }
    }

    void Vault17ClassManager::ActivateTacticalSkill(const Vector3D &mousePos, const Vector3D &pilotPos)
    {
        // Блокировка способностей до нахождения Pip-Pad в сессии Убежища 17
        if (!bunkerProgression.hasFoundPipPad || isInsideVehicle || tacticalCooldown > 0.0f || isTacticalActive)
            return;

        isTacticalActive = true;

        switch (activePilotClass)
        {
        case PilotClass::Grapple:
            grapple.isAttached = true;
            grapple.hookPoint = mousePos;
            {
                float dx = grapple.hookPoint.x - pilotPos.x;
                float dy = grapple.hookPoint.y - pilotPos.y;

                // ОПТИМИЗАЦИЯ: Сначала проверяем квадрат макс. длины лебедки (12 клеток = 144.0f)
                float distSq = dx * dx + dy * dy;
                if (distSq > 144.0f)
                {
                    grapple.isAttached = false;
                    isTacticalActive = false;
                    return;
                }
                grapple.length = std::sqrt(distSq); // Извлекаем корень только при успешном зацепе
            }
            tacticalActiveTimer = 2.0f;
            tacticalCooldown = 9.0f;
            break;

        case PilotClass::Stim:
            tacticalActiveTimer = 3.0f;
            tacticalCooldown = 14.0f;
            break;

        case PilotClass::PhaseShift:
            isPhaseDimensionActive = true;
            tacticalActiveTimer = 2.5f;
            tacticalCooldown = 18.0f;
            break;

        case PilotClass::Cloak:
            tacticalActiveTimer = 5.5f;
            tacticalCooldown = 16.0f;
            break;

        case PilotClass::AWall:
            aWallShield.isDeployed = true;
            aWallShield.position = pilotPos;
            aWallShield.health = 200.0f;
            tacticalActiveTimer = 5.0f;
            tacticalCooldown = 15.0f;
            break;

        default:
            isTacticalActive = false;
            break;
        }
        UpdateActiveStats();
    }

    void Vault17ClassManager::ProcessGrapplePhysics(Vector3D &pilotPos, float deltaTime)
    {
        if (!grapple.isAttached || activePilotClass != PilotClass::Grapple || isInsideVehicle)
            return;

        float tdx = grapple.hookPoint.x - pilotPos.x;
        float tdy = grapple.hookPoint.y - pilotPos.y;
        float currentDist = std::sqrt(tdx * tdx + tdy * tdy);

        if (currentDist > 0.4f && tacticalActiveTimer > 0.05f)
        {
            float pullForce = 8.0f;
            grapple.velocity.x = (tdx / currentDist) * pullForce;
            grapple.velocity.y = (tdy / currentDist) * pullForce;

            // Математическая парабола высоты полета Пилота на тросе по оси Z
            float progress = 1.0f - (currentDist / grapple.length);
            pilotPos.z = std::sin(progress * 3.1415926f) * 1.8f;

            // ИСПРАВЛЕНИЕ: Интеграция ААВВ-клиппинга стен. Пилот больше не пролетает сквозь геометрию бункера!
            float nextX = pilotPos.x + grapple.velocity.x * deltaTime;
            float nextY = pilotPos.y + grapple.velocity.y * deltaTime;
            float pRadius = 0.25f;

            if (!CheckWorldCollision(nextX, pilotPos.y, pRadius))
                pilotPos.x = nextX;
            if (!CheckWorldCollision(pilotPos.x, nextY, pRadius))
                pilotPos.y = nextY;
        }
        else
        {
            grapple.isAttached = false;
            isTacticalActive = false;
            pilotPos.z = 0.0f;
        }
    }

    void Vault17ClassManager::UpdateActiveStats()
    {
        if (!isInsideVehicle)
        {
            currentStats.isVehicleMode = false;
            currentStats.damageMultiplier = 1.0f;
            currentStats.erosionResistance = 0.0f;

            switch (activePilotClass)
            {
            case PilotClass::Grapple:
                currentStats.maxHealth = 100.0f;
                currentStats.moveSpeed = 5.8f;
                currentStats.weaponLabel = L"PILOT: GRAPPLE GEAR";
                break;
            case PilotClass::Stim:
                currentStats.maxHealth = 100.0f;
                currentStats.moveSpeed = isTacticalActive ? 6.8f : 5.5f;
                currentStats.weaponLabel = L"STIM CARBINE";
                break;
            case PilotClass::PhaseShift:
                currentStats.maxHealth = 100.0f;
                currentStats.moveSpeed = 5.5f;
                currentStats.erosionResistance = isTacticalActive ? 1.0f : 0.0f;
                currentStats.weaponLabel = isPhaseDimensionActive ? L"PHASE DIMENSION" : L"PHASE RIFLE";
                break;
            default:
                currentStats.maxHealth = 100.0f;
                currentStats.moveSpeed = 5.5f;
                currentStats.weaponLabel = L"STANDARD CARBINE";
                break;
            }
        }
    }

    std::wstring Vault17ClassManager::GetActiveClassNameW() const
    {
        return currentStats.weaponLabel;
    }

} // namespace bunker
