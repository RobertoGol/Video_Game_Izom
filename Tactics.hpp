#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cmath>
#include <vector>
#include <algorithm>
#include <string>

// ============================================================================
// ГЛОБАЛЬНЫЕ КОНСТАНТЫ И КОНФИГУРАЦИЯ СЕКТОРОВ УБЕЖИЩА 17
// ============================================================================
const int MAP_WIDTH = 20;
const int MAP_HEIGHT = 20;
const float EROSION_DAMAGE_THRESHOLD = 50.0f;

enum class AIState { Follow, Guard, Combat };
enum class TankWeaponMode { Cannon, AutoCannon };
enum class MissileStrikeMode { Ballistic, Artillery };
enum class BulletType { Standard, Pellet, BallisticMissile, ArtilleryMissile };
enum class UnitMode { Scout, Titan };

// 7 каноничных классов Пилотов (Тактик) из Titanfall 2
enum class PilotClass {
    Grapple,      // Крюк-кошка
    Cloak,        // Маскировка (Невидимость)
    Stim,         // Стимулятор (Скорость + регенерация)
    PhaseShift,   // Фазовый сдвиг (Неуязвимость)
    HoloPilot,    // Голографический пилот (Копия)
    AWall,        // Оборонительная стена Э-ДАНЬ (Усиление урона)
    PulseBlade    // Пульсовый клинок (Радар силуэтов)
};

// 7 уникальных прошивок / боевых доктрин ИИ Танка из канона Titanfall 2
enum class TitanClass {
    Ion, Scorch, Northstar, Ronin, Tone, Legion, Monarch
};

// ============================================================================
// БАЗОВЫЕ МАТЕМАТИЧЕСКИЕ И ИГРОВЫЕ СТРУКТУРЫ СУЩНОСТЕЙ
// ============================================================================
struct Vector3D { 
    float x = 0.0f; 
    float y = 0.0f; 
    float z = 0.0f; 
};

struct ScreenPoint { 
    float x = 0.0f; 
    float y = 0.0f; 
};

struct Vertex {
    float x, y, z;
    float r, g, b, a;
};

struct Bullet {
    Vector3D start;
    Vector3D current;
    Vector3D direction;
    float speed = 28.0f;
    float distanceTraveled = 0.0f;
    float maxDistance = 16.0f;
    bool isAlive = true;
    BulletType type = BulletType::Standard;
    float splashRadius = 0.0f;
    Vector3D targetPos; 
};

struct Enemy {
    Vector3D position;
    float health = 35.0f;
    float speed = 2.4f;
    bool isAlive = true;
    float radius = 0.35f;
};

// Распределение повреждений по 4 внутренним узлам Танка из GMyGameDoNotTouch
struct TitanComponents {
    float coreEnergy = 100.0f;       // Ядро
    float tracksCondition = 100.0f;  // Гусеницы / Ходовая
    float turretStatus = 100.0f;     // Механизм турели пушки
    float sensorLink = 100.0f;       // Сенсоры / Реле связи
};

struct TitanAlly {
    Vector3D position = { 2.0f, 2.0f, 0.0f };
    float health = 600.0f;
    float maxHealth = 600.0f;
    float speed = 3.2f;
    float fireCooldown = 0.0f;
    bool isPiloted = false;               
    AIState aiState = AIState::Follow;    
    TitanComponents systems;              
    
    bool hasMissileModule = true;         
    TankWeaponMode currentWeapon = TankWeaponMode::Cannon;
    MissileStrikeMode missileMode = MissileStrikeMode::Ballistic;
};

struct Vault17GridState {
    bool towerSyncRecovered = true;   
    bool localRelayAvailable = true;  
    bool feyRingGateUnlocked = true;  
    float towerHealth = 200.0f;       
};

struct StatModifiers {
    float moveSpeed = 5.5f;
    float maxHealth = 100.0f;
    float damageMultiplier = 1.0f;
    float erosionResistance = 0.0f; 
    bool isVehicleMode = false;
    std::wstring weaponLabel = L"STANDARD CARBINE";
};

// ============================================================================
// КЛАСС МЕНЕДЖЕРА ТАКТИЧЕСКИХ КЛАССОВ (ОБЪЯВЛЕНИЕ)
// ============================================================================
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

    Vault17ClassManager();

    void ChangePilotClass(PilotClass newClass);
    void ChangeTitanFirmware(TitanClass newClass);
    void EnterVehicle();
    void ExitVehicle();

    PilotClass GetActivePilotClass() const;
    TitanClass GetActiveTitanClass() const;

    void UpdateCooldowns(float deltaTime);
    void ActivateTacticalSkill();
    void UpdateActiveStats();
    std::wstring GetActiveClassNameW() const;
};

// ============================================================================
// КЛАСС ИЗОМЕТРИЧЕСКОЙ КАМЕРА (ОБЪЯВЛЕНИЕ)
// ============================================================================
class IsometricCamera {
public:
    const float ISO_COS = 0.8660254f; 
    const float ISO_SIN = 0.5000000f; 
    
    float zoom; 
    ScreenPoint centerOffset; 

    ScreenPoint WorldToScreen(const Vector3D& worldPos, const Vector3D& cameraTarget) const;
    Vector3D ScreenToWorldGround(const ScreenPoint& screenPos, const Vector3D& cameraTarget) const;
};
