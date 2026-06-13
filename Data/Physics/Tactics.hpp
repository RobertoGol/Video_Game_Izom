#pragma once
#include <string>
#include <vector>

#include "../Types.hpp"// Гарантирует определение Vector3D для всех тактик // Нативный Types.hpp вашего движка

namespace bunker {
// Силовые параметры троса Крюка-кошки (Grapple)
struct GrapplePhysics {
    Vector3D hookPoint;  //  <-- тут ошибки 
    float length = 0.0f;
    Vector3D velocity;   //  <-- тут ошибки 
    bool isAttached = false;
    bool isActive = false;
};

// Защитный экран Э-ДАНЬ (A-Wall)
struct DeployableShield {
    Vector3D position;
    float health = 150.0f;
    float currentHealth = 500.0f;
    float lifetime = 5.0f;
    bool isDeployed = false;
};


class Vault17ClassManager {
private:
    GrapplePhysics m_Grapple;
    DeployableShield m_Shield;
    PilotClass activePilotClass;
    TitanClass activeTitanClass;
    bool isInsideVehicle;
public:
    Vector3D pulseBladePos;
    float pulseBladeDuration = 0.0f;
    StatModifiers currentStats;
    float tacticalCooldown;
    float tacticalActiveTimer;
    bool isTacticalActive;

    // Внутренние параметры тактик по лору Titanfall 2
    GrapplePhysics grapple;
    DeployableShield aWallShield;
    Vector3D pulseBladePos;  //  <-- тут ошибки 
    float pulseBladeRadius;
    bool isPulseBladeActive;
    
    // Стейт Фазового сдвига (Titanfall 2 канон)
    bool isPhaseDimensionActive

public:

    Vault17ClassManager() = default;
    void ProcessGrapplePhysics(Vector3D& pPos, float deltaTime);
    void UpdateCooldowns(float deltaTime);
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
    
    // Оптимизированная физика зацепа с ААВВ-клиппингом стен бункера
    void ProcessGrapplePhysics(Vector3D& pilotPos, float deltaTime);
};

class IsometricCamera {
public:
    const float ISO_COS = 0.8660254f; 
    const float ISO_SIN = 0.5000000f; 
    
    float zoom = 55.0f; 
    ScreenPoint centerOffset = { 0.0f, 0.0f };   //  <-- тут ошибки 

    ScreenPoint WorldToScreen(const Vector3D& worldPos, const Vector3D& cameraTarget) const;
    Vector3D ScreenToWorldGround(const ScreenPoint& screenPos, const Vector3D& cameraTarget) const;
};

} // namespace bunker
