#include "../../main.hpp"
#include "Tactics.hpp"
#include <cmath>

Vault17ClassManager::Vault17ClassManager() {
    activePilotClass = PilotClass::Grapple;
    activeTitanClass = TitanClass::Tone;
    isInsideVehicle = false;
    tacticalCooldown = 0.0f;
    tacticalActiveTimer = 0.0f;
    isTacticalActive = false;
    
    // Обнуление физики
    grapple.isAttached = false;
    grapple.length = 0.0f;
    grapple.velocity = {0.0f, 0.0f, 0.0f};
    
    aWallShield.isDeployed = false;
    isPulseBladeActive = false;
    pulseBladeRadius = 0.0f;
    
    UpdateActiveStats();
}

void Vault17ClassManager::ChangePilotClass(PilotClass newClass) {
    if (!isInsideVehicle) {
        activePilotClass = newClass;
        isTacticalActive = false;
        tacticalActiveTimer = 0.0f;
        grapple.isAttached = false;
        aWallShield.isDeployed = false;
        isPulseBladeActive = false;
        UpdateActiveStats();
    }
}

void Vault17ClassManager::ChangeTitanFirmware(TitanClass newClass) {
    activeTitanClass = newClass;
    if (isInsideVehicle) UpdateActiveStats();
}

void Vault17ClassManager::EnterVehicle() {
    isInsideVehicle = true;
    isTacticalActive = false;
    grapple.isAttached = false;
    aWallShield.isDeployed = false;
    isPulseBladeActive = false;
    UpdateActiveStats();
}

void Vault17ClassManager::ExitVehicle() {
    isInsideVehicle = false;
    UpdateActiveStats();
}

PilotClass Vault17ClassManager::GetActivePilotClass() const { return activePilotClass; }
TitanClass Vault17ClassManager::GetActiveTitanClass() const { return activeTitanClass; }

void Vault17ClassManager::UpdateCooldowns(float deltaTime) {
    if (tacticalCooldown > 0.0f) tacticalCooldown -= deltaTime;
    
    // Обновление таймеров активного состояния тактик
    if (isTacticalActive) {
        tacticalActiveTimer -= deltaTime;
        if (tacticalActiveTimer <= 0.0f) {
            isTacticalActive = false;
            aWallShield.isDeployed = false;
            isPulseBladeActive = false;
            UpdateActiveStats();
        }
    }
    
    // Регенерация ХП от тактики Стимулятора (Stim) прямо во время действия кадра
    if (isTacticalActive && activePilotClass == PilotClass::Stim && !isInsideVehicle) {
        playerHealth = std::min(playerMaxHealth, playerHealth + 25.0f * deltaTime);
    }
}

// Ультимативный процессор активации способностей на кнопку Q
void Vault17ClassManager::ActivateTacticalSkill(const Vector3D& mousePos, const Vector3D& pilotPos) {
    if (isInsideVehicle || tacticalCooldown > 0.0f || isTacticalActive) return;
    
    isTacticalActive = true;
    
    switch (activePilotClass) {
        case PilotClass::Grapple:
            // Расчет зацепа Крюка-кошки за изометрическую точку земли
            grapple.isAttached = true;
            grapple.hookPoint = mousePos;
            float dx = grapple.hookPoint.x - pilotPos.x;
            float dy = grapple.hookPoint.y - pilotPos.y;
            grapple.length = std::sqrt(dx*dx + dy*dy);
            
            // Если зацепились слишком далеко — сбрасываем трос
            if (grapple.length > 12.0f) {
                grapple.isAttached = false;
                isTacticalActive = false;
                return;
            }
            tacticalActiveTimer = 2.5f; // Полет длится до 2.5 секунд
            tacticalCooldown = 8.0f;
            break;

        case PilotClass::Stim:
            tacticalActiveTimer = 3.5f; // Стимулятор разгоняет системы на 3.5 сек
            tacticalCooldown = 12.0f;
            break;

        case PilotClass::PhaseShift:
            tacticalActiveTimer = 2.0f; // Уход в фазовое измерение на 2 секунды
            tacticalCooldown = 16.0f;
            break;

        case PilotClass::Cloak:
            tacticalActiveTimer = 6.0f; // Маскировочный камуфляж на 6 секунд
            tacticalCooldown = 18.0f;
            break;

        case PilotClass::AWall:
            // Развертывание Э-Стены прямо перед пилотом
            aWallShield.isDeployed = true;
            aWallShield.position = pilotPos;
            aWallShield.health = 200.0f;
            tacticalActiveTimer = 5.0f; // Стоит 5 секунд
            tacticalCooldown = 14.0f;
            break;

        case PilotClass::PulseBlade:
            // Бросок ножа: сканирует область вокруг точки падения
            isPulseBladeActive = true;
            pulseBladePos = mousePos;
            pulseBladeRadius = 5.5f;
            tacticalActiveTimer = 4.0f; // Подсветка целей на 4 секунды
            tacticalCooldown = 10.0f;
            break;

        case PilotClass::HoloPilot:
            tacticalActiveTimer = 4.5f; // Голограмма отвлекает рой 4.5 секунды
            tacticalCooldown = 9.0f;
            break;
    }
    UpdateActiveStats();
}

