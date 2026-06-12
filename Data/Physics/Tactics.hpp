#pragma once
#include <string>
#include <vector>
#include "../Types.hpp"

struct GrapplePhysics {
    bool isAttached = false;
    Vector3D hookPoint;
    float length = 0.0f;
    Vector3D velocity; 
};

struct DeployableShield {
    bool isDeployed = false;
    Vector3D position;
    float health = 150.0f;
    float lifetime = 5.0f;
};

class Vault17ClassManager {
private:
    PilotClass activePilotClass;
    TitanClass activeTitanClass;
    bool isInsideVehicle;

public:
    StatModifiers currentStats;
    float tacticalCooldown;
    float tacticalActiveTimer;
    bool isTacticalActive;

    // Внутренние параметры каноничных тактик Titanfall 2
    GrapplePhysics grapple;
    DeployableShield aWallShield;
    Vector3D pulseBladePos;
    float pulseBladeRadius;
    bool isPulseBladeActive;
    
    // Стейт Фазового сдвига (Titanfall 2 канон)
    bool isPhaseDimensionActive; 

    Vault17ClassManager();

    void ChangePilotClass(PilotClass newClass);
    void ChangeTitanFirmware(TitanClass newClass);
    void EnterVehicle();
    void ExitVehicle();

    PilotClass GetActivePilotClass() const;
    TitanClass GetActiveTitanClass() const;

    void UpdateCooldowns(float deltaTime);
    void ActivateTacticalSkill(const Vector3D& mouseWorldPos, const Vector3D& pilotPos);
    void UpdateActiveStats();
    std::wstring GetActiveClassNameW() const;
    
    void ProcessGrapplePhysics(Vector3D& pilotPos, float deltaTime);
};

class IsometricCamera {
public:
    const float ISO_COS = 0.8660254f; 
    const float ISO_SIN = 0.5000000f; 
    
    float zoom; 
    ScreenPoint centerOffset; 

    ScreenPoint WorldToScreen(const Vector3D& worldPos, const Vector3D& cameraTarget) const;
    Vector3D ScreenToWorldGround(const ScreenPoint& screenPos, const Vector3D& cameraTarget) const;
};
