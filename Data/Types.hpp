#pragma once
#include <string>
#include <vector>

const int MAP_WIDTH = 20;
const int MAP_HEIGHT = 20;
const float EROSION_DAMAGE_THRESHOLD = 50.0f;

enum class AIState { Follow, Guard, Combat };
enum class TankWeaponMode { Cannon, AutoCannon };
enum class MissileStrikeMode { Ballistic, Artillery };
enum class BulletType { Standard, Pellet, BallisticMissile, ArtilleryMissile };
enum class UnitMode { Scout, Titan };

enum class PilotClass { Grapple, Cloak, Stim, PhaseShift, HoloPilot, AWall, PulseBlade };
enum class TitanClass { Ion, Scorch, Northstar, Ronin, Tone, Legion, Monarch };

struct Vector3D { float x = 0.0f; float y = 0.0f; float z = 0.0f; };
struct ScreenPoint { float x = 0.0f; float y = 0.0f; };
struct Vertex { float x, y, z; float r, g, b, a; };

struct Bullet {
    Vector3D start; Vector3D current; Vector3D direction;
    float speed = 28.0f; float distanceTraveled = 0.0f; float maxDistance = 16.0f;
    bool isAlive = true; BulletType type = BulletType::Standard;
    float splashRadius = 0.0f; Vector3D targetPos;
};

struct Enemy {
    Vector3D position; float health = 35.0f; float speed = 2.4f;
    bool isAlive = true; float radius = 0.35f;
};

struct TitanComponents {
    float coreEnergy = 100.0f; float tracksCondition = 100.0f;
    float turretStatus = 100.0f; float sensorLink = 100.0f;
};

struct TitanAlly {
    Vector3D position = { 2.0f, 2.0f, 0.0f };
    float health = 600.0f; float maxHealth = 600.0f; float speed = 3.2f; float fireCooldown = 0.0f;
    bool isPiloted = false; AIState aiState = AIState::Follow;
    TitanComponents systems;
    bool hasMissileModule = true;
    TankWeaponMode currentWeapon = TankWeaponMode::Cannon;
    MissileStrikeMode missileMode = MissileStrikeMode::Ballistic;
};

struct Vault17GridState {
    bool towerSyncRecovered = true; bool localRelayAvailable = true;
    bool feyRingGateUnlocked = true; float towerHealth = 200.0f;
};

struct StatModifiers {
    float moveSpeed = 5.5f; float maxHealth = 100.0f; float damageMultiplier = 1.0f;
    float erosionResistance = 0.0f; bool isVehicleMode = false;
    std::wstring weaponLabel = L"STANDARD CARBINE";
};

// ТИТАНФОЛЛ 2 & БУНКЕР v15: Дополнительные тактические стейты
struct VortexShieldState {
    bool isActive = false;
    float energy = 100.0f;
    int caughtBulletsCount = 0;
};

struct LockOnTarget {
    int enemyIndex = -1;
    int locksCount = 0;
    bool isFullyLocked = false;
};

struct Vault17Progression {
    bool hasFoundPipPad = false; // По лору v15 изначально равен false!
    Vector3D pipPadSpawnPos = { 8.0f, 6.0f, 0.0f }; // Координаты лежащего планшета
};