// Математический процессор физики Крюка-кошки (Grapple Hook Engine)
void Vault17ClassManager::ProcessGrapplePhysics(Vector3D& pilotPos, float deltaTime) {
    if (!grapple.isAttached || activePilotClass != PilotClass::Grapple || isInsideVehicle) return;

    float tdx = grapple.hookPoint.x - pilotPos.x;
    float tdy = grapple.hookPoint.y - pilotPos.y;
    float currentDist = std::sqrt(tdx * tdx + tdy * tdy);

    if (currentDist > 0.3f && tacticalActiveTimer > 0.1f) {
        // Сила натяжения троса: чем дальше, тем мощнее импульс притяжения
        float pullForce = 7.5f;
        grapple.velocity.x = (tdx / currentDist) * pullForce;
        grapple.velocity.y = (tdy / currentDist) * pullForce;
        
        // Симуляция высоты прыжка Z по параболе во время полета на тросе
        float progress = 1.0f - (currentDist / grapple.length);
        if (progress < 0.0f) progress = 0.0f;
        if (progress > 1.0f) progress = 1.0f;
        pilotPos.z = std::sin(progress * 3.1415926f) * 2.2f; // Взлет на 2.2 метра

        // Применяем физический сдвиг к координатам Скаута кадра
        pilotPos.x += grapple.velocity.x * deltaTime;
        pilotPos.y += grapple.velocity.y * deltaTime;
    } else {
        // Прилетели в точку или кончился таймер — обнуляем трос и высоту Z
        grapple.isAttached = false;
        isTacticalActive = false;
        pilotPos.z = 0.0f;
    }
}

