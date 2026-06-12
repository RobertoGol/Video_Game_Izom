#include "Tactics.hpp"

// ============================================================================
// РЕАЛИЗАЦИЯ МЕТОДОВ КЛАССA Vault17ClassManager
// ============================================================================

Vault17ClassManager::Vault17ClassManager() {
    activePilotClass = PilotClass::Grapple;
    activeTitanClass = TitanClass::Tone;
    isInsideVehicle = false;
    tacticalCooldown = 0.0f;
    tacticalActiveTimer = 0.0f;
    isTacticalActive = false;
    UpdateActiveStats();
}

void Vault17ClassManager::ChangePilotClass(PilotClass newClass) {
    if (!isInsideVehicle) {
        activePilotClass = newClass;
        isTacticalActive = false;
        tacticalActiveTimer = 0.0f;
        UpdateActiveStats();
    }
}

void Vault17ClassManager::ChangeTitanFirmware(TitanClass newClass) {
    activeTitanClass = newClass;
    if (isInsideVehicle) UpdateActiveStats();
}

void Vault17ClassManager::EnterVehicle() {
    isInsideVehicle = true;
    isTacticalActive = false; // Тактики пилота засыпают внутри герметичного Танка
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
            UpdateActiveStats(); // Сброс боевых баффов тактик после отката
        }
    }
}

void Vault17ClassManager::ActivateTacticalSkill() {
    if (isInsideVehicle || tacticalCooldown > 0.0f || isTacticalActive) return;
    isTacticalActive = true;
    
    switch (activePilotClass) {
        case PilotClass::Stim:       tacticalActiveTimer = 3.0f; tacticalCooldown = 12.0f; break;
        case PilotClass::PhaseShift: tacticalActiveTimer = 2.0f; tacticalCooldown = 15.0f; break;
        case PilotClass::Cloak:      tacticalActiveTimer = 6.0f; tacticalCooldown = 18.0f; break;
        case PilotClass::AWall:      tacticalActiveTimer = 5.0f; tacticalCooldown = 14.0f; break;
        default:                     tacticalActiveTimer = 1.0f; tacticalCooldown = 8.0f;  break;
    }
    UpdateActiveStats();
}

void Vault17ClassManager::UpdateActiveStats() {
    if (!isInsideVehicle) {
        currentStats.isVehicleMode = false;
        currentStats.damageMultiplier = 1.0f;
        currentStats.erosionResistance = 0.0f;

        switch (activePilotClass) {
            case PilotClass::Grapple:
                currentStats.maxHealth = 100.0f; currentStats.moveSpeed = 5.8f;
                currentStats.weaponLabel = L"GRAPPLE OPERATIVE CARBINE"; break;
            case PilotClass::Cloak:
                currentStats.maxHealth = 100.0f; currentStats.moveSpeed = 5.2f;
                currentStats.weaponLabel = isTacticalActive ? L"CLOAK FIELD ACTIVE" : L"STANDARD CARBINE"; break;
            case PilotClass::Stim:
                currentStats.maxHealth = 100.0f; currentStats.moveSpeed = isTacticalActive ? 9.5f : 5.5f; 
                currentStats.weaponLabel = L"STIMULANT INJECTED CARBINE"; break;
            case PilotClass::PhaseShift:
                currentStats.maxHealth = 100.0f; currentStats.moveSpeed = 5.5f;
                currentStats.erosionResistance = isTacticalActive ? 1.0f : 0.0f; // Во время фазы Скаут неуязвим к Эфиру
                currentStats.weaponLabel = L"PHASE EXPERIMENTAL RIFLE"; break;
            case PilotClass::AWall:
                currentStats.maxHealth = 100.0f; currentStats.moveSpeed = 5.3f;
                currentStats.damageMultiplier = isTacticalActive ? 1.7f : 1.0f; // Выстрелы сквозь Э-стену сильнее на 70%
                currentStats.weaponLabel = L"AMP-TACTICAL CARBINE"; break;
            default:
                currentStats.maxHealth = 100.0f; currentStats.moveSpeed = 5.5f;
                currentStats.weaponLabel = L"STANDARD CARBINE"; break;
        }
    } else {
        currentStats.isVehicleMode = true;
        currentStats.erosionResistance = 1.0f; // 100% герметичность Танка от Эфирной Эрозии секторов поверхности

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

std::wstring Vault17ClassManager::GetActiveClassNameW() const {
    if (!isInsideVehicle) {
        switch (activePilotClass) {
            case PilotClass::Grapple:    return L"TACTIC: GRAPPLE OPERATIVE";
            case PilotClass::Cloak:      return L"TACTIC: INFILTRATOR (CLOAK)";
            case PilotClass::Stim:       return L"TACTIC: BERSERKER (STIM)";
            case PilotClass::PhaseShift: return L"TACTIC: DIMENSION SHIFTER";
            case PilotClass::HoloPilot:  return L"TACTIC: DECOY MASTER";
            case PilotClass::AWall:      return L"TACTIC: FIRE-LINE LOCK (A-WALL)";
            case PilotClass::PulseBlade: return L"TACTIC: RECON SCOUT";
        }
    } else {
        switch (activeTitanClass) {
            case TitanClass::Ion:       return L"TITAN NET: ION [ATLAS]";
            case TitanClass::Scorch:    return L"TITAN NET: SCORCH [OGRE]";
            case TitanClass::Northstar: return L"TITAN NET: NORTHSTAR [STRIDER]";
            case TitanClass::Ronin:     return L"TITAN NET: RONIN [STRIDER]";
            case TitanClass::Tone:      return L"TITAN NET: TONE [ATLAS]";
            case TitanClass::Legion:    return L"TITAN NET: LEGION [OGRE]";
            case TitanClass::Monarch:   return L"TITAN NET: MONARCH [UPGRADE]";
        }
    }
    return L"UNKNOWN OVERLINK";
}

// ============================================================================
// РЕАЛИЗАЦИЯ МЕТОДОВ КЛАССA IsometricCamera
// ============================================================================

ScreenPoint IsometricCamera::WorldToScreen(const Vector3D& worldPos, const Vector3D& cameraTarget) const {
    float relX = worldPos.x - cameraTarget.x;
    float relY = worldPos.y - cameraTarget.y;
    float relZ = worldPos.z - cameraTarget.z;

    ScreenPoint screen;
    // Изометрический разворот осей в пространстве и сжатие по высоте Z
    screen.x = (relX - relY) * ISO_COS;
    screen.y = (relX + relY) * ISO_SIN - relZ;

    screen.x = (screen.x * zoom) + centerOffset.x;
    screen.y = (screen.y * zoom) + centerOffset.y;
    return screen;
}

Vector3D IsometricCamera::ScreenToWorldGround(const ScreenPoint& screenPos, const Vector3D& cameraTarget) const {
    float sx = (screenPos.x - centerOffset.x) / zoom;
    float sy = (screenPos.y - centerOffset.y) / zoom;

    Vector3D world;
    // Матричное решение системы уравнений обратной полуизометрической проекции при Z = 0
    world.x = (sx / (2.0f * ISO_COS)) + (sy / (2.0f * ISO_SIN)) + cameraTarget.x;
    world.y = (sy / (2.0f * ISO_SIN)) - (sx / (2.0f * ISO_COS)) + cameraTarget.y;
    world.z = 0.0f;
    return world;
}
