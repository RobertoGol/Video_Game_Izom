#pragma once
#include <string>
#include <vector>

namespace bunker { // ОТКРЫВАЕМ ПРОСТРАНСТВО ИМЕН ДЛЯ ВСЕГО ДВИЖКА

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
struct Vault17Progression { // Структура сюжетного прогресса Убежища 17
        bool hasFoundPipPad = false; // По лору v15 изначально равен false (игрок должен найти планшет)
        Vector3D pipPadSpawnPos = { 8.0f, 6.0f, 0.0f }; // Мировые координаты, где физически лежит планшет Pip-Pad
    };
    // === ГЛОБАЛЬНЫЙ ИНВЕНТАРНЫЙ КАТАЛОГ И ДЕТАЛИ СИСТЕМЫ ЛУТА (БЕЗ ДУБЛИКАТОВ!) ===
    // Перечисление типов предметов, которые могут существовать в игровом мире
    enum class ItemType { Weapon, Ammo, Medicine, Resource, Quest };
    // Единая структура инвентарной детали (описывает любой предмет в игре)
    struct InventoryItem {
        unsigned int itemID = 0; // Уникальный цифровой ID предмета для базы данных
        ItemType type = ItemType::Resource; // Категория предмета (по умолчанию — ресурс для крафта)
        int quantity = 1; // Количество предметов в данной пачке (стаке)
        float weightPerUnit = 0.1f; // Физическая масса одной единицы предмета (для ограничения веса рюкзака)
        std::wstring displayName = L"UNKNOWN ITEM"; // Локализованное имя предмета в широком формате UTF-16 (WString)
    };
    // Перечисление доступных строительных объектов для мастерской C.A.M.P.
    enum class CampObjectType { ConcreteWall, DefenseTurret, SupplyCrate };
    // Структура параметров строительного призрака-силуэта
    struct CampPreview {
        int tileX = 0; // Текущая координата X на сетке тайлов, куда наведен курсор
        int tileY = 0; // Текущая координата Y на сетке тайлов, куда наведен курсор
        CampObjectType activeType = CampObjectType::ConcreteWall; // Какую именно постройку сейчас выбрал игрок
        bool isPlacementValid = false; // Флаг проверки коллизий: true — силуэт зеленый (строить можно), false — красный
    };
    // Структура хранения векторов и индексов загруженной Wavefront 3D-геометрии
    struct ObjModelMesh {
        std::vector<Vector3D> vertices; // Массив всех трехмерных точек (вершин) модели в пространстве
        std::vector<int> indices; // Массив индексов (порядок соединения точек в треугольники для DirectX 11)
    };
    // Каноничные и правильные типы контейнеров, которые использует файл LootSystem.cpp
    enum class LootContainerType { WoodenCrate, IronSafe, DevVault };
    // Единая физическая структура контейнера (сундука) на карте
    struct LootContainer {
        Vector3D position; // Трехмерные координаты расположения сундука в игровом мире
        LootContainerType type = LootContainerType::WoodenCrate; // Тип сундука (по умолчанию — обычный деревянный ящик)
        bool isOpened = false; // Статус контейнера: true — вскрыт (анимация открыта), false — заперт
        float physicsRadius = 0.35f; // Физический радиус хитбокса сундука для обсчета коллизий Пилота и техники
        std::vector<InventoryItem> containsItems; // Вложенный список (массив) предметов, которые лежат внутри этого сундука
    };
} // namespace bunker // Закрываем пространство имен bunker в самом конце Types.hpp