void Vault17ClassManager::UpdateActiveStats() {
    if (!isInsideVehicle) {
        currentStats.isVehicleMode = false;
        currentStats.damageMultiplier = 1.0f;
        currentStats.erosionResistance = 0.0f;

        switch (activePilotClass) {
            case PilotClass::Grapple:
                currentStats.maxHealth = 100.0f; 
                currentStats.moveSpeed = 5.8f;
                currentStats.weaponLabel = L"PILOT: GRAPPLE GEAR"; break;
            case PilotClass::Cloak:
                currentStats.maxHealth = 100.0f; 
                currentStats.moveSpeed = 5.2f;
                currentStats.weaponLabel = isTacticalActive ? L"CLOAK ACTIVE [HIDDEN]" : L"STANDARD CARBINE"; break;
            case PilotClass::Stim:
                currentStats.maxHealth = 100.0f; 
                // Бешеное ускорение под стимулятором
                currentStats.moveSpeed = isTacticalActive ? 9.8f : 5.5f; 
                currentStats.weaponLabel = L"STIM LOADED CARBINE"; break;
            case PilotClass::PhaseShift:
                currentStats.maxHealth = 100.0f; 
                currentStats.moveSpeed = 5.5f;
                // Во время Фазы эрозия костюма ПОЛНОСТЬЮ блокируется (100% резист из GMyGameDoNotTouch)
                currentStats.erosionResistance = isTacticalActive ? 1.0f : 0.0f;
                currentStats.weaponLabel = L"PHASE EXPERIMENTAL RIFLE"; break;
            case PilotClass::AWall:
                currentStats.maxHealth = 100.0f; 
                currentStats.moveSpeed = 5.3f;
                // Усиление урона выстрелов сквозь Э-стену на 70%
                currentStats.damageMultiplier = isTacticalActive ? 1.7f : 1.0f; 
                currentStats.weaponLabel = L"AMP-TACTICAL CARBINE"; break;
            default:
                currentStats.maxHealth = 100.0f; 
                currentStats.moveSpeed = 5.5f;
                currentStats.weaponLabel = L"STANDARD CARBINE"; break;
        }
    } else {
        currentStats.isVehicleMode = true;
        currentStats.erosionResistance = 1.0f; 

        switch (activeTitanClass) {
            case TitanClass::Ion:       currentStats.maxHealth = 500.0f; currentStats.moveSpeed = 3.5f; currentStats.weaponLabel = L"ION: SPLITTER RIFLE"; break;
            case TitanClass::Scorch:    currentStats.maxHealth = 650.0f; currentStats.moveSpeed = 2.4f; currentStats.weaponLabel = L"SCORCH: THERMITE LAUNCHER"; break;
            case TitanClass::Northstar: currentStats.maxHealth = 350.0f; currentStats.moveSpeed = 4.5f; currentStats.weaponLabel = L"NORTHSTAR: PLASMA RAILGUN"; break;
            case TitanClass::Ronin:     currentStats.maxHealth = 350.0f; currentStats.moveSpeed = 4.8f; currentStats.weaponLabel = L"RONIN: LEADWALL SHOTGUN"; break;
            case TitanClass::Tone:      currentStats.maxHealth = 500.0f; currentStats.moveSpeed = 3.5f; currentStats.weaponLabel = L"TONE: 40mm TRACKING CANNON"; break;
            case TitanClass::Legion:    currentStats.maxHealth = 650.0f; currentStats.moveSpeed = 2.2f; currentStats.weaponLabel = L"LEGION: PREDATOR CANNON"; break;
            case TitanClass::Monarch:   currentStats.maxHealth = 500.0f; currentStats.moveSpeed = 3.6f; currentStats.weaponLabel = L"MONARCH: XO-16 UPGRADE CORE"; break;
        }
    }
}

// Реализация методов камеры
ScreenPoint IsometricCamera::WorldToScreen(const Vector3D& worldPos, const Vector3D& cameraTarget) const {
    float relX = worldPos.x - cameraTarget.x;
    float relY = worldPos.y - cameraTarget.y;
    float relZ = worldPos.z - cameraTarget.z;
    ScreenPoint screen;
    screen.x = (relX - relY) * ISO_COS;
    screen.y = (relX + relY) * ISO_SIN - relZ;
    screen.x = (screen.x * zoom) + centerOffset.x;
    screen.y = (screen.y * zoom) + centerOffset.y;
    return screen;
}

    Vector3D IsometricCamera::ScreenToWorldGround(const ScreenPoint& screenPos, const
    Vector3D& cameraTarget) const {
    float sx = (screenPos.x - centerOffset.x) / zoom;
    float sy = (screenPos.y - centerOffset.y) / zoom;
    Vector3D world;
    world.x = (sx / (2.0f * ISO_COS)) + (sy / (2.0f * ISO_SIN)) + cameraTarget.x;
    world.y = (sy / (2.0f * ISO_SIN)) - (sx / (2.0f * ISO_COS)) + cameraTarget.y;
    world.z = 0.0f;
    return world;
}