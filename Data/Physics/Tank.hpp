#pragma once
#include "../Types.hpp" // Нативный Types.hpp вашего движка

namespace bunker
{

    // Спецификация тяжелого аналогового вооружения по техпаспорту v15
    enum class AncientLoadout
    {
        XO16_SolidKinetic,     // Тяжелая автопушка на бездымном порохе (профиль XO-16)
        Scorch_ThermiteMortar, // Механический баллистический миномет (Мортира Скорч)
        Ion_SplitLaser_Vacuum  // Вакуумно-лучевая пушка на накопительных конденсаторах
    };

    // Профиль баллистических кулачков и клапанов для гидравлики рук БТ
    struct BallisticCamProfile
    {
        float hydraulic_pressure_bar; // Давление в гидросистеме для компенсации отдачи
        float recoil_decay_ms;        // Время затухания колебаний рессор
        float mechanical_lead_factor; // Угол упреждения для шестеренчатого привода
    };

    // Органы управления внутри герметичной кабины Авангарда
    struct PilotInputControls
    {
        float throttle_lever;       // Рычаг газа (0.0f до 1.0f)
        float steering_wheel_angle; // Угол поворота штурвала наведения
        bool trigger_pressed;       // Механический спуск оружия
        float hand_tremor_hz;       // Дрожание рук Пилота от стресса
    };

    // Физическое состояние грунта под ногами 40-тонной машины
    struct TerrainFrictionData
    {
        float surface_slickness;    // Скользкость грунта (0.0f = сухой бетон, 1.0f = лед/грязь)
        float structural_integrity; // Прочность почвы
    };

    class VanguardTitan
    {
    private:
        // Константы физики шасси БТ-7274
        const float m_TitanAcceleration = 4.0f;
        const float m_TitanDeceleration = 6.0f;
        const float m_ExclusionRadiusSq = 36.0f; // Квадрат 6.0 метров (Anti-Blocking сфера безопасности)

        // Внутренние физические векторы и ретро-профили
        Vector3D m_Velocity = {0.0f, 0.0f, 0.0f};
        AncientLoadout m_ActiveLoadout = AncientLoadout::XO16_SolidKinetic;
        BallisticCamProfile m_CurrentCamProfile;

        // Вортекс-щит (Вихревая камера удержания)
        bool m_VortexActive = false;
        float m_VortexEnergy = 100.0f;
        int m_CaughtBulletsCount = 0;

        // Параметры котла-реактора БТ (Аналоговое Ядро)
        float m_CoreChargePercentage = 0.0f;
        float m_BoilerSteamPressureBar = 350.0f;
        float m_CoolantTemperatureCelsius = 180.0f;
        bool m_CoreOverdriveActive = false;

        // Механические таймеры переключения кулачковых валов
        float m_ReMapTimer = 0.0f;
        bool m_IsReMapping = false;

    public:
        VanguardTitan();

        // Главный физический тик Авангарда: ИИ Протоколы, Гальванический мост и управление Danger Map
        void UpdateBT7274(float deltaTime);

        // БЛОК 1: МЕХАНИЧЕСКИЙ API RE-MAPPING (Смена калибров проворотом валов за 0.4 секунды)
        void TriggerMechanicalReMap(AncientLoadout newWeapon);

        // БЛОК 2: МОДУЛЬ УПРАВЛЕНИЯ ВИХРЕВЫМ ЩИТОМ (VORTEX SHIELD ENGINE)
        void UpdateVortexShield(bool isHoldingQ, float deltaTime);

        // БЛОК 3: ГАЛЬВАНИЧЕСКИЙ ИНТЕРФЕЙС ВЕДЕНИЯ БОЯ (TASK SPLITTING & BALANCE)
        PilotInputControls FilterAndStabilizeInputs(const PilotInputControls &rawInput, const TerrainFrictionData &terrain);

        // БЛОК 4: АКТИВАЦИЯ СУПЕР-СПОСОБНОСТИ ЯДРА (CORE ACTIVATION ENGINE)
        bool ValidateCoreOverdriveTrigger(float targetArmorHp, float timeToCover);
        void ExecuteCoreOverdrive();

        // Автономная навигация: Расчет Combat Anchor Point (Агрессивный танкинг под углом 40%)
        Vector3D CalculateCombatAnchorPoint();

        // Геттеры состояния
        bool IsVortexActive() const { return m_VortexActive; }
        float GetVortexEnergy() const { return m_VortexEnergy; }
        bool IsCoreActive() const { return m_CoreOverdriveActive; }
        float GetCoreCharge() const { return m_CoreChargePercentage; }
        AncientLoadout GetLoadout() const { return m_ActiveLoadout; }
    };

} // namespace bunker
