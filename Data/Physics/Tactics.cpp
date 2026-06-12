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
    isPulseBladeActive = false;
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
            isPhaseDimensionActive = false; // Возвращаемся из Фазового измерения в реальный мир
            UpdateActiveStats();
        }
    }
    
    // Мягкая регенерация ХП от Стимулятора (Stim) во время действия
    if (isTacticalActive && activePilotClass == PilotClass::Stim && !isInsideVehicle) {
        playerHealth = std::min(playerMaxHealth, playerHealth + 15.0f * deltaTime);
    }
}

void Vault17ClassManager::ActivateTacticalSkill(const Vector3D& mousePos, const Vector3D& pilotPos) {
    if (isInsideVehicle || tacticalCooldown > 0.0f || isTacticalActive) return;
    
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
            // ТИТАНФОЛЛ 2 КАНОН: Проваливаемся в фазовую изнанку
            isPhaseDimensionActive = true; 
            tacticalActiveTimer = 2.5f; // Фаза длится 2.5 секунды
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

        case PilotClass::PulseBlade:
            isPulseBladeActive = true;
            pulseBladePos = mousePos;
            pulseBladeRadius = 5.5f;
            tacticalActiveTimer = 4.0f; 
            tacticalCooldown = 12.0f;
            break;

        case PilotClass::HoloPilot:
            tacticalActiveTimer = 4.0f; 
            tacticalCooldown = 10.0f;
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
        if (progress < 0.0f) progress = 0.0f;
        if (progress > 1.0f) progress = 1.0f;
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
            case PilotClass::Cloak:
                currentStats.maxHealth = 100.0f; currentStats.moveSpeed = 5.2f;
                currentStats.weaponLabel = isTacticalActive ? L"CLOAK FIELD ACTIVE" : L"STANDARD CARBINE"; break;
            case PilotClass::Stim:
                currentStats.maxHealth = 100.0f; 
                // Адекватное, сбалансированное тактическое ускорение Стима взамен 9.8f
                currentStats.moveSpeed = isTacticalActive ? 6.8f : 5.5f; 
                currentStats.weaponLabel = L"STIM CARBINE"; break;
            case PilotClass::PhaseShift:
                currentStats.maxHealth = 100.0f; currentStats.moveSpeed = 5.5f;
                // В Фазовом измерении урон от Эфирной Эрозии СЕКТОРОВ полностью поглощается (100% резист)
                currentStats.erosionResistance = isTacticalActive ? 1.0f : 0.0f;
                currentStats.weaponLabel = isPhaseDimensionActive ? L"PHASE SHIFT INDUCTION" : L"PHASE EXPERIMENTAL RIFLE"; break;
            case PilotClass::AWall:
                currentStats.maxHealth = 100.0f; currentStats.moveSpeed = 5.3f;
                currentStats.damageMultiplier = isTacticalActive ? 1.7f : 1.0f; 
                currentStats.weaponLabel = L"AMP-TACTICAL CARBINE"; break;
            default:
                currentStats.maxHealth = 100.0f; currentStats.moveSpeed = 5.5f;
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
