#pragma once
#include <string>
#include <vector>

// Структура для хранения физических параметров Крюка-кошки
struct GrapplePhysics {
    bool isAttached = false;
    Vector3D hookPoint;
    float length = 0.0f;
    Vector3D velocity; // Внутренний вектор импульса полета
};

// Структура для Оборонительной стены Э-ДАНЬ (A-Wall)
struct DeployableShield {
    bool isDeployed = false;
    Vector3D position;
    float health = 150.0f;
    float lifetime = 5.0f;
};

// Расширенный класс менеджера тактик из лора GMyGameDoNotTouch
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

    // Специфические объекты физики тактик
    GrapplePhysics grapple;
    DeployableShield aWallShield;
    Vector3D pulseBladePos;
    float pulseBladeRadius;
    bool isPulseBladeActive;

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
    
    // Внутренние физические процессоры умений
    void ProcessGrapplePhysics(Vector3D& pilotPos, float deltaTime);
};

// Класс изометрической камеры
class IsometricCamera {
public:
    const float ISO_COS = 0.8660254f; 
    const float ISO_SIN = 0.5000000f; 
    
    float zoom; 
    ScreenPoint centerOffset; 

    ScreenPoint WorldToScreen(const Vector3D& worldPos, const Vector3D& cameraTarget) const;
    Vector3D ScreenToWorldGround(const ScreenPoint& screenPos, const Vector3D& cameraTarget) const;
};
