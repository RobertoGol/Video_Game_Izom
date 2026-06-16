#pragma once
#include <string>
#include <vector>
#include "../Types.hpp" // Гарантирует определение Vector3D для всех тактик

namespace bunker
{

    // Силовые параметры троса Крюка-кошки (Grapple)
    struct GrapplePhysics
    {
        Vector3D hookPoint;
        float length = 0.0f;
        Vector3D velocity;
        bool isAttached = false;
        bool isActive = false;
    };

    // Защитный экран Э-ДАНЬ (A-Wall)
    struct DeployableShield
    {
        Vector3D position;
        float health = 150.0f;
        float currentHealth = 500.0f;
        float lifetime = 5.0f;
        bool isDeployed = false;
    };

    class Vault17ClassManager
    {
    private:
        // ПРАВИЛЬНО: Задаем стартовые классы прямо при объявлении переменных
        PilotClass activePilotClass = PilotClass::Grapple;
        TitanClass activeTitanClass = TitanClass::Tone;
        bool isInsideVehicle = false;

    public:
        StatModifiers currentStats;
        float tacticalCooldown = 0.0f;
        float tacticalActiveTimer = 0.0f;
        bool isTacticalActive = false;

        // Внутренние параметры тактик по лору Titanfall 2 (без дубликатов)
        GrapplePhysics grapple;
        DeployableShield aWallShield;
        Vector3D pulseBladePos;
        float pulseBladeDuration = 0.0f;
        float pulseBladeRadius = 0.0f;
        bool isPulseBladeActive = false;

        // Стейт Фазового сдвига (Titanfall 2 канон)
        bool isPhaseDimensionActive = false;

    public:
        Vault17ClassManager() = default; // Эту строчку компилятор теперь соберет идеально!

        void UpdateCooldowns(float deltaTime);
        void ActivateTacticalSkill(const Vector3D &mouseWorldPos, const Vector3D &pilotPos);
        void UpdateActiveStats();
        std::wstring GetActiveClassNameW() const;

        void ChangePilotClass(PilotClass newClass);
        void ChangeTitanFirmware(TitanClass newClass);
        void EnterVehicle();
        void ExitVehicle();

        PilotClass GetActivePilotClass() const;
        TitanClass GetActiveTitanClass() const;

        // Оптимизированная физика зацепа с ААВВ-клиппингом стен бункера
        void ProcessGrapplePhysics(Vector3D &pilotPos, float deltaTime);
    };

} // namespace bunker
