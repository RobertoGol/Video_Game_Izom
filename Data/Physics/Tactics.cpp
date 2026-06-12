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
    
    grapple.isAttached = false;
    grapple.length = 0.0f;
    grapple.velocity = {0.0f, 0.0f, 0.0f};
    
    aWallShield.isDeployed = false;
    isPulseBladeActive = false;
    pulseBladeRadius = 0.0f;
    isPhaseDimensionActive = false;
    
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
        isPhaseDimensionActive = false;
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
    isPhaseDimensionActive = false;
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
    
    if (isTacticalActive) {
        tacticalActiveTimer -= deltaTime;
        if (tacticalActiveTimer <= 0.0f) {
            isTacticalActive = false;
            aWallShield.isDeployed = false;
            isPulseBladeActive = false;
            isPhaseDimensionActive = false; // Возвращаемся из Изнанки в реальный мир
            UpdateActiveStats();
        }
    }
    
    if (isTacticalActive && activePilotClass == PilotClass::Stim && !isInsideVehicle) {
        playerHealth = std::min(playerMaxHealth, playerHealth + 15.0f * deltaTime);
    }
}

void Vault17ClassManager::ActivateTacticalSkill(const Vector3D& mousePos, const Vector3D& pilotPos) {
    // Если Pip-Pad еще не найден — тактические умения костюма заблокированы!
    if (!bunkerProgression.hasFoundPipPad || isInsideVehicle || tacticalCooldown > 0.0f || isTacticalActive) return;
    
    isTacticalActive = true;
    
    switch (activePilotClass) {
        case PilotClass::Grapple:
            grapple.isAttached = true;
            grapple.hookPoint = mousePos;
            {
                float dx = grapple.hookPoint.x - pilotPos.x;
                float dy = grapple.hookPoint.y - pilotPos.y;
                grapple.length = std::sqrt(dx*dx + dy*dy);
                if (grapple.length > 12.0f) {
                    grapple.isAttached = false;
                    isTacticalActive = false;
                    return;
                }
            }
            tacticalActiveTimer = 2.0f;
            tacticalCooldown = 9.0f;
            break;

        case PilotClass::Stim:
            tacticalActiveTimer = 3.0f; 
            tacticalCooldown = 14.0f;
            break;

        case PilotClass::PhaseShift:
            // КАНOН TITANFALL 2: Проваливаемся в фазовое измерение
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

void Vault17ClassManager::ProcessGrapplePhysics(Vector3D& pilotPos, float deltaTime) {
    if (!grapple.isAttached || activePilotClass != PilotClass::Grapple || isInsideVehicle) return;

    float tdx = grapple.hookPoint.x - pilotPos.x;
    float tdy = grapple.hookPoint.y - pilotPos.y;
    float currentDist = std::sqrt(tdx * tdx + tdy * tdy);

    if (currentDist > 0.4f && tacticalActiveTimer > 0.05f) {
        float pullForce = 8.0f;
        grapple.velocity.x = (tdx / currentDist) * pullForce;
        grapple.velocity.y = (tdy / currentDist) * pullForce;
        
        float progress = 1.0f - (currentDist / grapple.length);
        pilotPos.z = std::sin(progress * 3.1415926f) * 1.8f; 

        pilotPos.x += grapple.velocity.x * deltaTime;
        pilotPos.y += grapple.velocity.y * deltaTime;
    } else {
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
                currentStats.maxHealth = 100.0f; currentStats.moveSpeed = 5.8f;
                currentStats.weaponLabel = L"PILOT: GRAPPLE GEAR"; break;
            case PilotClass::Stim:
                currentStats.maxHealth = 100.0f; 
                // ИСПРАВЛЕНO: Скорость строго 6.8f по канону баланса
                currentStats.moveSpeed = isTacticalActive ? 6.8f : 5.5f; 
                currentStats.weaponLabel = L"STIM CARBINE"; break;
            case PilotClass::PhaseShift:
                currentStats.maxHealth = 100.0f; currentStats.moveSpeed = 5.5f;
                currentStats.erosionResistance = isTacticalActive ? 1.0f : 0.0f; // 100% резист в фазе
                currentStats.weaponLabel = isPhaseDimensionActive ? L"PHASE DIMENSION" : L"PHASE RIFLE"; break;
            default:
                currentStats.maxHealth = 100.0f; currentStats.moveSpeed = 5.5f;
                currentStats.weaponLabel = L"STANDARD CARBINE"; break;
        }
    }
}
