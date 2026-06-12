#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cmath>
#include <vector>
#include <algorithm>
#include <string>

// ============================================================================
// ГЛОБАЛЬНЫЕ КОНСТАНТЫ И НАСТРОЙКИ СЕКТОРОВ УБЕЖИЩА 17
// ============================================================================
const int MAP_WIDTH = 20;
const int MAP_HEIGHT = 20;
const float EROSION_DAMAGE_THRESHOLD = 50.0f;

enum class AIState { Follow, Guard, Combat };
enum class TankWeaponMode { Cannon, AutoCannon };
enum class MissileStrikeMode { Ballistic, Artillery };
enum class BulletType { Standard, Pellet, BallisticMissile, ArtilleryMissile };
enum class UnitMode { Scout, Titan };

// 7 Каноничных тактических классов Пилотов из Titanfall 2
enum class PilotClass {
    Grapple,      // Крюк-кошка (Высокая мобильность)
    Cloak,        // Маскировка (Невидимость для Верминов)
    Stim,         // Стимулятор (Ускорение + мгновенный реген ХП)
    PhaseShift,   // Фазовый сдвиг (Временная неуязвимость / другое измерение)
    HoloPilot,    // Голографический пилот (Создание бегущего двойника)
    AWall,        // Оборонительная стена Э-ДАНЬ (Усиление урона сквозь нее)
    PulseBlade    // Пульсовый клинок (Временная подсветка угроз на HUD)
};

// 7 Уникальных прошивок / боевых доктрин ИИ Танка из канона Titanfall 2
enum class TitanClass {
    Ion,          // Ион (Энергетическое оружие, лазер)
    Scorch,       // Скорч (Термитовые заряды, контроль территории)
    Northstar,    // Нортстар (Плазменная рельсовая пушка, снайпер)
    Ronin,        // Ронин (Маневренное шасси, дробовик и меч ближнего боя)
    Tone,         // Тон (Постоянный настильный урон, управляемые ракеты)
    Legion,       // Легион (Тяжелая хищная пушка-миниган подавления)
    Monarch       // Монарх (Класс поддержки, восстановление щитов и апгрейды)
};

// ============================================================================
// БАЗОВЫЕ МАТЕМАТИЧЕСКИЕ И ИГРОВЫЕ СТРУКТУРЫ
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
    Vector3D targetPos; // Целевая точка для артиллерийских парабол
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
    float coreEnergy = 100.0f;       // Ядро (Критический узел)
    float tracksCondition = 100.0f;  // Гусеницы / Ходовая (Скорость до 30%)
    float turretStatus = 100.0f;     // Механизм турели пушки (Разброс и КД)
    float sensorLink = 100.0f;       // Сенсоры / Реле связи (Гасит радар на HUD)
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
    bool towerSyncRecovered = true;   // Вышка связи сектора
    bool localRelayAvailable = true;  // Локальное реле Pip-Pad
    bool feyRingGateUnlocked = true;  // Врата FeyRing
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
// КЛАСС МЕНЕДЖЕРА СИСТЕМ И ТАКТИК (ОБЪЯВЛЕНИЕ)
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
// ИЗОМЕТРИЧЕСКАЯ КАМЕРА И СИСТЕМА ОБРАТНОЙ ПРОЕКЦИИ (ОБЪЯВЛЕНИЕ)
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
